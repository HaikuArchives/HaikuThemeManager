/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Entry.h>
#include <Screen.h>
#include <Window.h>

#ifdef __HAIKU__
#include <locale/Catalog.h>
#endif
#ifdef B_ZETA_VERSION
#include <locale/Locale.h>
#define B_TRANSLATE _T
#endif

#ifndef B_TRANSLATE
#define B_TRANSLATE(v) v
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ThemesApp"

#include <stdio.h>
#include <unistd.h>

#include "ThemesApp.h"
#include "ThemeInterfaceView.h"

const char *kThemesAppSig = "application/x-vnd.mmu_man-ThemeManager";


ThemesApp::ThemesApp()
	: BApplication(kThemesAppSig)
{
}


ThemesApp::~ThemesApp()
{
}


void
ThemesApp::ReadyToRun()
{
	BScreen s;
	BRect frame(0, 0, 680, 400);
	frame.OffsetBySelf(s.Frame().Width()/2 - frame.Width()/2, 
						s.Frame().Height()/2 - frame.Height()/2);
	BWindow *w = new BWindow(frame, B_TRANSLATE("Theme Manager"), B_TITLED_WINDOW, B_NOT_RESIZABLE | B_QUIT_ON_WINDOW_CLOSE);
	ThemeInterfaceView *v = new ThemeInterfaceView(w->Bounds());
	w->AddChild(v);
	w->Show();
}


void
ThemesApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
	default:
		BApplication::MessageReceived(message);
	}
}


void
ThemesApp::RefsReceived(BMessage *message)
{
	message->PrintToStream();
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK) {
		;
	}
}


int main(int argc, char **argv)
{
	ThemesApp app;
	app.Run();
}
