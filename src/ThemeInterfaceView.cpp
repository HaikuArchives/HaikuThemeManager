/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <DataIO.h>
#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <BeBuild.h>

#ifdef __HAIKU__
#include <locale/Catalog.h>
#endif

#ifdef B_ZETA_VERSION
#include <add-ons/pref_app/PrefPanel.h>
#include <locale/Locale.h>
#include <Separator.h>
#define B_TRANSLATE _T
#else
#define B_PREF_APP_ENABLE_REVERT 'zPAE'
#define B_PREF_APP_SET_DEFAULTS 'zPAD'
#define B_PREF_APP_REVERT 'zPAR'
#define B_PREF_APP_ADDON_REF 'zPAA'
#endif

#ifndef B_TRANSLATE
#define B_TRANSLATE(v) v
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ThemeInterfaceView"

#include <Resources.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <Roster.h>

#include <Button.h>
#include <CheckBox.h>
#include <ListView.h>
#include <interface/StringView.h>
#include <ScrollView.h>
#include <TabView.h>
#include <TextControl.h>
#include <TextView.h>

#include <Application.h>
#include <MessageFilter.h>

#include <stdio.h>

#include "UITheme.h"

extern status_t ScaleBitmap(const BBitmap& inBitmap, BBitmap& outBitmap);

/* gui */
//#include "ThemeSelector.h"
#include "ThemeInterfaceView.h"
#include "ThemeItem.h"
#include "ThemeAddonItem.h"
#include "ScreenshotView.h"

/* impl */
#include "ThemeManager.h"

const uint32 kThemeChanged		= 'mThC';
const uint32 kApplyThemeBtn		= 'TmAp';
const uint32 kCreateThemeBtn	= 'TmCr';
const uint32 kReallyCreateTheme	= 'TmCR';
const uint32 kSaveThemeBtn		= 'TmSa';
const uint32 kDeleteThemeBtn	= 'TmDe';
const uint32 kThemeSelected		= 'TmTS';
const uint32 kMakeScreenshot	= 'TmMS';

const uint32 kHideSSPulse		= 'TmH1';
const uint32 kShowSSPulse		= 'TmH2';

static const uint32 skOnlineThemes	= 'TmOL';
static const char* skThemeURL		= "http://www.zeta-os.com/cms/download.php?list.4";
static const char* kHaikuDepotSig	= "application/x-vnd.Haiku-HaikuDepot";

#define HIDESS_OFFSET (Bounds().Width()/2 - 130)


// #pragma mark - refs_filter


filter_result
refs_filter(BMessage *message, BHandler **handler, BMessageFilter *filter)
{
	(void)handler;
	(void)filter;
	switch (message->what) {
		case B_REFS_RECEIVED:
			message->PrintToStream();
			break;
	}
	return B_DISPATCH_MESSAGE;
}


// #pragma mark -


//extern "C" BView *get_pref_view(const BRect& Bounds)
extern "C" BView *
themes_pref(const BRect& Bounds)
{
	return new ThemeInterfaceView(Bounds);
}


// #pragma mark - ThemeInterfaceView


ThemeInterfaceView::ThemeInterfaceView(BRect _bounds)
	: BView(_bounds, "Themes", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fThemeManager(NULL),
	fScreenshotPaneHidden(false),
	fHasScreenshot(false),
	fScreenshotTab(NULL)
{
/*
	BMessageFilter *filt = new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, refs_filter);
	be_app->Lock();
	be_app->AddCommonFilter(filt);
	be_app->Unlock();
*/
}


ThemeInterfaceView::~ThemeInterfaceView()
{
	delete fThemeManager;
}


void
ThemeInterfaceView::AllAttached()
{
	BView::AllAttached();

	SetViewPanelBgColor();
	
	fThemeManager = new ThemeManager;

	BRect frame = Bounds();
	frame.InsetBy(10.0, 10.0);
	
	// add the theme listview
	BRect list_frame = frame;
	list_frame.right = 130;
	fThemeList = new BListView(list_frame.InsetByCopy(3.0, 3.0), "themelist");
	fThemeListSV = new BScrollView("themelistsv", fThemeList, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true);
	AddChild(fThemeListSV);
	fThemeList->SetSelectionMessage(new BMessage(kThemeSelected));
	fThemeList->SetInvocationMessage(new BMessage(kApplyThemeBtn));
	fThemeList->SetTarget(this);

	// buttons...
	fNewBtn = new BButton(BRect(), "create", B_TRANSLATE("New"), new BMessage(kCreateThemeBtn));
	AddChild(fNewBtn);
	fNewBtn->SetTarget(this);
	fNewBtn->ResizeToPreferred();
	fNewBtn->MoveTo(fThemeListSV->Frame().right + 15.0, frame.bottom - fNewBtn->Bounds().Height());
	BPoint lt = fNewBtn->Frame().LeftTop();

	fNameText = new BTextControl(BRect(), "text", "", "My Theme", new BMessage(kCreateThemeBtn));
	AddChild(fNameText);
	fNameText->SetTarget(this);
	fNameText->ResizeToPreferred();
	// default is a bit small
	fNameText->ResizeBy(fNameText->Bounds().Width(), 0);
	fNameText->MoveTo(lt);
	fNameText->MoveBy(0, (fNewBtn->Bounds().Height() - fNameText->Bounds().Height()) / 2);
	//fNameText->MoveBy(0, - fNewBtn->Bounds().Height());
	fNameText->Hide();

	lt.x = fNewBtn->Frame().right + 10.0;
	fSaveBtn = new BButton(BRect(), "save", B_TRANSLATE("Save"), new BMessage(kSaveThemeBtn));
	AddChild(fSaveBtn);
	fSaveBtn->SetTarget(this);
	fSaveBtn->ResizeToPreferred();
	fSaveBtn->MoveTo(lt);

	lt.x = fSaveBtn->Frame().right + 10.0;
	fDeleteBtn = new BButton(BRect(), "delete", B_TRANSLATE("Delete"), new BMessage(kDeleteThemeBtn));
	AddChild(fDeleteBtn);
	fDeleteBtn->SetTarget(this);
	fDeleteBtn->ResizeToPreferred();
	fDeleteBtn->MoveTo(lt);

	// buttons...
	fSetShotBtn = new BButton(BRect(), "makeshot", B_TRANSLATE("Add screenshot"), new BMessage(kMakeScreenshot), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fSetShotBtn);
	fSetShotBtn->SetTarget(this);
	fSetShotBtn->ResizeToPreferred();
	
	fMoreThemesBtn = new BButton(BRect(), "getthemes", B_TRANSLATE("More themes"), new BMessage(skOnlineThemes), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fMoreThemesBtn);
	fMoreThemesBtn->SetTarget(this);
	fMoreThemesBtn->ResizeToPreferred();

	fDefaultsBtn = new BButton(BRect(), "defaults", B_TRANSLATE("Defaults"), new BMessage(B_PREF_APP_SET_DEFAULTS), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fDefaultsBtn);
	fDefaultsBtn->ResizeToPreferred();
	fDefaultsBtn->SetTarget(this);

	fApplyBtn = new BButton(BRect(), "apply", B_TRANSLATE("Apply"), new BMessage(kApplyThemeBtn), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fApplyBtn);
	fApplyBtn->ResizeToPreferred();
	fApplyBtn->SetTarget(this);

	float widest = max_c(fSetShotBtn->Bounds().Width(), fMoreThemesBtn->Bounds().Width());
	widest = max_c(widest, fDefaultsBtn->Bounds().Width());
	widest = max_c(widest, fApplyBtn->Bounds().Width());
	float height = fSetShotBtn->Bounds().Height();
	fSetShotBtn->ResizeTo(widest, height);
	fMoreThemesBtn->ResizeTo(widest, height);
	fDefaultsBtn->ResizeTo(widest, height);
	fApplyBtn->ResizeTo(widest, height);
	
	fSetShotBtn->MoveTo(frame.right - widest, frame.top + 5.0);
	fMoreThemesBtn->MoveTo(frame.right - widest, fSetShotBtn->Frame().bottom + 10.0);
	fApplyBtn->MoveTo(frame.right - widest, fNewBtn->Frame().top - fApplyBtn->Bounds().Height() - 10);
	fDefaultsBtn->MoveTo(frame.right - widest, fNewBtn->Frame().top - (fApplyBtn->Bounds().Height() + 10) * 2);

	// add the preview screen
	BRect preview_frame(fNewBtn->Frame().left, fThemeListSV->Frame().top, frame.right - widest - 10, fNewBtn->Frame().top - 10);

	fTabView = new BTabView(preview_frame, "tabs");
	fTabView->SetViewPanelBgColor();
	AddChild(fTabView);

	preview_frame = fTabView->ContainerView()->Bounds();

	fScreenshotTab = new BView(preview_frame, B_TRANSLATE("Screenshot"), B_FOLLOW_ALL, B_WILL_DRAW);
	fScreenshotTab->SetViewPanelBgColor();
	//AddChild(fScreenshotTab);
	fTabView->AddTab(fScreenshotTab);
	fTabView->Select(0L);

	fScreenshotPane = new ScreenshotView(preview_frame, "screenshot", B_FOLLOW_ALL, B_WILL_DRAW);
	fScreenshotTab->AddChild(fScreenshotPane);
	fScreenshotPane->SetViewPanelBgColor();
	
	fScreenshotText = new BStringView(BRect(), "sshotnone", B_TRANSLATE("No theme selected"), B_FOLLOW_ALL);
	fScreenshotText->SetFontSize(20.0);
	fScreenshotText->SetAlignment(B_ALIGN_CENTER);
	fScreenshotTab->AddChild(fScreenshotText);
	fScreenshotText->SetViewPanelBgColor();
	fScreenshotText->ResizeToPreferred();
	fScreenshotText->ResizeTo(fScreenshotTab->Bounds().Width() - 10.0, fScreenshotText->Bounds().Height());
	fScreenshotText->MoveTo(fScreenshotTab->Bounds().left + 5.0,
							((fScreenshotTab->Frame().Height() - fScreenshotText->Frame().Height()) / 2.0));


	// TODO: real preview from actual data and BControlLook & co
	//fTabView->AddTab(new BStringView(fTabView->ContainerView()->Bounds(), B_TRANSLATE("Preview"), "TODO", B_FOLLOW_ALL, B_WILL_DRAW));


	// TODO: theme informations
	fDetails = new BTextView(preview_frame, B_TRANSLATE("Details"), preview_frame.InsetByCopy(3,3), B_FOLLOW_ALL, B_WILL_DRAW);
	fDetails->SetText("TODO");
	fTabView->AddTab(fDetails);

	// Theme hyperlink
	/*
	BStringView* hlink = new BStringView(BRect(), "theme_hyperlink", B_TRANSLATE("More themes online"), new BMessage(skOnlineThemes), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(hlink);
	hlink->SetClickText(hlink->GetText(), *this);
	hlink->ResizeToPreferred();
	hlink->MoveTo(frame.right - hlink->Bounds().Width(), fNewBtn->Frame().top + 5);
	*/
	
	// the addons list view

	preview_frame = fTabView->ContainerView()->Bounds();
	preview_frame.right -= B_V_SCROLL_BAR_WIDTH;
	fAddonList = new BListView(preview_frame/*BRect()*/, "addonlist");

	PopulateAddonList();

	fAddonListSV = new BScrollView(B_TRANSLATE("Options"), fAddonList, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true);
	fAddonList->SetSelectionMessage(new BMessage(kThemeSelected));
	fAddonList->SetInvocationMessage(new BMessage(kApplyThemeBtn));
	fAddonList->SetTarget(this);	


	fTabView->AddTab(fAddonListSV);

	PopulateThemeList();
}


void
ThemeInterfaceView::MessageReceived(BMessage *_msg)
{
	ThemeManager *tman;
	int32 value;
	int32 id;

_msg->PrintToStream();

	switch(_msg->what)
	{
		case B_REFS_RECEIVED:
			_msg->PrintToStream();
			break;

		case kMakeScreenshot:
			AddScreenshot();
			break;

		case kThemeSelected:
			ThemeSelected();
			break;

		case kApplyThemeBtn:
			ApplySelected();
			break;

		case kCreateThemeBtn:
		if (fNameText->IsHidden()) {
			float w = fNameText->Bounds().Width() + 10.0;
			fNameText->Show();
			fNameText->MakeFocus();
			fNewBtn->MoveBy(w, 0);
			fSaveBtn->MoveBy(w, 0);
			fDeleteBtn->MoveBy(w, 0);
			break;
		} else {
			float w = fNameText->Bounds().Width() + 10.0;
			fNameText->Hide();
			fNameText->MakeFocus(false);
			fNewBtn->MoveBy(-w, 0);
			fSaveBtn->MoveBy(-w, 0);
			fDeleteBtn->MoveBy(-w, 0);
		}
		/* FALLTHROUGH */

		case kReallyCreateTheme:
			CreateNew(fNameText->Text());
			break;

		case kSaveThemeBtn:
			SaveSelected();
			break;

		case kDeleteThemeBtn:
			DeleteSelected();
			break;
#if 0
		case kColorsChanged:
		case kGeneralChanged:
		case kFontsChanged:
		{
			BMessenger msgr (Parent());
			msgr.SendMessage(B_PREF_APP_ENABLE_REVERT);
			BMessage changes;
			if (_msg->FindMessage("changes", &changes) == B_OK)
			{
				update_ui_settings(changes);
			}
			break;
		}
#endif
		case B_PREF_APP_SET_DEFAULTS:
		{
			ApplyDefaults();
			break;
		}
		
		case B_PREF_APP_REVERT:
		{
			Revert();
			break;
		}
		
		case B_PREF_APP_ADDON_REF:
		{
			break;
		}

		case kThemeChanged:
		{
/*			BMessage data;
			BMessage names;
			get_ui_settings(&data, &names);
			fColorSelector->Update(data);
			fFontSelector->Refresh();
			fGeneralSelector->Refresh(data);
*/
			break;
		}

		case CB_APPLY:
			tman = GetThemeManager();
			_msg->PrintToStream();
			if (_msg->FindInt32("be:value", &value) < B_OK)
				value = false;
			if (_msg->FindInt32("addon", &id) < B_OK)
				break;

			if (id > -1) {
				tman->SetAddonFlags(id, (tman->AddonFlags(id) & ~Z_THEME_ADDON_DO_SET_ALL) | (value?Z_THEME_ADDON_DO_SET_ALL:0));
			} else {
				// apply globally
				int32 i;
				for (i = fAddonList->CountItems() - 1; i > 0; i--) {
					ThemeAddonItem *item = static_cast<ThemeAddonItem *>(fAddonList->ItemAt(i));
					item->ApplyBox()->SetValue(value);
					tman->SetAddonFlags(item->AddonId(), (tman->AddonFlags(item->AddonId()) & ~Z_THEME_ADDON_DO_SET_ALL) | (value?Z_THEME_ADDON_DO_SET_ALL:0));
				}
			}
			break;

		case CB_SAVE:
			tman = GetThemeManager();
			_msg->PrintToStream();
			if (_msg->FindInt32("be:value", &value) < B_OK)
				value = false;
			if (_msg->FindInt32("addon", &id) < B_OK)
				break;

			if (id > -1) {
				tman->SetAddonFlags(id, (tman->AddonFlags(id) & ~Z_THEME_ADDON_DO_RETRIEVE) | (value?Z_THEME_ADDON_DO_RETRIEVE:0));
			} else {
				// apply globally
				int32 i;
				for (i = fAddonList->CountItems() - 1; i > 0; i--) {
					ThemeAddonItem *item = static_cast<ThemeAddonItem *>(fAddonList->ItemAt(i));
					item->SaveBox()->SetValue(value);
					tman->SetAddonFlags(item->AddonId(), (tman->AddonFlags(item->AddonId()) & ~Z_THEME_ADDON_DO_RETRIEVE) | (value?Z_THEME_ADDON_DO_RETRIEVE:0));
				}
			}
			break;

		case BTN_PREFS:
			tman = GetThemeManager();
			if (_msg->FindInt32("addon", &id) < B_OK)
				break;
			tman->RunPreferencesPanel(id);
			break;

		case kHideSSPulse:
			break;
			
		case kShowSSPulse:
			break;
		
		case skOnlineThemes:
		{
			/*
			ZETA code:
			be_roster->Launch( "application/x-vnd.Mozilla-Firefox", 1,
				(char **)&skThemeURL);
			*/
			if (!be_roster->IsRunning(kHaikuDepotSig)) {
				be_roster->Launch(kHaikuDepotSig);
				snooze(1000000);
			}
			BMessenger msgr(kHaikuDepotSig);
			BMessage message(B_SET_PROPERTY);
			message.AddString("data", "_theme");
			message.AddSpecifier("Value");
			message.AddSpecifier("View", "search terms");
			message.AddSpecifier("Window", (int32)0);
			msgr.SendMessage(&message);
			break;
		}
		default:
		{
			BView::MessageReceived(_msg);
			break;
		}
	}
}


ThemeManager* 
ThemeInterfaceView::GetThemeManager()
{
	return fThemeManager;
}


void
ThemeInterfaceView::PopulateThemeList()
{
	int i;
	BControl *c;
	for (i = 0; ChildAt(i); i++) {
		c = dynamic_cast<BControl *>(ChildAt(i));
		if (c)
			c->SetEnabled(false);
	}
	thread_id tid = spawn_thread(_ThemeListPopulatorTh, "ThemeListPopulator", 
		B_LOW_PRIORITY, this);
	resume_thread(tid);
}


int32
ThemeInterfaceView::_ThemeListPopulatorTh(void *arg)
{
	ThemeInterfaceView *_this = (ThemeInterfaceView *)arg;
	_this->_ThemeListPopulator();
	return 0;
}


void
ThemeInterfaceView::_ThemeListPopulator()
{
	status_t err;
	int32 i, count;
	int32 importer;
	BString name;
	ThemeItem *ti;
	bool isro;
	BStringItem *si;

	LockLooper();
	fThemeList->MakeEmpty();
	UnlockLooper();

	ThemeManager* tman = GetThemeManager();
	tman->LoadThemes();

	count = tman->CountThemes();
	

	LockLooper();

	si = new BStringItem("(System themes)");
	si->SetEnabled(false);
	fThemeList->AddItem(si);
	si = NULL; // first non-readonly item will set it again

	// native themes
	for (i = 0; i < count; i++) {
		err = tman->ThemeName(i, name);
		isro = tman->ThemeIsReadOnly(i);
		if (err)
			continue;

		if (!isro && si == NULL) {
			si = new BStringItem("(User themes)");
			si->SetEnabled(false);
			fThemeList->AddItem(si);
		}

		ti = new ThemeItem(i, name.String(), isro);
		fThemeList->AddItem(ti);
	}

	UnlockLooper();

	// for each importer
	for (importer = 0; importer < tman->CountThemeImporters(); importer++) {
		err = tman->ImportThemesFor(importer);
		if (err < 0)
			continue;
		PRINT(("Imports for %s: %d\n", tman->ThemeImporterAt(importer), (tman->CountThemes() - count)));
		if (tman->CountThemes() == count)
			continue; // nothing found

		// separator item
		name = "Imported (";
		name << tman->ThemeImporterAt(importer) << ")";
		si = new BStringItem(name.String());
		si->SetEnabled(false);
		LockLooper();
		fThemeList->AddItem(si);
		UnlockLooper();

		// add new themes
		count = tman->CountThemes();
		// we reuse i from where it was left
		for (; i < count; i++) {
			err = tman->ThemeName(i, name);
			isro = true;// importers can't save themes back
			if (err)
				continue;
			ti = new ThemeItem(i, name.String(), isro);
			LockLooper();
			fThemeList->AddItem(ti);
			UnlockLooper();
			// rest a bit
			snooze(1000);
		}
	}

	// enable controls again
	BControl *c;
	LockLooper();
	for (i = 0; ChildAt(i); i++) {
		c = dynamic_cast<BControl *>(ChildAt(i));
		if (c)
			c->SetEnabled(true);
	}
	UnlockLooper();
}


void
ThemeInterfaceView::PopulateAddonList()
{
	int32 i, count;
	ViewItem *vi;
	ThemeManager* tman = GetThemeManager();

	fAddonList->MakeEmpty();

	// global item
	vi = new ThemeAddonItem(BRect(0,0,200,52), this, -1);
	fAddonList->AddItem(vi);

	count = tman->CountAddons();
	for (i = 0; i < count; i++) {
		vi = new ThemeAddonItem(BRect(0,0,200,52), this, i);
		fAddonList->AddItem(vi);
	}
}


status_t
ThemeInterfaceView::Revert()
{
	status_t err = B_OK;
	ThemeManager* tman = GetThemeManager();
	
	if (tman->CanRevert())
		err = tman->RestoreCurrent();
	if (err)
		return err;
	
	return B_OK;
}


status_t
ThemeInterfaceView::ApplyDefaults()
{
	status_t err = B_OK;
	ThemeManager* tman = GetThemeManager();
	
	SetIsRevertable();

	err = tman->ApplyDefaultTheme();
	return err;
}


status_t
ThemeInterfaceView::ApplySelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return ENOENT;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return ENOENT;
	id = item->ThemeId();
	if (id < 0)
		return ENOENT;
	SetIsRevertable();
	err = tman->ApplyTheme(id);
	return err;
}


status_t
ThemeInterfaceView::CreateNew(const char *name)
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *ti;
	BString n(name);
	
	id = tman->MakeTheme();
	if (id < 0)
		return B_ERROR;
	err = tman->SetThemeName(id, name);
	if (err)
		return err;
	err = tman->ThemeName(id, n);
	if (err)
		return err;
	err = tman->SaveTheme(id, true);
	if (err)
		return err;
	ti = new ThemeItem(id, n.String(), false);
	fThemeList->AddItem(ti);
	fThemeList->DeselectAll();
	fThemeList->Select(fThemeList->CountItems() - 1);
	fThemeList->ScrollToSelection();
	return B_OK;
}


status_t
ThemeInterfaceView::SaveSelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BMessage theme;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return B_OK;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return AError(__FUNCTION__, ENOENT);
	if (item->IsReadOnly())
		return AError(__FUNCTION__, B_READ_ONLY_DEVICE);
	id = item->ThemeId();
	if (id < 0)
		return AError(__FUNCTION__, ENOENT);

	err = tman->UpdateTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	err = tman->SaveTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	//err = tman->ApplyTheme(theme);
	return err;
}


status_t
ThemeInterfaceView::DeleteSelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BMessage theme;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return B_OK;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return AError(__FUNCTION__, ENOENT);
	if (item->IsReadOnly())
		return AError(__FUNCTION__, B_READ_ONLY_DEVICE);
	id = item->ThemeId();
	if (id < 0)
		return AError(__FUNCTION__, ENOENT);
	// then apply
	err = tman->DeleteTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	fThemeList->RemoveItem(item);
	delete item;
	//err = tman->ApplyTheme(theme);
	return err;
}


status_t
ThemeInterfaceView::AddScreenshot()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BMessage theme;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return B_OK;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return AError(__FUNCTION__, ENOENT);
	id = item->ThemeId();
	if (id < 0)
		return AError(__FUNCTION__, ENOENT);
	// then apply
	err = tman->MakeThemeScreenShot(id);
	if (err)
		return AError(__FUNCTION__, err);
	err = tman->SaveTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	ThemeSelected(); // force reload of description for selected theme.
	return err;
}


status_t
ThemeInterfaceView::ThemeSelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BString desc;
	BBitmap *sshot = NULL;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
	{
		fScreenshotPane->ClearViewBitmap();
		fScreenshotPane->Invalidate(fScreenshotPane->Bounds());
		
		while(true == fScreenshotText->IsHidden())
			fScreenshotText->Show();
			
		fScreenshotText->SetText(B_TRANSLATE("No theme selected"));
		return ENOENT;
	}
	
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return ENOENT;
	id = item->ThemeId();
	if (id < 0)
		return ENOENT;
	// then apply
	
	err = tman->ThemeScreenShot(id, &sshot);
	if (err)
		sshot = NULL;
	if (sshot == NULL) 
	{
		SetScreenshot(NULL);
		fprintf(stderr, "ThemeManager: no screenshot; error 0x%08lx\n", err);

		while(true == fScreenshotText->IsHidden())
			fScreenshotText->Show();

		fScreenshotText->SetText(B_TRANSLATE("No Screenshot"));
		return err;
	}

	SetScreenshot(sshot);
	while(false == fScreenshotText->IsHidden())
			fScreenshotText->Hide();

	//err = tman->ApplyTheme(theme);
	return err;
}


void
ThemeInterfaceView::SetIsRevertable()
{
	BMessenger msgr(Parent());
	msgr.SendMessage(B_PREF_APP_ENABLE_REVERT);
}


void
ThemeInterfaceView::SetScreenshot(BBitmap *shot)
{
	// no screenshotpanel
	if(NULL == fScreenshotPane)
		return;

	fScreenshotPane->SetScreenshot(shot);
	fHasScreenshot = (shot != NULL);
}


status_t
ThemeInterfaceView::AError(const char *func, status_t err)
{
	BAlert *alert;
	BString msg;
	char *str = strerror(err);
	msg << "Error in " << func << "() : " << str;
	alert = new BAlert("error", msg.String(), B_TRANSLATE("Ok"));
	alert->Go();
	return err; /* pass thru */
}

