/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ThemeAddonItem.h"
#include "ThemeInterfaceView.h"
#include "ThemeManager.h"
#include "UITheme.h"
#include <BeBuild.h>

#ifdef __HAIKU__
#include <locale/Catalog.h>
#endif

#ifdef B_ZETA_VERSION
#include <locale/Locale.h>
#define B_TRANSLATE _T
#else
#define B_LANGUAGE_CHANGED '_BLC'
#define B_PREF_APP_SET_DEFAULTS 'zPAD'
#define B_PREF_APP_REVERT 'zPAR'
#define B_PREF_APP_ADDON_REF 'zPAA'
#endif

#ifndef B_TRANSLATE
#define B_TRANSLATE(v) v
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ThemeAddonItem"

#include <Button.h>
#include <CheckBox.h>
#include <Font.h>
#include <Roster.h>
#include <View.h>
#include <stdio.h>

#define CTRL_OFF_X 40
#define CTRL_OFF_Y 25
#define CTRL_SPACING 10

#define HAVE_PREF_BTN


ThemeAddonItem::ThemeAddonItem(BRect bounds, ThemeInterfaceView *iview, int32 id)
	: ViewItem(bounds, "addonitem", B_FOLLOW_NONE, B_WILL_DRAW),
	fPrefsBtn(NULL)
{
	ThemeManager *tman;
	fId = id;
	fIView = iview;
	fAddonName = B_TRANSLATE("Global");
	BString tip(B_TRANSLATE("Set apply/save actions for all items"));
	tman = iview->GetThemeManager();
	uint32 flags = 0;
	bool detected = true;
	if (id > -1) {
		fAddonName = tman->AddonName(id);
		tip.SetTo(tman->AddonDescription(id));
		detected = tman->DetectApplication(id) == B_OK;
		if (!detected) {
			tip << " " << B_TRANSLATE("(Application not installed)");
			SetEnabled(false);
		}
		flags = tman->AddonFlags(id);
	}
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	SetToolTip(tip.String());
#endif
	SetViewPanelBgColor();
#ifdef B_BEOS_VERSION_DANO
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
#else
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
#endif
	BMessage *apply = new BMessage(CB_APPLY);
	apply->AddInt32("addon", id);
	BMessage *save = new BMessage(CB_SAVE);
	save->AddInt32("addon", id);
	int32 value;
	value = (flags & Z_THEME_ADDON_DO_SET_ALL) ? B_CONTROL_ON : B_CONTROL_OFF;
#ifdef __HAIKU__
	if (id < 0) value = B_CONTROL_PARTIALLY_ON;
#endif
	fApplyBox = new BCheckBox(BRect(CTRL_OFF_X, CTRL_OFF_Y, CTRL_OFF_X+50, CTRL_OFF_Y+30), "apply", B_TRANSLATE("Apply"), apply);
	fApplyBox->SetValue(value);
	fSaveBox = new BCheckBox(BRect(CTRL_OFF_X+50+CTRL_SPACING, CTRL_OFF_Y, CTRL_OFF_X+100+CTRL_SPACING, CTRL_OFF_Y+30), "save", B_TRANSLATE("Save"), save);
	value = (flags & Z_THEME_ADDON_DO_RETRIEVE) ? B_CONTROL_ON : B_CONTROL_OFF;
#ifdef __HAIKU__
	if (id < 0) value = B_CONTROL_PARTIALLY_ON;
#endif
	fSaveBox->SetValue(value);
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	fApplyBox->SetToolTip(B_TRANSLATE("Apply these settings from themes"));
	fSaveBox->SetToolTip(B_TRANSLATE("Save these settings to themes"));
#endif
	if (!detected) {
		fApplyBox->SetEnabled(false);
		fSaveBox->SetEnabled(false);
	}
#ifdef HAVE_PREF_BTN
	if (id > -1) {
		BMessage *prefs = new BMessage(BTN_PREFS);
		prefs->AddInt32("addon", id);
		fPrefsBtn = new BButton(BRect(CTRL_OFF_X+100+CTRL_SPACING*2, CTRL_OFF_Y, CTRL_OFF_X+150+CTRL_SPACING*2, CTRL_OFF_Y+30), "prefs", B_TRANSLATE("Preferences"), prefs);
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
		fPrefsBtn->SetToolTip(B_TRANSLATE("Open the preference panel for this item"));
#endif
		if (!detected)
			fPrefsBtn->SetEnabled(false);
	}
#endif
	BFont fnt;
	GetFont(&fnt);
	fnt.SetSize(fnt.Size()+1);
	fnt.SetFace(B_ITALIC_FACE);
	SetFont(&fnt);

	AddChild(fApplyBox);
	AddChild(fSaveBox);
#ifdef HAVE_PREF_BTN
	if (fPrefsBtn) {
		AddChild(fPrefsBtn);
	}
#endif
}


ThemeAddonItem::~ThemeAddonItem()
{
	delete fApplyBox;
	delete fSaveBox;
#ifdef HAVE_PREF_BTN
	delete fPrefsBtn;
#endif
}


void
ThemeAddonItem::DrawItem(BView *ownerview, BRect frame, bool complete)
{
	ViewItem::DrawItem(ownerview, frame, complete);
}


void
ThemeAddonItem::AttachedToWindow()
{
	fApplyBox->SetTarget(fIView);
	fSaveBox->SetTarget(fIView);
#ifdef HAVE_PREF_BTN
	if (fPrefsBtn) {
		fPrefsBtn->SetTarget(fIView);
	}
#endif
	RelayoutButtons();
}


void
ThemeAddonItem::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case B_LANGUAGE_CHANGED:
		break;
	}
	BView::MessageReceived(message);
}


void
ThemeAddonItem::Draw(BRect)
{
	DrawString(fAddonName.String(), BPoint(10, 15/*Bounds().Height()/2*/));
}


void
ThemeAddonItem::RelocalizeStrings()
{
	fApplyBox->SetLabel(B_TRANSLATE("Apply"));
	fSaveBox->SetLabel(B_TRANSLATE("Save"));
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	fApplyBox->SetToolTip(B_TRANSLATE("Apply these settings from themes"));
	fSaveBox->SetToolTip(B_TRANSLATE("Save these settings to themes"));
#endif
#ifdef HAVE_PREF_BTN
	if (fPrefsBtn) {
		fPrefsBtn->SetLabel(B_TRANSLATE("Preferences"));
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
		fPrefsBtn->SetToolTip(B_TRANSLATE("Open the preference panel for this item"));
#endif
	}
#endif
	RelayoutButtons();
}


void
ThemeAddonItem::RelayoutButtons()
{
	fApplyBox->ResizeToPreferred();
	fSaveBox->MoveTo(fApplyBox->Frame().right+CTRL_SPACING, fApplyBox->Frame().top);
	fSaveBox->ResizeToPreferred();
#ifdef HAVE_PREF_BTN
	if (fPrefsBtn) {
		fPrefsBtn->MoveTo(fSaveBox->Frame().right+CTRL_SPACING, fSaveBox->Frame().top);
		fPrefsBtn->ResizeToPreferred();
	}
#endif
}


int32
ThemeAddonItem::AddonId()
{
	return fId;
}

