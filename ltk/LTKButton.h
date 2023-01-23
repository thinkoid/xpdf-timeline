//========================================================================
//
// LTKButton.h
//
// Copyright 1996-2002 Glyph & Cog, LLC
//
//========================================================================

#ifndef LTKBUTTON_H
#define LTKBUTTON_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stddef.h>
#include <X11/Xlib.h>
#include "gtypes.h"
#include "GString.h"
#include "LTKWidget.h"

//------------------------------------------------------------------------
// button action type
//------------------------------------------------------------------------

enum LTKButtonAction {
  ltkButtonClick,		// momentarily on
  ltkButtonSticky,		// stays on until setState() is called
  ltkButtonToggle		// press on, press off
};

//------------------------------------------------------------------------
// LTKButton
//------------------------------------------------------------------------

class LTKButton: public LTKWidget {
public:

  //---------- constructors and destructor ----------

  LTKButton(char *nameA, int widgetNumA, const char *labelA,
	    LTKButtonAction actionA, LTKBoolValCbk pressCbkA);

  LTKButton(char *nameA, int widgetNumA,
	    unsigned char *iconDataA,
	    int iconWidthA, int iconHeightA,
	    LTKButtonAction actionA, LTKBoolValCbk pressCbkA);

  virtual ~LTKButton();

  //---------- access ----------

  virtual long getEventMask();

  //---------- special access ----------

  void setState(GBool onA);

  //---------- layout ----------

  virtual void layout1();
  virtual void layout3();

  //---------- drawing ----------

  virtual void redraw();

  //---------- callbacks and event handlers ----------

  virtual void buttonPress(int bx, int by, int button, GBool dblClick);
  virtual void buttonRelease(int bx, int by, int button, GBool click);
  virtual void activateDefault();

protected:

  GString *label;		// label (button has either label or icon)
  Pixmap icon;			// icon
  unsigned char *iconData;	// the icon data (bitmap format)
  int iconWidth, iconHeight;	// the icon size
  LTKButtonAction action;	// action kind
  GBool on;			// current state
  GBool oldOn;			// saved state
  int textWidth, textHeight;	// size of text
  int textBase;			// baseline offset

  LTKBoolValCbk pressCbk;	// button-press callback
};

#endif