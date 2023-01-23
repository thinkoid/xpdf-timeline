//========================================================================
//
// LTKButtonDialog.h
//
// Copyright 1997-2002 Glyph & Cog, LLC
//
//========================================================================

#ifndef LTKBUTTONDIALOG_H
#define LTKBUTTONDIALOG_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stddef.h>
#include <X11/Xlib.h>
#include "gtypes.h"
#include "LTKCompoundWidget.h"

class LTKButtonDialogWidget;

//------------------------------------------------------------------------
// LTKButtonDialog
//------------------------------------------------------------------------

class LTKButtonDialog {
public:

  LTKButtonDialog(LTKWindow *overWinA, char *title,
		  char *line1, char *line2, char *line3,
		  char *trueBtnLabel, char *falseBtnLabel);
  ~LTKButtonDialog();
  GBool go();

private:

  LTKWindow *overWin;
  LTKWindow *win;
  LTKButtonDialogWidget *widget;
};

//------------------------------------------------------------------------
// LTKButtonDialogWidget
//------------------------------------------------------------------------

class LTKButtonDialogWidget: public LTKCompoundWidget {
public:

  //---------- constructor ----------

  LTKButtonDialogWidget(char *nameA, int widgetNumA,
			char *line1, char *line2, char *line3,
			char *trueBtnLabel, char *falseBtnLabel);

  //---------- special access ----------

  GBool isDone() { return done; }
  GBool whichBtn() { return btn; }

protected:

  static void buttonCbk(LTKWidget *widget, int n, GBool on);

  GBool done;
  GBool btn;
};

#endif
