/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ThemeAddonItem.h"
#include "ThemeInterfaceView.h"
#include "ThemeManager.h"
#include "UITheme.h"
#include <BeBuild.h>
#ifdef B_ZETA_VERSION
#include <locale/Locale.h>
#else
#define _T(v) v
#define B_LANGUAGE_CHANGED '_BLC'
#define B_PREF_APP_SET_DEFAULTS 'zPAD'
#define B_PREF_APP_REVERT 'zPAR'
#define B_PREF_APP_ADDON_REF 'zPAA'
#endif
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
	fAddonName = _T("Global");
	BString tip(_T("Change actions for all"));
	tman = iview->GetThemeManager();
	uint32 flags = 0;
	bool detected = true;
	if (id > -1) {
		fAddonName = tman->AddonName(id);
		tip.SetTo(tman->AddonDescription(id));
		detected = tman->DetectApplication(id) == B_OK;
		if (!detected) {
			tip << " " << _T("(Application not installed)");
			SetEnabled(false);
		}
		flags = tman->AddonFlags(id);
	}
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	SetToolTip(tip.String());
#endif
#ifdef B_BEOS_VERSION_DANO
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
#else
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
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
	fApplyBox = new BCheckBox(BRect(CTRL_OFF_X, CTRL_OFF_Y, CTRL_OFF_X+50, CTRL_OFF_Y+30), "apply", _T("Apply"), apply);
	fApplyBox->SetValue(value);
	fSaveBox = new BCheckBox(BRect(CTRL_OFF_X+50+CTRL_SPACING, CTRL_OFF_Y, CTRL_OFF_X+100+CTRL_SPACING, CTRL_OFF_Y+30), "save", _T("Save"), save);
	value = (flags & Z_THEME_ADDON_DO_RETRIEVE) ? B_CONTROL_ON : B_CONTROL_OFF;
#ifdef __HAIKU__
	if (id < 0) value = B_CONTROL_PARTIALLY_ON;
#endif
	fSaveBox->SetValue(value);
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	fApplyBox->SetToolTip(_T("Use this information from themes"));
	fSaveBox->SetToolTip(_T("Save this information to themes"));
#endif
	if (!detected) {
		fApplyBox->SetEnabled(false);
		fSaveBox->SetEnabled(false);
	}
#ifdef HAVE_PREF_BTN
	if (id > -1) {
		BMessage *prefs = new BMessage(BTN_PREFS);
		prefs->AddInt32("addon", id);
		fPrefsBtn = new BButton(BRect(CTRL_OFF_X+100+CTRL_SPACING*2, CTRL_OFF_Y, CTRL_OFF_X+150+CTRL_SPACING*2, CTRL_OFF_Y+30), "prefs", _T("Preferences"), prefs);
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
		fPrefsBtn->SetToolTip(_T("Run the preferences panel for this topic."));
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
	AddChild(fApplyBox);
	fApplyBox->SetTarget(fIView);
	AddChild(fSaveBox);
	fSaveBox->SetTarget(fIView);
#ifdef HAVE_PREF_BTN
	if (fPrefsBtn) {
		AddChild(fPrefsBtn);
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
	fApplyBox->SetLabel(_T("Apply"));
	fSaveBox->SetLabel(_T("Save"));
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	fApplyBox->SetToolTip(_T("Use this information from themes"));
	fSaveBox->SetToolTip(_T("Save this information to themes"));
#endif
#ifdef HAVE_PREF_BTN
	if (fPrefsBtn) {
		fPrefsBtn->SetLabel(_T("Preferences"));
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
		fPrefsBtn->SetToolTip(_T("Run the preferences panel for this topic."));
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

