//========================================================================
//
// GfxState.h
//
//========================================================================

#ifndef GFXSTATE_H
#define GFXSTATE_H

#pragma interface

#include <stypes.h>

class Object;
class GfxFont;

//------------------------------------------------------------------------
// GfxColor
//------------------------------------------------------------------------

enum GfxColorMode {
  colorGray, colorCMYK, colorRGB
};

class GfxColor {
public:

  GfxColor(): mode(colorGray), gray(0) {}

  // Set color.
  void setGray(double gray1);
  void setCMYK(double c1, double m1, double y1, double k1);
  void setRGB(double r1, double g1, double b1);

  // Accessors.
  GfxColorMode getMode() { return mode; }
  double getGray() { return gray; }
  double getC() { return c; }
  double getM() { return m; }
  double getY() { return y; }
  double getK() { return k; }
  double getR() { return r; }
  double getG() { return g; }
  double getB() { return b; }

private:

  GfxColorMode mode;
  double gray;
  double c, m, y, k;
  double r, g, b;
};

//------------------------------------------------------------------------
// GfxColorSpace
//------------------------------------------------------------------------

class GfxColorSpace {
public:

  // Constructor.
  GfxColorSpace(int bits1, Object *colorSpace, Object *decode);

  // Destructor.
  ~GfxColorSpace();

  // Is color space valid?
  Boolean isOk() { return ok; }

  // Get stream decoding info.
  int getNumComponents() { return numComponents; }
  int getBits() { return bits; }

  // Get color mode.
  GfxColorMode getMode() { return mode; }
  Boolean isIndexed() { return indexed; }

  // Convert an input value to a color.
  void getGray(int x[4], uchar *gray);
  void getCMYK(int x[4], uchar *c, uchar *y, uchar *m, uchar *k);
  void getRGB(int x[4], uchar *r, uchar *g, uchar *b);

private:

  GfxColorMode mode;		// color mode
  Boolean indexed;		// use lookup table?
  int bits;			// bits per component
  int numComponents;		// number of components in input values
  int lookupComponents;		// number of components in lookup table
  uchar (*lookup)[4];		// lookup table
  Boolean ok;			// is color space valid?
};

//------------------------------------------------------------------------
// GfxSubpath and GfxPath
//------------------------------------------------------------------------

class GfxSubpath {
public:

  // Constructor.
  GfxSubpath(double x1, double y1);

  // Destructor.
  ~GfxSubpath();

  // Copy.
  GfxSubpath *copy() { return new GfxSubpath(x, y, n, size); }

  // Get points.
  int getNumPoints() { return n; }
  double getX(int i) { return x[i]; }
  double getY(int i) { return y[i]; }

  // Get last point.
  double getLastX() { return x[n-1]; }
  double getLastY() { return y[n-1]; }

  // Add a segment.
  void lineTo(double x1, double y1);

  // Close the subpath.
  void close()
    { if (x[n-1] != x[0] || y[n-1] != y[0]) lineTo(x[0], y[0]); }

private:

  double *x, *y;		// points
  int n;			// number of points
  int size;			// size of x/y arrays

  GfxSubpath(double *x1, double *y1, int n1, int size1);
};

class GfxPath {
public:

  // Constructor.
  GfxPath();

  // Destructor.
  ~GfxPath();

  // Copy.
  GfxPath *copy() { return new GfxPath(subpaths, n, size); }

  // Is the path non-empty, i.e., is there a current point?
  Boolean isPath() { return n > 0; }

  // Get subpaths.
  int getNumSubpaths() { return n; }
  GfxSubpath *getSubpath(int i) { return subpaths[i]; }

  // Get last point on last subpath.
  double getLastX() { return subpaths[n-1]->getLastX(); }
  double getLastY() { return subpaths[n-1]->getLastY(); }

  // Move the current point, i.e., start a new subpath.
  void moveTo(double x, double y);

  // Add a segment to the last subpath.
  void lineTo(double x, double y) { subpaths[n-1]->lineTo(x, y); }

  // Close the last subpath.
  void close() { subpaths[n-1]->close(); }

private:

  GfxSubpath **subpaths;	// subpaths
  int n;			// number of subpaths
  int size;			// size of subpaths array

  GfxPath(GfxSubpath **subpaths1, int n1, int size1);
};

//------------------------------------------------------------------------
// GfxState
//------------------------------------------------------------------------

class GfxState {
public:

  // Construct a default GfxState, for a device with resolution <dpi>,
  // page box (<x1>,<y1>)-(<x2>,<y2>), page rotation <rotate>, and
  // coordinate system specified by <upsideDown>.
  GfxState(int dpi, int x1, int y1, int x2, int y2, int rotate,
	   Boolean upsideDown);

  // Destructor.
  ~GfxState();

  // Copy.
  GfxState *copy() { return new GfxState(this); }

  // Accessors.
  double *getCTM() { return ctm; }
  int getPageWidth() { return pageWidth; }
  int getPageHeight() { return pageHeight; }
  GfxColor *getFillColor() { return &fillColor; }
  GfxColor *getStrokeColor() { return &strokeColor; }
  double getLineWidth() { return lineWidth; }
  void getLineDash(double **dash, int *length, double *start)
    { *dash = lineDash; *length = lineDashLength; *start = lineDashStart; }
  int getFlatness() { return flatness; }
  int getLineJoin() { return lineJoin; }
  int getLineCap() { return lineCap; }
  double getMiterLimit() { return miterLimit; }
  GfxFont *getFont() { return font; }
  double getFontSize() { return fontSize; }
  double *getTextMat() { return textMat; }
  double getCharSpace() { return charSpace; }
  double getWordSpace() { return wordSpace; }
  double getHorizScaling() { return horizScaling; }
  double getLeading() { return leading; }
  double getRise() { return rise; }
  int getRender() { return render; }
  GfxPath *getPath() { return path; }
  double getCurX() { return curX; }
  double getCurY() { return curY; }
  double getLineX() { return lineX; }
  double getLineY() { return lineY; }

  // Is there a current point?
  Boolean isPath() { return path->isPath(); }

  // Transforms.
  void transform(double x1, double y1, double *x2, double *y2);
  void transformDelta(double x1, double y1, double *x2, double *y2);
  double transformWidth(double w);
  void textTransform(double x1, double y1, double *x2, double *y2);
  void textTransformDelta(double x1, double y1, double *x2, double *y2);
  double getTransformedLineWidth() { return transformWidth(lineWidth); }
  double getTransformedFontSize();

  // Change state parameters.
  void concatCTM(double a, double b, double c,
		 double d, double e, double f);
  void setFillGray(double gray)
    { fillColor.setGray(gray); }
  void setFillCMYK(double c, double m, double y, double k)
    { fillColor.setCMYK(c, m, y, k); }
  void setFillRGB(double r, double g, double b)
    { fillColor.setRGB(r, g, b); }
  void setStrokeGray(double gray)
    { strokeColor.setGray(gray); }
  void setStrokeCMYK(double c, double m, double y, double k)
    { strokeColor.setCMYK(c, m, y, k); }
  void setStrokeRGB(double r, double g, double b)
    { strokeColor.setRGB(r, g, b); }
  void setLineWidth(double width)
    { lineWidth = width; }
  void setLineDash(double *dash, int length, double start);
  void setFlatness(int flatness1) { flatness = flatness1; }
  void setLineJoin(int lineJoin1) { lineJoin = lineJoin1; }
  void setLineCap(int lineCap1) { lineCap = lineCap1; }
  void setMiterLimit(double miterLimit1) { miterLimit = miterLimit1; }
  void setFont(GfxFont *font1, double fontSize1)
    { font = font1; fontSize = fontSize1; }
  void setTextMat(double a, double b, double c,
		  double d, double e, double f)
    { textMat[0] = a; textMat[1] = b; textMat[2] = c;
      textMat[3] = d; textMat[4] = e; textMat[5] = f; }
  void setCharSpace(double space)
    { charSpace = space; }
  void setWordSpace(double space)
    { wordSpace = space; }
  void setHorizScaling(double scale)
    { horizScaling = scale; }
  void setLeading(double leading1)
    { leading = leading1; }
  void setRise(double rise1)
    { rise = rise1; }
  void setRender(int render1)
    { render = render1; }

  // Add to path.
  void moveTo(double x, double y)
    { path->moveTo(curX = x, curY = y); }
  void lineTo(double x, double y)
    { path->lineTo(curX = x, curY = y); }
  void curveTo(double x1, double y1, double x2, double y2,
	       double x3, double y3);
  void closePath()
    { path->close(); curX = path->getLastX(); curY = path->getLastY(); }
  void clearPath();

  // Text position.
  void textMoveTo(double tx, double ty)
    { lineX = tx; lineY = ty; textTransform(tx, ty, &curX, &curY); }
  void textShift(double tx);

  // Push/pop GfxState on/off stack.
  GfxState *save();
  GfxState *restore();

private:

  double ctm[6];		// coord transform matrix
  int pageWidth, pageHeight;	// page size (pixels)

  GfxColor fillColor;		// fill color
  GfxColor strokeColor;		// stroke color

  double lineWidth;		// line width
  double *lineDash;		// line dash
  int lineDashLength;
  double lineDashStart;
  int flatness;			// curve flatness
  int lineJoin;			// line join style
  int lineCap;			// line cap style
  double miterLimit;		// line miter limit

  GfxFont *font;		// font
  double fontSize;		// font size
  double textMat[6];		// text matrix
  double charSpace;		// character spacing
  double wordSpace;		// word spacing
  double horizScaling;		// horizontal scaling
  double leading;		// text leading
  double rise;			// text rise
  int render;			// text rendering mode

  GfxPath *path;		// array of path elements
  double curX, curY;		// current point (user coords)
  double lineX, lineY;		// start of current text line (text coords)

  GfxState *saved;		// next GfxState on stack

  GfxState(GfxState *state);
  void doCurveTo(double x0, double y0, double x1, double y1,
		 double x2, double y2, double x3, double y3,
		 int splits);
};

#endif
