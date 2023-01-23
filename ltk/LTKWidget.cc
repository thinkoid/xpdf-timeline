//========================================================================
//
// LTKWidget.cc
//
// Widget base class.
//
// Copyright 1996 Derek B. Noonburg
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
#include "LTKApp.h"
#include "LTKWindow.h"
#include "LTKWidget.h"

LTKWidget::LTKWidget(char *nameA, int widgetNumA) {
  name = nameA;
  widgetNum = widgetNumA;
  parent = NULL;
  compParent = NULL;
  x = 0;
  y = 0;
  width = 0;
  height = 0;
  btnPressCbk = NULL;
  btnReleaseCbk = NULL;
  mouseMoveCbk = NULL;
  mouseDragCbk = NULL;
  xwin = None;
  mapped = gFalse;
  next = NULL;
}

LTKWidget::~LTKWidget() {
}

void LTKWidget::setParent(LTKWindow *parentA) {
  parent = parentA;
  parent->addWidget(this);
}

void LTKWidget::setCompoundParent(LTKWidget *compParentA) {
  compParent = compParentA;
}

long LTKWidget::getEventMask() {
  long mask;

  mask = ExposureMask;
  if (btnPressCbk)
    mask |= ButtonPressMask | ButtonReleaseMask;
  if (btnReleaseCbk)
    mask |= ButtonReleaseMask;
  if (mouseMoveCbk)
    mask |= PointerMotionMask | PointerMotionHintMask;
  if (mouseDragCbk)
    mask |= ButtonMotionMask | PointerMotionHintMask;
  return mask;
}

void LTKWidget::layout2(int xA, int yA, int widthA, int heightA) {
  x = xA;
  y = yA;
  width = widthA;
  height = heightA;
}

void LTKWidget::layout3() {
  if (xwin == None) {
    xwin = XCreateSimpleWindow(getDisplay(), parent->getXWindow(),
			       x, y, width, height, 0,
			       getFgColor(), getBgColor());
    parent->getApp()->registerXWindow(xwin, parent, this);
    XSelectInput(getDisplay(), xwin, getEventMask());
  } else {
    XMoveResizeWindow(getDisplay(), xwin, x, y, width, height);
  }
}

void LTKWidget::map() {
  XMapWindow(getDisplay(), xwin);
  mapped = gTrue;
}

void LTKWidget::clear() {
  XClearWindow(getDisplay(), xwin);
}

void LTKWidget::buttonPress(int mx, int my, int button, GBool dblClick) {
  if (btnPressCbk)
    (*btnPressCbk)(this, widgetNum, mx, my, button, dblClick);
}

void LTKWidget::buttonRelease(int mx, int my, int button, GBool click) {
  if (btnReleaseCbk)
    (*btnReleaseCbk)(this, widgetNum, mx, my, button, click);
}

void LTKWidget::mouseMove(int mx, int my, int btn) {
  if (btn == 0) {
    if (mouseMoveCbk)
      (*mouseMoveCbk)(this, widgetNum, mx, my);
  } else {
    if (mouseDragCbk)
      (*mouseDragCbk)(this, widgetNum, mx, my, btn);
  }
}
