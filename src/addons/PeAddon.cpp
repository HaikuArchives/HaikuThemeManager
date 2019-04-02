/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * Pe Color ThemesAddon class
 */

#include <Control.h>
#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Font.h>
#include <List.h>
#include <String.h>
#include <Roster.h>
#include <storage/Path.h>
#include <storage/File.h>
#include <storage/NodeInfo.h>
#include <storage/FindDirectory.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_pe
#endif

#define A_NAME "Pe colors"
#define A_MSGNAME NULL //Z_THEME_PE_SETTINGS
#define A_DESCRIPTION "Make Pe use system colors"

#define PE_SETTINGS_NAME "pe/settings"
static const char *kAppSig = "application/x-vnd.beunited.pe";
static const char *kAppSigs[] = {
	kAppSig,
	NULL
};

#define msg_Preferences					'Pref'
#define msg_NewColor					'NClr'

static struct pe_color_map {
	const char *pe;
	const char *view;
	const char *dano;
	const char *fallback;
} sPeColors[] = {
	{ "low", "lowc", B_UI_DOCUMENT_BACKGROUND_COLOR, NULL },
	{ "text", "txtc", B_UI_DOCUMENT_TEXT_COLOR, NULL },
	{ "selection", "selc", B_UI_DOCUMENT_SELECTION_BACKGROUND_COLOR, B_UI_MENU_SELECTED_BACKGROUND_COLOR },
	{ NULL, NULL, NULL, NULL }
};

class PeThemesAddon : public ThemesAddon {
public:
	PeThemesAddon();
	~PeThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);

private:
status_t	FindPrefWindow(BMessenger &messenger);
};


PeThemesAddon::PeThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME, kAppSigs)
{
}


PeThemesAddon::~PeThemesAddon()
{
}


const char *
PeThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
PeThemesAddon::RunPreferencesPanel()
{
	status_t err;

	// make sure Terminal is running
	if (!be_roster->IsRunning(kAppSig)) {
		err = be_roster->Launch(kAppSig);
		if (err < B_OK)
			return err;
	}

	// force showing the prefs window
	BMessenger app(kAppSig);
	BMessage msgShowPref(msg_Preferences);
	err = app.SendMessage(&msgShowPref);

	return B_OK;
}


status_t
PeThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_PE_SETTINGS, "Pe Settings");
	return B_OK;
}


status_t
PeThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BMessage peColors;
	status_t err;
	BPath PeSPath;
	rgb_color col;
	BString text;
	char buffer[10];
	int i;
	
	err = theme.FindMessage(Z_THEME_UI_SETTINGS, &uisettings);
	if (err)
		return err;
	
	for (i = 0; sPeColors[i].pe; i++) {
		if (FindRGBColor(uisettings, sPeColors[i].dano, 0, &col) < B_OK)
			if (sPeColors[i].fallback &&
				FindRGBColor(uisettings, sPeColors[i].fallback, 0, &col) < B_OK)
				continue;
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << sPeColors[i].pe << " color=#" << buffer << "\n";
	}
	
	// apply
	BMessenger msgrPrefs;
	if (flags & UI_THEME_SETTINGS_APPLY &&
		AddonFlags() & Z_THEME_ADDON_DO_APPLY &&
		FindPrefWindow(msgrPrefs) == B_OK) {


		for (i = 0; sPeColors[i].pe; i++) {
			if (FindRGBColor(uisettings, sPeColors[i].dano, 0, &col) < B_OK)
				if (FindRGBColor(uisettings, sPeColors[i].fallback, 0, &col) < B_OK)
					continue;
			BMessage msgColor(msg_NewColor);
			msgColor.AddData("color", B_RGB_COLOR_TYPE, &col, sizeof(col));
			msgColor.AddSpecifier("View", sPeColors[i].view);
			msgrPrefs.SendMessage(&msgColor);
		}
		
		// simulate a click on the "Ok" button in prefs
		// this doesn't seem to work anymore in Haiku !?
		BMessage click(B_SET_PROPERTY);
		click.AddSpecifier("Value");
		click.AddSpecifier("View", "ok  ");
		click.AddInt32("data", B_CONTROL_ON);
		msgrPrefs.SendMessage(&click);

		// just in case
		click.RemoveName("data");
		click.AddInt32("data", B_CONTROL_OFF);
		msgrPrefs.SendMessage(&click);

		// instead simulate a button down/up
		snooze(100000);
		click.MakeEmpty();
		click.what = B_MOUSE_DOWN;
		click.AddSpecifier("View", "ok  ");
		click.AddSpecifier("View", (int32)0);
		msgrPrefs.SendMessage(&click);

		snooze(100000);
		click.what = B_MOUSE_UP;
		msgrPrefs.SendMessage(&click);
	}

	// save
	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &PeSPath) < B_OK)
			return B_ERROR;
		PeSPath.Append(PE_SETTINGS_NAME);
		BFile PeSettings(PeSPath.Path(), B_WRITE_ONLY|B_OPEN_AT_END);
		if (PeSettings.InitCheck() < B_OK)
			return PeSettings.InitCheck();
	
		if (PeSettings.Write(text.String(), strlen(text.String())) < B_OK)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
PeThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	(void)theme; (void)flags;
	return B_OK;
}


status_t
PeThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	rgb_color bg = {255, 255, 255, 255};
	rgb_color fg = {0, 0, 0, 255};
	rgb_color selbg = {180, 200, 240, 255};
	AddRGBColor(uisettings, B_UI_DOCUMENT_BACKGROUND_COLOR, bg);
	AddRGBColor(uisettings, B_UI_DOCUMENT_TEXT_COLOR, fg);
	AddRGBColor(uisettings, B_UI_DOCUMENT_SELECTION_BACKGROUND_COLOR, selbg);
	theme.AddMessage(Z_THEME_UI_SETTINGS, &uisettings);
	return ApplyTheme(theme, flags);
}


status_t
PeThemesAddon::FindPrefWindow(BMessenger &messenger)
{
	BMessenger app(kAppSig);
	BMessenger win;
	status_t err;
	int i;

	if (!app.IsValid())
		return B_ERROR;

	BMessage answer;
	BMessage msgGetMsgr(B_GET_PROPERTY);
	msgGetMsgr.AddSpecifier("Windows");
	err = app.SendMessage(&msgGetMsgr, &answer, 2000000LL, 2000000LL);
	if (B_OK == err) {
		for (i = 0; answer.FindMessenger("result", i, &win) == B_OK; i++) {
			BString title;
			//TODO: FIXME: title might get localized someday...
			BMessage m(B_GET_PROPERTY);
			m.AddSpecifier("Title");
			err = win.SendMessage(&m, &m, 20000LL, 20000LL);
			if (m.FindString("result", &title) == B_OK &&
				title == "Preferences") {
				// get the top level view
				BMessenger view;
				m = BMessage(B_GET_PROPERTY);
				m.AddSpecifier("View", (int32)0);
				err = win.SendMessage(&m, &m, 20000LL, 20000LL);
				if (m.FindMessenger("result", &view) == B_OK) {
					messenger = view;
					return B_OK;
				}
			}
		}
	}

	// force showing the prefs window
	BMessage msgShowPref(msg_Preferences);
	err = app.SendMessage(&msgShowPref);

	err = app.SendMessage(&msgGetMsgr, &answer, 2000000LL, 2000000LL);
	if (B_OK == err) {
		for (i = 0; answer.FindMessenger("result", i, &win) == B_OK; i++) {
			BString title;
			BMessage m(B_GET_PROPERTY);
			m.AddSpecifier("Title");
			err = win.SendMessage(&m, &m, 20000LL, 20000LL);
			if (m.FindString("result", &title) == B_OK &&
				title == "Preferences") {
				// hide the prefs window
				BMessage hide(B_SET_PROPERTY);
				hide.AddSpecifier("Hidden");
				hide.AddBool("data", true);
				err = win.SendMessage(&hide);
				// get the top level view
				BMessenger view;
				m = BMessage(B_GET_PROPERTY);
				m.AddSpecifier("View", (int32)0);
				err = win.SendMessage(&m, &m, 20000LL, 20000LL);
				if (m.FindMessenger("result", &view) == B_OK) {
					messenger = view;
					return B_OK;
				}
			}
		}
	}

	return B_ERROR;
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new PeThemesAddon;
}

