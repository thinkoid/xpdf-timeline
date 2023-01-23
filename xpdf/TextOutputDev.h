//========================================================================
//
// TextOutputDev.h
//
// Copyright 1997 Derek B. Noonburg
//
//========================================================================

#ifndef TEXTOUTPUTDEV_H
#define TEXTOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stdio.h>
#include <gtypes.h>
#include "OutputDev.h"

class GfxState;
class GfxFont;
class GString;

//------------------------------------------------------------------------
// TextString
//------------------------------------------------------------------------

class TextString {
public:

  // Constructor.
  TextString(GfxState *state);

  // Destructor.
  ~TextString();

  // Add a character to the string.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy,
	       Guchar c, GBool useASCII7);

private:

  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  int col;			// starting column
  GString *text;		// the text
  double *xRight;		// right-hand x coord of each char
  TextString *yxNext;		// next string in y-major order
  TextString *xyNext;		// next string in x-major order

  friend class TextBlock;
  friend class TextPage;
};

//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

class TextPage {
public:

  // Constructor.
  TextPage(GBool useASCII71);

  // Destructor.
  ~TextPage();

  // Begin a new string.
  void beginString(GfxState *state, GString *s);

  // Add a character to the current string.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy, Guchar c);

  // End the current string, sorting it into the list of strings.
  void endString();

  // Coalesce strings that look like parts of the same line.
  void coalesce();

  // Find a string.  If <top> is true, starts looking at <xMin>,<yMin>;
  // otherwise starts looking at top of page.  If <bottom> is true,
  // stops looking at <xMax>,<yMax>; otherwise stops looking at bottom
  // of page.  If found, sets the text bounding rectange and returns
  // true; otherwise returns false.
  GBool findText(char *s, GBool top, GBool bottom,
		 double *xMin, double *yMin,
		 double *xMax, double *yMax);

  // Get the text which is inside the specified rectangle.
  GString *getText(double xMin, double yMin,
		   double xMax, double yMax);

  // Dump contents of page to a file.
  void dump(FILE *f);

  // Clear the page.
  void clear();

private:

  GBool useASCII7;		// use 7-bit ASCII?

  TextString *curStr;		// currently active string

  TextString *yxStrings;	// strings in y-major order
  TextString *xyStrings;	// strings in x-major order
};

//------------------------------------------------------------------------
// TextOutputDev
//------------------------------------------------------------------------

class TextOutputDev: public OutputDev {
public:

  // Open a text output file.  If <fileName> is NULL, no file is written
  // (this is useful, e.g., for searching text).  If <useASCII7> is true,
  // text is converted to 7-bit ASCII; otherwise, text is converted to
  // 8-bit ISO Latin-1.
  TextOutputDev(char *fileName, GBool useASCII7);

  // Destructor.
  virtual ~TextOutputDev();

  // Check if file was successfully created.
  virtual GBool isOk() { return ok; }

  //---- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() { return gTrue; }

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() { return gTrue; }

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  //----- text drawing
  virtual void beginString(GfxState *state, GString *s);
  virtual void endString(GfxState *state);
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy, Guchar c);

  //----- special access

  // Find a string.  If <top> is true, starts looking at <xMin>,<yMin>;
  // otherwise starts looking at top of page.  If <bottom> is true,
  // stops looking at <xMax>,<yMax>; otherwise stops looking at bottom
  // of page.  If found, sets the text bounding rectange and returns
  // true; otherwise returns false.
  GBool findText(char *s, GBool top, GBool bottom,
		 double *xMin, double *yMin,
		 double *xMax, double *yMax);

private:

  FILE *f;			// text file
  GBool needClose;		// need to close the file?
  TextPage *text;		// text for the current page
  GBool ok;			// set up ok?
};

#endif
