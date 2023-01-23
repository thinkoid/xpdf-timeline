//========================================================================
//
// LTKDblBufCanvas.cc
//
// Copyright 1996-2002 Glyph & Cog, LLC
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include <aconf.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "LTKWindow.h"
#include "LTKDblBufCanvas.h"

LTKDblBufCanvas::LTKDblBufCanvas(char *nameA, int widgetNumA,
				 int minWidthA, int minHeightA):
    LTKWidget(nameA, widgetNumA) {
  minWidth = minWidthA;
  minHeight = minHeightA;
  pixmap = None;
}

LTKDblBufCanvas::~LTKDblBufCanvas() {
  if (pixmap != None)
    XFreePixmap(getDisplay(), pixmap);
}

void LTKDblBufCanvas::layout1() {
  width = minWidth;
  height = minHeight;
}

void LTKDblBufCanvas::layout3() {
  Pixmap oldPixmap;

  LTKWidget::layout3();
  oldPixmap = pixmap;
  pixmap = XCreatePixmap(getDisplay(), getXWindow(), width, height,
			 DefaultDepth(getDisplay(), getScreenNum()));
  XFillRectangle(getDisplay(), pixmap, getBgGC(),
		 0, 0, width, height);
  if (oldPixmap != None) {
    XCopyArea(getDisplay(), oldPixmap, pixmap, getFgGC(),
	      0, 0, width, height, 0, 0);
    XFreePixmap(getDisplay(), oldPixmap);
  }
}

void LTKDblBufCanvas::redraw() {
  XCopyArea(getDisplay(), pixmap, getXWindow(), getFgGC(),
	    0, 0, width, height, 0, 0);
}