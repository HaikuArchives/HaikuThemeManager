/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * ui_settings ThemesAddon class
 */

#include <BeBuild.h>
#ifdef B_HAIKU_VERSION_1

#include <Alert.h>
#include <Application.h>
#include <InterfaceDefs.h>
#include <Entry.h>
#include <Font.h>
#include <Menu.h>
#include <Message.h>
#include <Roster.h>
#include <String.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_ui_settings
#endif

#define DEBUG_TA
#ifdef DEBUG_TA
#define FENTRY PRINT(("*ThemesAddon[%s]::%s()\n", Name(), __FUNCTION__))
#else
#define FENTRY
#endif

#define DERR(e) { PRINT(("%s: err: %s\n", __FUNCTION__, strerror(e))); }

// headers/private/interface/DecoratorPrivate.h
namespace BPrivate {
bool get_decorator(BString &name);
status_t set_decorator(const BString &name);
status_t get_decorator_preview(const BString &name, BBitmap *bitmap);
}
using namespace BPrivate;

// private font API
extern void _set_system_font_(const char *which, font_family family,
	font_style style, float size);
extern status_t _get_system_default_font_(const char* which,
	font_family family, font_style style, float* _size);

// default values for colors in Interface Kit
// TODO: use it
namespace BPrivate {
extern const rgb_color* kDefaultColors;
}

#define A_NAME "System Colors and Fonts"
#define A_MSGNAME Z_THEME_UI_SETTINGS
#define A_DESCRIPTION "System colors, fonts and other goodies"


class UISettingsThemesAddon : public ThemesAddon {
public:
	UISettingsThemesAddon();
	~UISettingsThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


struct ui_color_map {
	const char *name;
	color_which id;
} gUIColorMap[] = {
	{ B_UI_PANEL_BACKGROUND_COLOR, B_PANEL_BACKGROUND_COLOR },
	{ B_UI_PANEL_TEXT_COLOR, B_PANEL_TEXT_COLOR },
//	{ B_UI_PANEL_LINK_COLOR, B_PANEL_LINK_COLOR },
	{ B_UI_DOCUMENT_BACKGROUND_COLOR, B_DOCUMENT_BACKGROUND_COLOR },
	{ B_UI_DOCUMENT_TEXT_COLOR, B_DOCUMENT_TEXT_COLOR },
//	{ B_UI_DOCUMENT_LINK_COLOR, B_DOCUMENT_LINK_COLOR },
	{ B_UI_CONTROL_BACKGROUND_COLOR, B_CONTROL_BACKGROUND_COLOR },
	{ B_UI_CONTROL_TEXT_COLOR, B_CONTROL_TEXT_COLOR },
	{ B_UI_CONTROL_BORDER_COLOR, B_CONTROL_BORDER_COLOR },
	{ B_UI_CONTROL_HIGHLIGHT_COLOR, B_CONTROL_HIGHLIGHT_COLOR },
	{ B_UI_CONTROL_MARK_COLOR, B_CONTROL_MARK_COLOR },
	{ B_UI_NAVIGATION_BASE_COLOR, B_NAVIGATION_BASE_COLOR },
	{ B_UI_NAVIGATION_PULSE_COLOR, B_NAVIGATION_PULSE_COLOR },
	{ B_UI_SHINE_COLOR, B_SHINE_COLOR },
	{ B_UI_SHADOW_COLOR, B_SHADOW_COLOR },
	{ B_UI_TOOLTIP_BACKGROUND_COLOR, B_TOOL_TIP_BACKGROUND_COLOR },
	{ B_UI_TOOLTIP_TEXT_COLOR, B_TOOL_TIP_TEXT_COLOR },
	{ B_UI_MENU_BACKGROUND_COLOR, B_MENU_BACKGROUND_COLOR },
	{ B_UI_MENU_SELECTED_BACKGROUND_COLOR, B_MENU_SELECTED_BACKGROUND_COLOR },
	{ B_UI_MENU_ITEM_TEXT_COLOR, B_MENU_ITEM_TEXT_COLOR },
	{ B_UI_MENU_SELECTED_ITEM_TEXT_COLOR, B_MENU_SELECTED_ITEM_TEXT_COLOR },
	{ B_UI_MENU_SELECTED_BORDER_COLOR, B_MENU_SELECTED_BORDER_COLOR },
	{ B_UI_SUCCESS_COLOR, B_SUCCESS_COLOR },
	{ B_UI_FAILURE_COLOR, B_FAILURE_COLOR },
	{ B_UI_WINDOW_TAB_COLOR, B_WINDOW_TAB_COLOR },
	{ B_UI_WINDOW_TEXT_COLOR, B_WINDOW_TEXT_COLOR },
	{ B_UI_WINDOW_INACTIVE_TAB_COLOR, B_WINDOW_INACTIVE_TAB_COLOR },
	{ B_UI_WINDOW_INACTIVE_TEXT_COLOR, B_WINDOW_INACTIVE_TEXT_COLOR },
	{ B_UI_WINDOW_BORDER_COLOR, B_WINDOW_BORDER_COLOR },
	{ B_UI_WINDOW_INACTIVE_BORDER_COLOR, B_WINDOW_INACTIVE_BORDER_COLOR },
	{ B_UI_LIST_BACKGROUND_COLOR, B_LIST_BACKGROUND_COLOR },
	{ B_UI_LIST_SELECTED_BACKGROUND_COLOR, B_LIST_SELECTED_BACKGROUND_COLOR },
	{ B_UI_LIST_ITEM_TEXT_COLOR, B_LIST_ITEM_TEXT_COLOR },
	{ B_UI_LIST_SELECTED_ITEM_TEXT_COLOR, B_LIST_SELECTED_ITEM_TEXT_COLOR },
	{ B_UI_SCROLL_BAR_THUMB_COLOR, B_SCROLL_BAR_THUMB_COLOR },
	
	{ NULL, (color_which)-1 }
};


UISettingsThemesAddon::UISettingsThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
	FENTRY;
}


UISettingsThemesAddon::~UISettingsThemesAddon()
{
	FENTRY;
}


const char *
UISettingsThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
UISettingsThemesAddon::RunPreferencesPanel()
{
	status_t err = B_OK;

	if (be_roster->Launch("application/x-vnd.Haiku-Appearance") == B_OK)
		return B_OK;

	entry_ref ref;
	BEntry ent;
	FENTRY;
	/*
	err = ent.SetTo("/boot/beos/preferences/Colors");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	*/
	if (!err)
		return B_OK;
	err = ent.SetTo("/boot/system/preferences/Appearance");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	return err;
}


status_t
UISettingsThemesAddon::AddNames(BMessage &names)
{
	FENTRY;
	
	names.AddString(Z_THEME_UI_SETTINGS, "UI Settings");
	// Haiku doesn't know about them, we add them ourselves
	//XXX: use symbolic names.
	names.AddString("be:c:PanBg", "Panel Background");
	names.AddString("be:c:PanTx", "Panel Text");
	names.AddString("be:c:PanLn", "Panel Link");
	names.AddString("be:c:DocBg", "Document Background");
	names.AddString("be:c:DocTx", "Document Text");
	names.AddString("be:c:DocLn", "Document Link");
	names.AddString("be:c:CtlBg", "Control Background");
	names.AddString("be:c:CtlTx", "Control Text");
	names.AddString("be:c:CtlBr", "Control Border");
	names.AddString("be:c:CtlHg", "Control Highlight");
	names.AddString("be:c:NavBs", "Navigation Base");
	names.AddString("be:c:NavPl", "Navigation Pulse");
	names.AddString("be:c:Shine", "Shine");
	names.AddString("be:c:Shadow", "Shadow");
	names.AddString("be:c:TipBg", "ToolTip Background");
	names.AddString("be:c:TipTx", "ToolTip Text");
	names.AddString("be:c:MenBg", "Menu Background");
	names.AddString("be:c:MenSBg", "Menu Selected Background");
	names.AddString("be:c:MenTx", "Menu Item Text");
	names.AddString("be:c:MenSTx", "Menu Selected Item Text");
	names.AddString("be:c:MenSBr", "Menu Selected Border");
	names.AddString("be:c:Success", "Success");
	names.AddString("be:c:Failure", "Failure");
	names.AddString("be:f:MenTx", "Menu Item Text");
	names.AddString("be:MenSep", "Menu Separator");
	names.AddString("be:MenTrig", "Show Menu Triggers");
	names.AddString("be:MenZSnake", "Menu ZSnake");
	names.AddString("be:f:Tip", "ToolTip");
	names.AddString("be:f:be_plain_font", "System Plain");
	names.AddString("be:f:be_bold_font", "System Bold");
	names.AddString("be:f:be_fixed_font", "System Fixed");
	names.AddString("be:f:be_fixed_font", "System Fixed");
	names.AddString("h:c:WinTabBg", "Window Tab Background");
	names.AddString("h:c:WinTabTx", "Window Tab Text");
	names.AddString("h:c:InWinTabBg", "Inactive Window Tab Background");
	names.AddString("h:c:InWinTabTx", "Inactive Window Tab Text");
	names.AddString(B_UI_WINDOW_BORDER_COLOR, "Window Border");
	names.AddString(B_UI_WINDOW_INACTIVE_BORDER_COLOR, "Inactive Window Border");
	names.AddString(B_UI_DOCUMENT_SELECTION_BACKGROUND_COLOR, "Document Selection Background");
	names.AddString(B_UI_CONTROL_MARK_COLOR, "Control Mark");
	names.AddString(B_UI_LIST_BACKGROUND_COLOR, "List Background");
	names.AddString(B_UI_LIST_SELECTED_BACKGROUND_COLOR, "List Selected Background");
	names.AddString(B_UI_LIST_ITEM_TEXT_COLOR, "List Item Text");
	names.AddString(B_UI_LIST_SELECTED_ITEM_TEXT_COLOR, "List Selected Item Text");
	names.AddString(B_UI_SCROLL_BAR_THUMB_COLOR, "Scrollbar Thumb");
	return B_OK;
}


status_t
UISettingsThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BFont fnt;
	struct menu_info menuInfo;
	status_t err;
	status_t gmierr = ENOENT;
	int i;
	FENTRY;

	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, uisettings);
	DERR(err);
	if (err)
		return err;

	font_family family;
	font_style style;
	
	err = FindFont(uisettings, "be:f:be_plain_font", 0, &fnt);
	//uisettings.RemoveName("be:f:be_plain_font");
	DERR(err);
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		_set_system_font_("plain", family, style, fnt.Size());
	}

	err = FindFont(uisettings, "be:f:be_bold_font", 0, &fnt);
	DERR(err);
	//uisettings.RemoveName("be:f:be_bold_font");
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		_set_system_font_("bold", family, style, fnt.Size());
	}

	err = FindFont(uisettings, "be:f:be_fixed_font", 0, &fnt);
	DERR(err);
	//uisettings.RemoveName("be:f:be_fixed_font");
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		_set_system_font_("fixed", family, style, fnt.Size());
	}

	err = FindFont(uisettings, "be:f:Tip", 0, &fnt);
	DERR(err);
	//uisettings.RemoveName("be:f:Tip");
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		// not implemented yet in Haiku
		//_set_system_font_("tip", family, style, fnt.Size());
		//_set_system_font_("tooltip", family, style, fnt.Size());
	}

	gmierr = get_menu_info(&menuInfo);

	for (i = 0; gUIColorMap[i].name; i++) {
		rgb_color c;
		if (FindRGBColor(uisettings, gUIColorMap[i].name, 0, &c) == B_OK) {
			set_ui_color(gUIColorMap[i].id, c);
			PRINT(("set_ui_color(%d, #%02x%02x%02x)\n", 
				gUIColorMap[i].id, c.red, c.green, c.blue));
			if (gUIColorMap[i].id == B_MENU_BACKGROUND_COLOR)
				menuInfo.background_color = c;
		}
	}
	
	if (gmierr >= B_OK) {
		bool bval;
		int32 ival;
		if (uisettings.FindBool("be:MenTrig", &bval) >= B_OK)
			menuInfo.triggers_always_shown = bval;
		if (uisettings.FindInt32("be:MenSep", &ival) >= B_OK)
			menuInfo.separator = ival;
		err = FindFont(uisettings, "be:f:MenTx", 0, &fnt);
		DERR(err);
		if (err == B_OK) {
			fnt.GetFamilyAndStyle(&menuInfo.f_family, &menuInfo.f_style);
			menuInfo.font_size = fnt.Size();
		}
		set_menu_info(&menuInfo);
	}

	// force reloading the current decor to pick up the changed colors
	BString decor;
	if (get_decorator(decor))
		err = set_decorator(decor);

	uisettings.what = B_UI_SETTINGS_CHANGED;
	// doesn't work, we must enumerate all windows
	//be_roster->Broadcast(&uisettings);
	BList apps;
	be_roster->GetAppList(&apps);
	for (i = 0; i < apps.CountItems(); i++) {
		BMessenger app(NULL, (team_id)(addr_t)apps.ItemAt(i));
		if (!app.IsValid())
			continue;
		
		BMessage answer;
		BMessage msgGetMsgr(B_GET_PROPERTY);
		msgGetMsgr.AddSpecifier("Windows");
		err = app.SendMessage(&msgGetMsgr, &answer, 2000000LL, 2000000LL);
		if (err < B_OK)
			continue;

		BMessenger win;
		int j;
		for (j = 0; answer.FindMessenger("result", j, &win) == B_OK; j++) {
			err = win.SendMessage(&uisettings, (BHandler *)NULL, 20000LL);
			//printf("post to %d (%d) 0x%08lx\n", (team_id)apps.ItemAt(i), j, err);
			// we don't care if it worked
		}
	}

	return B_OK;
}


status_t
UISettingsThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BMessage names;
	BFont fnt;
	menu_info menuInfo;
	status_t err;
	int i;
	FENTRY;
	
	(void)flags;
	err = MyMessage(theme, uisettings);
	if (err)
		uisettings.MakeEmpty();
	
	for (i = 0; gUIColorMap[i].name; i++) {
		rgb_color c = ui_color(gUIColorMap[i].id);
		AddRGBColor(uisettings, gUIColorMap[i].name, c);
	}
	
	// hack for legacy fonts
	AddFont(uisettings, "be:f:be_plain_font", (BFont *)be_plain_font);
	AddFont(uisettings, "be:f:be_bold_font", (BFont *)be_bold_font);
	AddFont(uisettings, "be:f:be_fixed_font", (BFont *)be_fixed_font);
	// XXX: FIXME
	//AddFont(uisettings, "be:f:Tip", (BFont *)be_tip_font);

	// menu stuff
	err = get_menu_info(&menuInfo);
	if (err >= B_OK) {
		uisettings.AddBool("be:MenTrig", menuInfo.triggers_always_shown);
		uisettings.AddInt32("be:MenSep", menuInfo.separator);
		fnt.SetFamilyAndStyle(menuInfo.f_family, menuInfo.f_style);
		fnt.SetSize(menuInfo.font_size);
		AddFont(uisettings, "be:f:MenTx", &fnt);
	}

	err = SetMyMessage(theme, uisettings);
	return err;
}


status_t
UISettingsThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	rgb_color color;
	FENTRY;
	
	/*
	status_t err;
	err = get_default_settings(&uisettings);
	if (err)
		return err;
	*/
	
	font_family family;
	font_style style;
	float size;
	BFont f;
	if (_get_system_default_font_("plain", family, style, &size) != B_OK)
		return B_ERROR;
	f.SetFamilyAndStyle(family, style);
	f.SetSize(size);
	AddFont(uisettings, "be:f:be_plain_font", &f);
	
	if (_get_system_default_font_(Name(), family, style, &size) != B_OK)
		return B_ERROR;
	f.SetFamilyAndStyle(family, style);
	f.SetSize(size);
	AddFont(uisettings, "be:f:be_plain_font", &f);
	
	if (_get_system_default_font_(Name(), family, style, &size) != B_OK)
		return B_ERROR;
	f.SetFamilyAndStyle(family, style);
	f.SetSize(size);
	AddFont(uisettings, "be:f:be_plain_font", &f);
	
	// menu stuff
	sprintf(family,"%s","Bitstream Vera Sans");
	sprintf(style,"%s","Roman");
	f.SetFamilyAndStyle(family, style);
	f.SetSize(12.0);
	AddFont(uisettings, "be:f:MenTx", &f);
	color.red = 216;
	color.blue = 216;
	color.green = 216;
	color.alpha = 255;
	AddRGBColor(uisettings, "be:c:MenBg", color);
	uisettings.AddBool("be:MenTrig", false);
	uisettings.AddInt32("be:MenSep", 0);
	
	theme.AddMessage(A_MSGNAME, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new UISettingsThemesAddon;
}


#endif /* B_BEOS_VERSION_DANO */
