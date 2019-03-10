/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCREENSHOT_VIEW_H_
#define _SCREENSHOT_VIEW_H_

#include <View.h>

class BBitmap;

class ScreenshotView : public BView
{
public:
					ScreenshotView(BRect bounds, const char *name,
						uint32 resizeMask = B_FOLLOW_ALL,
						uint32 flags = B_FULL_UPDATE_ON_RESIZE
							| B_WILL_DRAW | B_FRAME_EVENTS);
	virtual			~ScreenshotView();

	virtual	void 	AttachedToWindow();

	void			UpdateScreenshot();
	void			SetScreenshot(BBitmap *bitmap);
private:
	BBitmap*		fBitmap;
};

#endif // _SCREENSHOT_VIEW_H_
