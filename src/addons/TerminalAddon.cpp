/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * Terminal Color ThemesAddon class
 */

#include <Alert.h>
#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Font.h>
#include <String.h>
#include <Roster.h>
#include <storage/Path.h>
#include <storage/File.h>
#include <storage/NodeInfo.h>
#include <storage/FindDirectory.h>

#define DEBUG 1
#include <Debug.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_terminal
#endif

#define A_NAME "Terminal"
#define A_MSGNAME Z_THEME_TERMINAL_SETTINGS
#define A_DESCRIPTION "Terminal color, font and size"

#define NENTS(x)      sizeof(x)/sizeof(x[0])

/* definitions for R5 Terminal settings file */

#define TP_MAGIC 0xf1f2f3f4
#define TP_VERSION 0x02
#define TP_FONT_NAME_SZ 128

// __PACKED

struct tpref {
	uint32 cols;
	uint32 rows;
	uint32 tab_width;
	uint32 font_size;
	char font[TP_FONT_NAME_SZ]; // "Family/Style"
	uint32 cursor_blink_rate; // blinktime in µs = 1000000
	uint32 refresh_rate; // ??? = 0
	rgb_color bg;
	rgb_color fg;
	rgb_color curbg;
	rgb_color curfg;
	rgb_color selbg;
	rgb_color selfg;
	char encoding; // index in the menu (0 = UTF-8)
	char unknown[3];
}; /* this is what is sent to the Terminal window... actually not ! It's sent by Terminal to itself. */

struct termprefs {
	uint32 magic;
	uint32 version;
	float x;
	float y;
	struct tpref p;
};

#define MSG_R5_SET_PREF 'pref'

/* definitions for Haiku Terminal settings file */
/* from TermConst.h */

static const char* const PREF_HALF_FONT_FAMILY = "Half Font Family";
static const char* const PREF_HALF_FONT_STYLE = "Half Font Style";
static const char* const PREF_HALF_FONT_SIZE = "Half Font Size";

static const char* const PREF_TEXT_FORE_COLOR = "Text";
static const char* const PREF_TEXT_BACK_COLOR = "Background";
static const char* const PREF_CURSOR_FORE_COLOR = "Text under cursor";
static const char* const PREF_CURSOR_BACK_COLOR = "Cursor";
static const char* const PREF_SELECT_FORE_COLOR = "Selected text";
static const char* const PREF_SELECT_BACK_COLOR = "Selected background";

static const char* const PREF_IM_FORE_COLOR = "IM foreground color";
static const char* const PREF_IM_BACK_COLOR = "IM background color";
static const char* const PREF_IM_SELECT_COLOR = "IM selection color";

static const char* const PREF_ANSI_BLACK_COLOR = "ANSI black color";
static const char* const PREF_ANSI_RED_COLOR = "ANSI red color";
static const char* const PREF_ANSI_GREEN_COLOR = "ANSI green color";
static const char* const PREF_ANSI_YELLOW_COLOR = "ANSI yellow color";
static const char* const PREF_ANSI_BLUE_COLOR = "ANSI blue color";
static const char* const PREF_ANSI_MAGENTA_COLOR = "ANSI magenta color";
static const char* const PREF_ANSI_CYAN_COLOR = "ANSI cyan color";
static const char* const PREF_ANSI_WHITE_COLOR = "ANSI white color";

static const char* const PREF_ANSI_BLACK_HCOLOR = "ANSI bright black color";
static const char* const PREF_ANSI_RED_HCOLOR = "ANSI bright red color";
static const char* const PREF_ANSI_GREEN_HCOLOR = "ANSI bright green color";
static const char* const PREF_ANSI_YELLOW_HCOLOR = "ANSI bright yellow color";
static const char* const PREF_ANSI_BLUE_HCOLOR = "ANSI bright blue color";
static const char* const PREF_ANSI_MAGENTA_HCOLOR = "ANSI bright magenta color";
static const char* const PREF_ANSI_CYAN_HCOLOR = "ANSI bright cyan color";
static const char* const PREF_ANSI_WHITE_HCOLOR = "ANSI bright white color";

static const char* const PREF_COLS = "Cols";
static const char* const PREF_ROWS = "Rows";

static const char* const PREF_BLINK_CURSOR = "Blinking cursor";
static const char* const PREF_CURSOR_STYLE = "Cursor style";
static const char* const PREF_EMULATE_BOLD = "Emulate bold";


/* theme field names */

#define TP_COLS "term:cols"
#define TP_ROWS "term:rows"
#define TP_TABWIDTH "term:tab"
#define TP_FONT "term:font"
#define TP_BG "term:c:bg"
#define TP_FG "term:c:fg"
#define TP_CURBG "term:c:curbg"
#define TP_CURFG "term:c:curfg"
#define TP_SELBG "term:c:selbg"
#define TP_SELFG "term:c:selfg"
#define TP_ENCODING "term:encoding"

static struct {
	const char *name; // if NULL, just prefix "term:"
	const char *pref;
	int32 def;
} sHaikuPrefsMapInt32[] = {
	{ TP_COLS, PREF_COLS, 80 },
	{ TP_ROWS, PREF_ROWS, 25 }
	// discard encoding and tab width
};

static struct {
	const char *name; // if NULL, just prefix "term:"
	const char *pref;
	rgb_color def;
} sHaikuPrefsMapColors[] = {
	{ TP_BG, PREF_TEXT_BACK_COLOR, {255, 255, 255, 0} },
	{ TP_FG, PREF_TEXT_FORE_COLOR, {0, 0, 0, 0} },
	{ TP_CURBG, PREF_CURSOR_BACK_COLOR, {0, 0, 0, 0} },
	{ TP_CURFG, PREF_CURSOR_FORE_COLOR, {255, 255, 255, 0} },
	{ TP_SELBG, PREF_SELECT_BACK_COLOR, {0, 0, 0, 0} },
	{ TP_SELFG, PREF_SELECT_FORE_COLOR, {255, 255, 255, 0} }
	//TODO: handle IM colors?
	//TODO: handle ANSI colors?
};

// TODO: split family/style/size
static struct {
	const char *name; // if NULL, just prefix "term:"
	const char *pref;
	const char *def;
} sHaikuPrefsMapFonts[] = {
	{ TP_FONT, NULL, NULL },
};

static const char *kHaikuTerminalAppSig = "application/x-vnd.Haiku-Terminal";
static const char *kBeOSTerminalAppSig = "application/x-vnd.Be-SHEL";


class TerminalThemesAddon : public ThemesAddon {
public:
	TerminalThemesAddon();
	~TerminalThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);

	/* Theme installation */
status_t	InstallFiles(BMessage &theme, BDirectory &folder);
status_t	BackupFiles(BMessage &theme, BDirectory &folder);

private:
status_t	ApplyThemeR5(BMessage &theme, uint32 flags);
status_t	MakeThemeR5(BMessage &theme, uint32 flags);
status_t	ApplyThemeHaiku(BMessage &theme, uint32 flags);
status_t	MakeThemeHaiku(BMessage &theme, uint32 flags);
status_t	LoadHaikuTerminalSettings(BMessage &into);
status_t	SaveHaikuTerminalSettings(BMessage &from);
};


TerminalThemesAddon::TerminalThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


TerminalThemesAddon::~TerminalThemesAddon()
{
}


const char *
TerminalThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
TerminalThemesAddon::RunPreferencesPanel()
{
	status_t err;

	// make sure Terminal is running
	if (!be_roster->IsRunning(kHaikuTerminalAppSig)) {
		err = be_roster->Launch(kHaikuTerminalAppSig);
		if (err < B_OK)
			return err;
	}

	// and fake the menu item click
	BMessage command('MPre');
	command.AddSpecifier("Window", (int32)0);

	BMessenger msgr(kHaikuTerminalAppSig);
	err = msgr.SendMessage(&command);

	return err;
}


status_t
TerminalThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_TERMINAL_SETTINGS, "Terminal Preferences");
	names.AddString(TP_COLS, "Terminal column count");
	names.AddString(TP_ROWS, "Terminal row count");
	names.AddString(TP_TABWIDTH, "Terminal tab width");
	names.AddString(TP_FONT, "Terminal font");
	names.AddString(TP_BG, "Terminal background color");
	names.AddString(TP_FG, "Terminal foreground color");
	names.AddString(TP_CURBG, "Terminal cursor background color");
	names.AddString(TP_CURFG, "Terminal cursor foreground color");
	names.AddString(TP_SELBG, "Terminal selection background color");
	names.AddString(TP_SELFG, "Terminal selection foreground color");
	names.AddString(TP_ENCODING, "Terminal text encoding");
	return B_OK;
}


status_t
TerminalThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	status_t err;

#ifndef __HAIKU__
	err = ApplyThemeR5(theme, flags);
#endif
	// Some people use the Haiku terminal in BeOS.
	err = ApplyThemeHaiku(theme, flags);
	if (err)
		return err;
	return B_OK;
}


status_t
TerminalThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
#ifdef __HAIKU__
	return MakeThemeHaiku(theme, flags);
#else
	return MakeThemeR5(theme, flags);
#endif
}


status_t
TerminalThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage termpref;
	int32 i;

	// XXX: add font and stuff...

	for (i = 0; i < NENTS(sHaikuPrefsMapInt32); i++) {
		termpref.AddInt32(sHaikuPrefsMapInt32[i].name,
			sHaikuPrefsMapInt32[i].def);
	}

	for (i = 0; i < NENTS(sHaikuPrefsMapColors); i++) {
		AddRGBColor(termpref, sHaikuPrefsMapColors[i].name,
			sHaikuPrefsMapColors[i].def);
	}

	theme.AddMessage(Z_THEME_TERMINAL_SETTINGS, &termpref);
	return ApplyTheme(theme, flags);
}


status_t
TerminalThemesAddon::InstallFiles(BMessage &theme, BDirectory &folder)
{
	BMessage termpref;
	status_t err;
	
	(void)folder;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	return B_OK;
}


status_t
TerminalThemesAddon::BackupFiles(BMessage &theme, BDirectory &folder)
{
	BMessage termpref;
	status_t err;
	
	(void)folder;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	entry_ref ref;
	//find_font_file(&ref, ff, fs);
	return B_OK;
}


status_t
TerminalThemesAddon::ApplyThemeR5(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	status_t err;
	struct termprefs tp;
	
	err = MyMessage(theme, termpref);
	if (err)
		return err;
	tp.magic = TP_MAGIC;
	tp.version = TP_VERSION;
	tp.x = 0; // don't open at specific coords
	tp.y = 0;
	if (termpref.FindInt32(TP_COLS, (int32 *)&tp.p.cols) != B_OK)
		tp.p.cols = 80;
	if (termpref.FindInt32(TP_ROWS, (int32 *)&tp.p.rows) != B_OK)
		tp.p.rows = 25;
	if (termpref.FindInt32(TP_TABWIDTH, (int32 *)&tp.p.tab_width) != B_OK)
		tp.p.tab_width = 8;
	BFont tFont;
	tp.p.font_size = 12;
	strcpy(tp.p.font, "Courier10 BT/Roman");
	if (FindFont(termpref, TP_FONT, 0, &tFont) == B_OK) {
		font_family ff;
		font_style fs;
		tFont.GetFamilyAndStyle(&ff, &fs);
		strcpy(tp.p.font, ff);
		strcat(tp.p.font, "/");
		strcat(tp.p.font, fs);
		tp.p.font_size = (uint32)tFont.Size();
	}
	tp.p.cursor_blink_rate = 1000000;
	tp.p.refresh_rate = 0;
	
	if (FindRGBColor(termpref, TP_BG, 0, &tp.p.bg) != B_OK)
		tp.p.bg = make_color(255,255,255,255);
	if (FindRGBColor(termpref, TP_FG, 0, &tp.p.fg) != B_OK)
		tp.p.fg = make_color(0,0,0,255);
	if (FindRGBColor(termpref, TP_CURBG, 0, &tp.p.curbg) != B_OK)
		tp.p.curbg = make_color(255,255,255,255);
	if (FindRGBColor(termpref, TP_CURFG, 0, &tp.p.curfg) != B_OK)
		tp.p.curfg = make_color(0,0,0,255);
	if (FindRGBColor(termpref, TP_SELBG, 0, &tp.p.selbg) != B_OK)
		tp.p.selbg = make_color(0,0,0,255);
	if (FindRGBColor(termpref, TP_SELFG, 0, &tp.p.selfg) != B_OK)
		tp.p.selfg = make_color(255,255,255,255);
	
	if (termpref.FindInt32(TP_ENCODING, (int32 *)&tp.p.encoding) != B_OK)
		tp.p.encoding = 0; // UTF-8
	
	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		BPath pTermPref;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
			return EINVAL;
		pTermPref.Append("Terminal");
		BFile fTermPref(pTermPref.Path(), B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
		if (fTermPref.InitCheck() != B_OK) {
			return fTermPref.InitCheck();
		}
		fTermPref.Write(&tp, sizeof(struct termprefs));
		BNodeInfo ni(&fTermPref);
		if (ni.InitCheck() == B_OK)
			ni.SetType("application/x-vnd.Be-pref");
	}

	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY) {
		BList teamList;
		app_info ainfo;
		int32 count, i;
		be_roster->GetAppList(&teamList);
		count = teamList.CountItems();
		for (i = 0; i < count; i++) {
			if (be_roster->GetRunningAppInfo((team_id)((addr_t)teamList.ItemAt(i)), &ainfo) == B_OK) {
				if (!strcmp(ainfo.signature, kBeOSTerminalAppSig)) {
					err = B_OK;
					BMessage msg(MSG_R5_SET_PREF);
					BMessenger msgr(NULL, ainfo.team);
					tp.x = 0;
					tp.y = 0;
					
					//msg.AddData("", 'UBYT', &(tp.p), sizeof(struct tpref));
					msg.AddData("", 'UBYT', &(tp), sizeof(struct termprefs));
					msg.AddSpecifier("Window", (int32)0);
					err = msgr.SendMessage(&msg);
				}
			}
		}
	}
	
	return B_OK;
}


status_t
TerminalThemesAddon::MakeThemeR5(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	status_t err;
	struct termprefs tp;
	BPath pTermPref;
	
	(void)flags;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
		return EINVAL;
	pTermPref.Append("Terminal");
	BFile fTermPref(pTermPref.Path(), B_READ_ONLY);
	if (fTermPref.InitCheck() != B_OK)
		return fTermPref.InitCheck();
	if (fTermPref.Read(&tp, sizeof(struct termprefs)) < (ssize_t)sizeof(struct termprefs))
		return EIO;
	if ((tp.magic != TP_MAGIC) || (tp.version != TP_VERSION))
		return EINVAL;
	termpref.AddInt32(TP_COLS, tp.p.cols);
	termpref.AddInt32(TP_ROWS, tp.p.rows);
	termpref.AddInt32(TP_TABWIDTH, tp.p.tab_width);
	BFont tFont;
	font_family ff;
	font_style fs;
	BString str(tp.p.font);
	str.Truncate(str.FindFirst('/'));
	strncpy(ff, str.String(), sizeof(ff));
	str.SetTo(tp.p.font);
	str.Remove(0, str.FindFirst('/')+1);
	strncpy(fs, str.String(), sizeof(fs));
	tFont.SetFamilyAndStyle(ff, fs);
	tFont.SetSize(tp.p.font_size);
	AddFont(termpref, TP_FONT, &tFont);
	AddRGBColor(termpref, TP_BG, tp.p.bg);
	AddRGBColor(termpref, TP_FG, tp.p.fg);
	AddRGBColor(termpref, TP_CURBG, tp.p.curbg);
	AddRGBColor(termpref, TP_CURFG, tp.p.curfg);
	AddRGBColor(termpref, TP_SELBG, tp.p.selbg);
	AddRGBColor(termpref, TP_SELFG, tp.p.selfg);
	termpref.AddInt32(TP_ENCODING, tp.p.encoding);

	err = SetMyMessage(theme, termpref);
	return B_OK;
}


status_t
TerminalThemesAddon::ApplyThemeHaiku(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	BMessage lines;
	status_t err;
	int32 ival;
	rgb_color color;
	BString s;
	int i;

	err = MyMessage(theme, termpref);
	if (err)
		return err;

	for (i = 0; i < NENTS(sHaikuPrefsMapInt32); i++) {
		if (termpref.FindInt32(sHaikuPrefsMapInt32[i].name, &ival) < B_OK)
			ival = sHaikuPrefsMapInt32[i].def;
		s = "";
		s << ival;
		lines.AddString(sHaikuPrefsMapInt32[i].pref, s.String());
	}

	for (i = 0; i < NENTS(sHaikuPrefsMapColors); i++) {
		if (FindRGBColor(termpref, sHaikuPrefsMapColors[i].name, 0, &color) != B_OK)
			color = sHaikuPrefsMapColors[i].def;
		s = "";
		s << color.red << ", " << color.green << ", " << color.blue;
		lines.AddString(sHaikuPrefsMapColors[i].pref, s.String());
	}

	BFont tFont;
	for (i = 0; FindFont(termpref, TP_FONT, i, &tFont) == B_OK; i++) {
		// at least set the size
		s = "";
		s << (int32)tFont.Size();
		lines.AddString(PREF_HALF_FONT_SIZE, s.String());
		//tFont.PrintToStream();
		//printf("fixed: %d\n", tFont.IsFixed());
		//printf("f&h fixed: %d\n", tFont.IsFullAndHalfFixed());
		// don't even try to set the font if it's not there or not fixed
		if (!tFont.IsFixed() && !tFont.IsFullAndHalfFixed())
			continue;
		font_family ff;
		font_style fs;
		tFont.GetFamilyAndStyle(&ff, &fs);
		s = "";
		s << ff;
		lines.AddString(PREF_HALF_FONT_FAMILY, s.String());
		s = "";
		s << fs;
		lines.AddString(PREF_HALF_FONT_STYLE, s.String());
	}

	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		SaveHaikuTerminalSettings(lines);
	}

	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY) {
		BList teamList;
		app_info ainfo;
		int32 count, i;
		be_roster->GetAppList(&teamList);
		count = teamList.CountItems();
		for (i = 0; i < count; i++) {
			if (be_roster->GetRunningAppInfo((team_id)((addr_t)teamList.ItemAt(i)), &ainfo) == B_OK) {
				if (!strcmp(ainfo.signature, kHaikuTerminalAppSig)) {
					err = B_OK;
					//XXX: WRITEME
/*					BMessage msg(MSG_R5_SET_PREF);
					BMessenger msgr(NULL, ainfo.team);
					tp.x = 0;
					tp.y = 0;
					
					//msg.AddData("", 'UBYT', &(tp.p), sizeof(struct tpref));
					msg.AddData("", 'UBYT', &(tp), sizeof(struct termprefs));
					msg.AddSpecifier("Window", 0L);
					err = msgr.SendMessage(&msg);
*/
				}
			}
		}
	}
	
	return B_OK;
}


status_t
TerminalThemesAddon::MakeThemeHaiku(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	BMessage lines;
	status_t err;
	BString value;
	int n, i;
	
	(void)flags;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	err = LoadHaikuTerminalSettings(lines);
	
	for (i = 0; i < NENTS(sHaikuPrefsMapInt32); i++) {
		int v;

		if (lines.FindString(sHaikuPrefsMapInt32[i].pref, &value) < B_OK)
			continue;

		n = sscanf(value.String(), "%d", &v);
		//printf("n=%d '%s'\n", n, value.String());

		if (n == 1)
			termpref.AddInt32(sHaikuPrefsMapInt32[i].name, v);
	}

	for (i = 0; i < NENTS(sHaikuPrefsMapColors); i++) {
		int r, g, b;

		if (lines.FindString(sHaikuPrefsMapColors[i].pref, &value) < B_OK)
			continue;

		n = sscanf(value.String(), "%d%*[, ]%d%*[, ]%d", &r, &g, &b);
		//printf("n=%d '%s' %d,%d,%d\n", n, value.String(), r, g, b);

		if (n == 3) {
			rgb_color c = make_color(r, g, b, 255);
			AddRGBColor(termpref, sHaikuPrefsMapColors[i].name, c);
		}
	}

	BFont font;
	BString s;
	font_family family;
	font_style style;
	float size = 12.0;
	memset(&family, 0, sizeof(family));
	memset(&style, 0, sizeof(style));

	if (lines.FindString(PREF_HALF_FONT_FAMILY, &s) == B_OK)
		strncpy(family, s.String(), B_FONT_FAMILY_LENGTH);

	if (lines.FindString(PREF_HALF_FONT_STYLE, &s) == B_OK)
		strncpy(style, s.String(), B_FONT_STYLE_LENGTH);

	font.SetFamilyAndStyle(family, style);

	if (lines.FindString(PREF_HALF_FONT_SIZE, &s) == B_OK)
		sscanf(s.String(), "%f", &size);

	font.SetSize(size);

	//termpref.PrintToStream();

	err = SetMyMessage(theme, termpref);
	return B_OK;
}


status_t
TerminalThemesAddon::LoadHaikuTerminalSettings(BMessage &into)
{
	BPath pTermPref;
	char buffer[1024];
	char key[B_FIELD_NAME_LENGTH], data[512];
	int n;
	FILE *file;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
		return EINVAL;
#ifdef __HAIKU__
	pTermPref.Append("Terminal");
	pTermPref.Append("Default");
#else
	pTermPref.Append("HaikuTerminal_settings");
#endif
	// cf PrefHandler.cpp
	file = fopen(pTermPref.Path(), "r");
	if (file == NULL)
		return B_ENTRY_NOT_FOUND;

	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		if (*buffer == '#')
			continue;

		n = sscanf(buffer, "%*[\"]%[^\"]%*[\"]%*[^\"]%*[\"]%[^\"]", key, data);
		if (n == 2) {
			into.AddString(key, data);
		}
	}
	fclose(file);
	return B_OK;
}


status_t
TerminalThemesAddon::SaveHaikuTerminalSettings(BMessage &from)
{
	BMessage settings;
	status_t err;
	BPath pTermPref;
	
	// load existing
	err = LoadHaikuTerminalSettings(settings);
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
		return EINVAL;
#ifdef __HAIKU__
	pTermPref.Append("Terminal");
	pTermPref.Append("Default");
#else
	pTermPref.Append("HaikuTerminal_settings");
#endif

	BFile file(pTermPref.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	char buffer[512];
	type_code type;
	char *key;
	err = file.InitCheck();
	if (err < B_OK)
		return err;

	// merge new values
	for (int32 i = 0;
			from.GetInfo(B_STRING_TYPE, i, GET_INFO_NAME_PTR(&key), &type) == B_OK;
			i++) {
		BString s;
		if (from.FindString(key, &s) < B_OK)
			continue;
		settings.RemoveName(key);
		settings.AddString(key, s.String());
	}

	for (int32 i = 0;
			settings.GetInfo(B_STRING_TYPE, i, GET_INFO_NAME_PTR(&key), &type) == B_OK;
			i++) {
		BString s;
		if (settings.FindString(key, &s) < B_OK)
			continue;
		int len = snprintf(buffer, sizeof(buffer), "\"%s\" , \"%s\"\n", key, s.String());
		file.Write(buffer, len);
	}
	return B_OK;
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new TerminalThemesAddon;
}

