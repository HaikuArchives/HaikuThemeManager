/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ScreenshotView.h"
#include <View.h>
#include <Bitmap.h>
#include <stdio.h>
#include <Debug.h>


// PRIVATE: in libzeta for now.
extern status_t ScaleBitmap(const BBitmap& inBitmap, BBitmap& outBitmap);


ScreenshotView::ScreenshotView(BRect bounds, const char *name,
						uint32 resizeMask = B_FOLLOW_ALL,
						uint32 flags = B_FULL_UPDATE_ON_RESIZE
							| B_WILL_DRAW | B_FRAME_EVENTS)
	: BView(bounds, name, resizeMask, flags)
{
	fBitmap = NULL;
}


ScreenshotView::~ScreenshotView()
{
	if (fBitmap)
		delete fBitmap;
}


void
ScreenshotView::AttachedToWindow()
{
	UpdateScreenshot();
}


void
ScreenshotView::UpdateScreenshot()
{
	if (Window() == NULL)
		return;

	ClearViewBitmap();

	if (fBitmap) {
#ifdef __HAIKU__
		SetViewBitmap(fBitmap, fBitmap->Bounds(), Bounds(), B_FOLLOW_ALL,
			B_FILTER_BITMAP_BILINEAR);

#else
		SetViewBitmap(fBitmap, fBitmap->Bounds(), Bounds());
#endif
	}

	Invalidate();
}


void
ScreenshotView::SetScreenshot(BBitmap *bitmap)
{
	if (fBitmap)
		delete fBitmap;
	fBitmap = bitmap;

#ifdef B_ZETA_VERSION
	if (bitmap)
	{
		BBitmap *scaled = new BBitmap(Bounds(), B_RGB32);
		status_t err = ENOSYS;
		err = ScaleBitmap(*bitmap, *scaled);
		if( err == B_OK ) {
			fBitmap = scaled;
			delete bitmap;
		} else
			delete scaled;
	}
#endif

	UpdateScreenshot();
}
