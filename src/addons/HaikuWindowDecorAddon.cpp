/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * window_decor ThemesAddon class
 */

#include <BeBuild.h>
#ifdef B_HAIKU_VERSION_1

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <MediaFiles.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_window_decor
#endif

#define DERR(e) { PRINT(("%s: err: %s\n", __FUNCTION__, strerror(e))); }

// headers/private/interface/DecoratorPrivate.h
namespace BPrivate {
bool get_decorator(BString &name);
status_t set_decorator(const BString &name);
status_t get_decorator_preview(const BString &name, BBitmap *bitmap);
} 

using namespace BPrivate;


//XXX: maybe check before setting the current decorator again ?
#if 0
status_t
set_decorator(const char *name)
{
	BString n;
	status_t err;
	int i = count_decorators() - 1;
	for (; i > -1; i--) {
		err = get_decorator_name(i, n);
		DERR(err);
		if (err < B_OK)
			continue;
		if (n == name) {
			err = set_decorator(i);
			DERR(err);
			return err;
		}
	}
	return ENOENT;
}
#endif

#define A_NAME "Window decor"
#define A_MSGNAME Z_THEME_WINDOW_DECORATIONS
#define A_DESCRIPTION "Window decorations and scrollbars"


class DecorThemesAddon : public ThemesAddon {
public:
	DecorThemesAddon();
	~DecorThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


DecorThemesAddon::DecorThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


DecorThemesAddon::~DecorThemesAddon()
{
}


const char *
DecorThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
DecorThemesAddon::RunPreferencesPanel()
{
	status_t err;

	// Haiku (tab colors)
	if (be_roster->Launch("application/x-vnd.Haiku-Appearance") == B_OK)
		return B_OK;

	// ZETA
	entry_ref ref;
	BEntry ent;
	err = ent.SetTo("/boot/beos/references/Appearance");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	return err;
}


status_t
DecorThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_WINDOW_DECORATIONS, "Window decorations and scrollbars");
	names.AddString("window:decor", "Window decor");
	names.AddString("window:decor_globals", "Window decor parameters");
	return B_OK;
}


status_t
DecorThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage window_decor;
	BMessage ui_settings;
	BMessage globals;
	BString decorName;
	int32 decorId;
	bool decorDone = false;
	status_t err;
	rgb_color col;

	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, window_decor);
	DERR(err);
	if (err)
		return err;

	theme.FindMessage(Z_THEME_UI_SETTINGS, &ui_settings);

	if (window_decor.FindMessage("window:decor_globals", &globals) == B_OK) {
		BMessage bwindow;
		// Try to map colors from the Dano fields
		if (globals.FindMessage("BWindow", &bwindow) == B_OK) {
			if (FindRGBColor(bwindow, "f:Inactive Title", 0, &col) == B_OK)
				set_ui_color(B_WINDOW_INACTIVE_TEXT_COLOR, col);
			if (FindRGBColor(bwindow, "f:Active Title", 0, &col) == B_OK)
				set_ui_color(B_WINDOW_TEXT_COLOR, col);
		}

	}
	// get colors here, just in case the UI Settings addon isn't enabled.
	// apply them before changing the decor since for now they don't handle color changes.
	if (FindRGBColor(window_decor, B_UI_WINDOW_TAB_COLOR, 0, &col) == B_OK ||
		FindRGBColor(ui_settings, B_UI_WINDOW_TAB_COLOR, 0, &col) == B_OK)
		set_ui_color(B_WINDOW_TAB_COLOR, col);

	if (FindRGBColor(window_decor, B_UI_WINDOW_TEXT_COLOR, 0, &col) == B_OK ||
		FindRGBColor(ui_settings, B_UI_WINDOW_TEXT_COLOR, 0, &col) == B_OK)
		set_ui_color(B_WINDOW_TEXT_COLOR, col);

	if (FindRGBColor(window_decor, B_UI_WINDOW_INACTIVE_TAB_COLOR, 0, &col) == B_OK ||
		FindRGBColor(ui_settings, B_UI_WINDOW_INACTIVE_TAB_COLOR, 0, &col) == B_OK)
		set_ui_color(B_WINDOW_INACTIVE_TAB_COLOR, col);

	if (FindRGBColor(window_decor, B_UI_WINDOW_INACTIVE_TEXT_COLOR, 0, &col) == B_OK ||
		FindRGBColor(ui_settings, B_UI_WINDOW_INACTIVE_TEXT_COLOR, 0, &col) == B_OK)
		set_ui_color(B_WINDOW_INACTIVE_TEXT_COLOR, col);

	// try each name until one works
	for (int i = 0; window_decor.FindString("window:decor", i, &decorName) == B_OK; i++) {
		if (set_decorator(decorName) == B_OK) {
			decorDone = true;
			break;
		}
	}
	// none... maybe R5 number ?
	if (!decorDone && 
		window_decor.FindInt32("window:R5:decor", &decorId) == B_OK) {
		switch (decorId) {
			case R5_DECOR_BEOS:
			default:
				set_decorator(BString("Default"));
				break;
			case R5_DECOR_WIN95:
				set_decorator(BString("WinDecorator"));
				break;
			case R5_DECOR_AMIGA:
				set_decorator(BString("AmigaDecorator"));
				break;
			case R5_DECOR_MAC:
				set_decorator(BString("MacDecorator"));
				break;
		}
	}
#if 0
#ifdef B_BEOS_VERSION_DANO
	if (window_decor.FindString("window:decor", &decorName) == B_OK)
		set_window_decor(decorName.String(), 
			(window_decor.FindMessage("window:decor_globals", &globals) == B_OK)?&globals:NULL);
#else
extern void __set_window_decor(int32 theme);
	int32 decorNr = 0;
	if (window_decor.FindInt32("window:R5:decor", &decorNr) == B_OK)
		__set_window_decor(decorNr);
#endif
#endif

	return B_OK;
}


status_t
DecorThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage window_decor;
	BMessage globals;
	BMessage bwindow;
	BString decorName;
	status_t err;
	
	(void)flags;
	err = MyMessage(theme, window_decor);
	DERR(err);
	if (err)
		window_decor.MakeEmpty();

	err = get_decorator(decorName) ? B_OK : B_ERROR;
	DERR(err);
	if (err == B_OK) {
		AddRGBColor(bwindow, "f:Inactive Title", ui_color(B_WINDOW_INACTIVE_TEXT_COLOR));
		AddRGBColor(bwindow, "f:Active Title", ui_color(B_WINDOW_INACTIVE_TEXT_COLOR));

		globals.AddMessage("window:decor_globals", &bwindow);
		window_decor.AddString("window:decor", decorName.String());
		window_decor.AddMessage("window:decor_globals", &globals);

		AddRGBColor(bwindow, B_UI_WINDOW_TAB_COLOR, ui_color(B_WINDOW_TAB_COLOR));
		AddRGBColor(bwindow, B_UI_WINDOW_TEXT_COLOR, ui_color(B_WINDOW_TEXT_COLOR));
		AddRGBColor(bwindow, B_UI_WINDOW_INACTIVE_TAB_COLOR, ui_color(B_WINDOW_INACTIVE_TAB_COLOR));
		AddRGBColor(bwindow, B_UI_WINDOW_INACTIVE_TEXT_COLOR, ui_color(B_WINDOW_INACTIVE_TEXT_COLOR));
	}
#if 0
#ifdef B_BEOS_VERSION_DANO
	err = get_window_decor(&decorName, &globals);
	DERR(err);
	if (err == B_OK) {
		window_decor.AddString("window:decor", decorName.String());
		window_decor.AddMessage("window:decor_globals", &globals);
	}
#else
	window_decor.AddInt32("window:R5:decor", 0);
#endif
#endif
	
	err = SetMyMessage(theme, window_decor);
	DERR(err);
	return err;
}


status_t
DecorThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage window_decor;
	window_decor.AddString("window:decor", "Default");
	window_decor.AddInt32("window:R5:decor", 0L);
	theme.AddMessage(A_MSGNAME, &window_decor);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new DecorThemesAddon;
}


#endif /* B_BEOS_VERSION_DANO */
