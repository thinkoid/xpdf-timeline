//========================================================================
//
// LTKScrollingCanvas.h
//
// Copyright 1996-2002 Glyph & Cog, LLC
//
//========================================================================

#ifndef LTKSCROLLINGCANVAS_H
#define LTKSCROLLINGCANVAS_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stddef.h>
#include <X11/Xlib.h>
#include "LTKWidget.h"

//------------------------------------------------------------------------
// LTKScrollingCanvas
//------------------------------------------------------------------------

class LTKScrollingCanvas: public LTKWidget {
public:

  //---------- constructor and destructor ----------

  LTKScrollingCanvas(char *nameA, int widgetNumA,
		     int realWidthA, int realHeightA,
		     int minWidthA, int minHeightA);

  virtual ~LTKScrollingCanvas();

  //---------- special access ----------

  Pixmap getPixmap() { return pixmap; }
  void resize(int realWidthA, int realHeightA);
  int getRealWidth() { return realWidth; }
  int getRealHeight() { return realHeight; }
  int getMaxX() { return realWidth >= width ? realWidth - width : 0; }
  int getMaxY() { return realHeight >= height ? realHeight - height : 0; }
  void setScrollPos(int xA, int yA);
  void scroll(int xA, int yA);
  void redrawRect(int x1, int y1, int x2, int y2);

  //---------- layout ----------

  virtual void layout1();
  virtual void layout3();

  //---------- drawing ----------

  virtual void redraw();

  //---------- callbacks and event handlers ----------

  virtual void buttonPress(int mx, int my, int button, GBool dblClick);
  virtual void buttonRelease(int mx, int my, int button, GBool click);
  virtual void mouseMove(int mx, int my, int btn);

protected:

  int realWidth, realHeight;	// size of "real" canvas, i.e., backdrop
  int minWidth, minHeight;	// minimum size

  int left, top;		// location of window in "real" canvas
  Pixmap pixmap;		// the "real" canvas drawable
};

#endif