//========================================================================
//
// LTKDblBufCanvas.cc
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#pragma implementation

#include <stdlib.h>
#include <stdarg.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <LTKWindow.h>
#include <LTKDblBufCanvas.h>

LTKDblBufCanvas::LTKDblBufCanvas(char *name1, int minWidth1, int minHeight1):
    LTKWidget(ltkDblBufCanvas, name1) {
  minWidth = minWidth1;
  minHeight = minHeight1;
  pixmap = None;
}

LTKDblBufCanvas::LTKDblBufCanvas(LTKDblBufCanvas *canvas):
    LTKWidget(canvas) {
  minWidth = canvas->minWidth;
  minHeight = canvas->minHeight;
  pixmap = None;
}

LTKDblBufCanvas::~LTKDblBufCanvas() {
  if (pixmap != None)
    XFreePixmap(getDisplay(), pixmap);
}

long LTKDblBufCanvas::getEventMask() {
  return ExposureMask;
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
