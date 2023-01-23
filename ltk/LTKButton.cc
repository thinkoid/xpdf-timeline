//========================================================================
//
// LTKButton.cc
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
#include "LTKButton.h"
#include "LTKBorder.h"

LTKButton::LTKButton(char *nameA, int widgetNumA, const char *labelA,
		     LTKButtonAction actionA, LTKBoolValCbk pressCbkA):
    LTKWidget(nameA, widgetNumA) {
  label = new GString(labelA);
  icon = None;
  iconData = NULL;
  action = actionA;
  on = gFalse;
  pressCbk = pressCbkA;
}

LTKButton::LTKButton(char *name1, int widgetNum1,
		     unsigned char *iconData1,
		     int iconWidth1, int iconHeight1,
		     LTKButtonAction action1, LTKBoolValCbk pressCbk1):
    LTKWidget(name1, widgetNum1) {
  label = NULL;
  icon = None;
  iconData = iconData1;
  iconWidth = iconWidth1;
  iconHeight = iconHeight1;
  action = action1;
  on = gFalse;
  pressCbk = pressCbk1;
}

LTKButton::~LTKButton() {
  if (label)
    delete label;
  if (icon)
    XFreePixmap(getDisplay(), icon);
}

long LTKButton::getEventMask() {
  return LTKWidget::getEventMask() | ButtonPressMask | ButtonReleaseMask;
}

void LTKButton::layout1() {
  XFontStruct *fontStruct;
  XCharStruct extents;
  int direction, ascent, descent;

  if (label) {
    fontStruct = getXFontStruct();
    XTextExtents(fontStruct, label->getCString(), label->getLength(),
		 &direction, &ascent, &descent, &extents);
    textWidth = extents.width;
    textHeight = fontStruct->ascent + fontStruct->descent;
    textBase = fontStruct->ascent;
    width = textWidth + 12 + 2 * ltkBorderWidth;
    height = textHeight + 4 + 2 * ltkBorderWidth;
  } else {
    width = iconWidth + 12 + 2 * ltkBorderWidth;
    height = iconHeight + 4 + 2 * ltkBorderWidth;
  }
}

void LTKButton::layout3() {
  LTKWidget::layout3();
  if (iconData && icon == None) {
    icon = XCreatePixmapFromBitmapData(
             getDisplay(), getXWindow(), (char *)iconData,
             iconWidth, iconHeight, getFgColor(), getBgColor(),
	     DefaultDepth(getDisplay(), getScreenNum()));
  }
}

void LTKButton::redraw() {
  int tx, ty;

  ltkDrawBorder(getDisplay(), xwin, getBrightGC(), getDarkGC(), getBgGC(),
		0, 0, width, height, on ? ltkBorderSunken : ltkBorderRaised);
  if (label) {
    tx = (width - textWidth) / 2;
    ty = (height - textHeight) / 2 + textBase;
    XDrawString(getDisplay(), xwin, getFgGC(), tx, ty,
		label->getCString(), label->getLength());
  } else {
    XCopyArea(getDisplay(), icon, getXWindow(), getFgGC(),
	      0, 0, iconWidth, iconHeight,
	      (width - iconWidth) / 2, (height - iconHeight) / 2);
  }
}

void LTKButton::buttonPress(int mx, int my, int button, GBool dblClick) {
  oldOn = on;
  switch (action) {
  case ltkButtonClick:
    setState(gTrue);
    break;
  case ltkButtonSticky:
    setState(gTrue);
    break;
  case ltkButtonToggle:
    setState(!on);
    break;
  }
}

void LTKButton::buttonRelease(int mx, int my, int button, GBool click) {
  // mouse was released over button
  if (mx >= 0 && mx < width && my >= 0 && my < height) {
    switch (action) {
    case ltkButtonClick:
      setState(gFalse);
      break;
    case ltkButtonSticky:
      break;
    case ltkButtonToggle:
      break;
    }
    if (pressCbk)
      (*pressCbk)(this, widgetNum, on);

  // mouse was released outside button
  } else {
    setState(oldOn);
  }
}

//~ add a delay between press and release
void LTKButton::activateDefault() {
  buttonPress(0, 0, 1, gFalse);
  XFlush(getDisplay());
  buttonRelease(0, 0, 1, gTrue);
}

void LTKButton::setState(GBool onA) {
  if (onA != on) {
    on = onA;
    ltkDrawBorder(getDisplay(), xwin, getBrightGC(), getDarkGC(), getBgGC(),
		  0, 0, width, height, on ? ltkBorderSunken : ltkBorderRaised);
  }
}