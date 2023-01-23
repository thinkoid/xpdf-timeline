//========================================================================
//
// XOutputDev.cc
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "gmem.h"
#include "gfile.h"
#include "GString.h"
#include "GList.h"
#include "Object.h"
#include "Stream.h"
#include "Link.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "UnicodeMap.h"
#include "CharCodeToUnicode.h"
#include "FontFile.h"
#include "Error.h"
#include "TextOutputDev.h"
#include "XOutputDev.h"
#if HAVE_T1LIB_H
#include "T1Font.h"
#endif
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
#include "FTFont.h"
#endif
#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
#include "TTFont.h"
#endif

#ifdef VMS
#if (__VMS_VER < 70000000)
extern "C" int unlink(char *filename);
#endif
#endif

#ifdef XlibSpecificationRelease
#if XlibSpecificationRelease < 5
typedef char *XPointer;
#endif
#else
typedef char *XPointer;
#endif

//------------------------------------------------------------------------
// Constants and macros
//------------------------------------------------------------------------

#define xoutRound(x) ((int)(x + 0.5))

#define maxCurveSplits 6	// max number of splits when recursively
				//   drawing Bezier curves

//------------------------------------------------------------------------
// Font substitutions
//------------------------------------------------------------------------

struct XOutFontSubst {
  char *name;
  double mWidth;
};

// index: {symbolic:12, fixed:8, serif:4, sans-serif:0} + bold*2 + italic
static XOutFontSubst xOutSubstFonts[16] = {
  {"Helvetica",             0.833},
  {"Helvetica-Oblique",     0.833},
  {"Helvetica-Bold",        0.889},
  {"Helvetica-BoldOblique", 0.889},
  {"Times-Roman",           0.788},
  {"Times-Italic",          0.722},
  {"Times-Bold",            0.833},
  {"Times-BoldItalic",      0.778},
  {"Courier",               0.600},
  {"Courier-Oblique",       0.600},
  {"Courier-Bold",          0.600},
  {"Courier-BoldOblique",   0.600},
  {"Symbol",                0.576},
  {"Symbol",                0.576},
  {"Symbol",                0.576},
  {"Symbol",                0.576}
};

//------------------------------------------------------------------------
// XOutputFont
//------------------------------------------------------------------------

XOutputFont::XOutputFont(Ref *idA, double m11OrigA, double m12OrigA,
			 double m21OrigA, double m22OrigA,
			 double m11A, double m12A, double m21A, double m22A,
			 Display *displayA) {
  id = *idA;
  display = displayA;
  m11Orig = m11OrigA;
  m12Orig = m12OrigA;
  m21Orig = m21OrigA;
  m22Orig = m22OrigA;
  m11 = m11A;
  m12 = m12A;
  m21 = m21A;
  m22 = m22A;
}

XOutputFont::~XOutputFont() {
}

#if HAVE_T1LIB_H
//------------------------------------------------------------------------
// XOutputT1Font
//------------------------------------------------------------------------

XOutputT1Font::XOutputT1Font(Ref *idA, T1FontFile *fontFileA,
			     double m11OrigA, double m12OrigA,
			     double m21OrigA, double m22OrigA,
			     double m11A, double m12A,
			     double m21A, double m22A, Display *displayA):
  XOutputFont(idA, m11OrigA, m12OrigA, m21OrigA, m22OrigA,
	      m11A, m12A, m21A, m22A, displayA)
{
  double matrix[4];

  fontFile = fontFileA;

  // create the transformed instance
  matrix[0] = m11;
  matrix[1] = -m12;
  matrix[2] = m21;
  matrix[3] = -m22;
  font = new T1Font(fontFile, matrix);
}

XOutputT1Font::~XOutputT1Font() {
  if (font) {
    delete font;
  }
}

GBool XOutputT1Font::isOk() {
  return font != NULL;
}

void XOutputT1Font::updateGC(GC gc) {
}

void XOutputT1Font::drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			     GC gc, double x, double y, double dx, double dy,
			     CharCode c, Unicode *u, int uLen) {
  GfxRGB rgb;

  if (state->getRender() & 1) {
    state->getStrokeRGB(&rgb);
  } else {
    state->getFillRGB(&rgb);
  }
  font->drawChar(pixmap, w, h, gc, xoutRound(x), xoutRound(y),
		 (int)(rgb.r * 65535), (int)(rgb.g * 65535),
		 (int)(rgb.b * 65535), c, u[0]);
}
#endif // HAVE_T1LIB_H

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
//------------------------------------------------------------------------
// XOutputFTFont
//------------------------------------------------------------------------

XOutputFTFont::XOutputFTFont(Ref *idA, FTFontFile *fontFileA,
			     double m11OrigA, double m12OrigA,
			     double m21OrigA, double m22OrigA,
			     double m11A, double m12A,
			     double m21A, double m22A, Display *displayA):
  XOutputFont(idA, m11OrigA, m12OrigA, m21OrigA, m22OrigA,
	      m11A, m12A, m21A, m22A, displayA)
{
  double matrix[4];

  fontFile = fontFileA;

  // create the transformed instance
  matrix[0] = m11;
  matrix[1] = -m12;
  matrix[2] = m21;
  matrix[3] = -m22;
  font = new FTFont(fontFile, matrix);
}

XOutputFTFont::~XOutputFTFont() {
  if (font) {
    delete font;
  }
}

GBool XOutputFTFont::isOk() {
  return font != NULL;
}

void XOutputFTFont::updateGC(GC gc) {
}

void XOutputFTFont::drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			     GC gc, double x, double y, double dx, double dy,
			     CharCode c, Unicode *u, int uLen) {
  GfxRGB rgb;

  if (state->getRender() & 1) {
    state->getStrokeRGB(&rgb);
  } else {
    state->getFillRGB(&rgb);
  }
  font->drawChar(pixmap, w, h, gc, xoutRound(x), xoutRound(y),
		 (int)(rgb.r * 65535), (int)(rgb.g * 65535),
		 (int)(rgb.b * 65535), c, u[0]);
}
#endif // FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
//------------------------------------------------------------------------
// XOutputTTFont
//------------------------------------------------------------------------

XOutputTTFont::XOutputTTFont(Ref *idA, TTFontFile *fontFileA,
			     double m11OrigA, double m12OrigA,
			     double m21OrigA, double m22OrigA,
			     double m11A, double m12A,
			     double m21A, double m22A, Display *displayA):
  XOutputFont(idA, m11OrigA, m12OrigA, m21OrigA, m22OrigA,
	      m11A, m12A, m21A, m22A, displayA)
{
  double matrix[4];

  fontFile = fontFileA;

  // create the transformed instance
  matrix[0] = m11;
  matrix[1] = -m12;
  matrix[2] = m21;
  matrix[3] = -m22;
  font = new TTFont(fontFile, matrix);
}

XOutputTTFont::~XOutputTTFont() {
  if (font) {
    delete font;
  }
}

GBool XOutputTTFont::isOk() {
  return font != NULL;
}

void XOutputTTFont::updateGC(GC gc) {
}

void XOutputTTFont::drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			     GC gc, double x, double y, double dx, double dy,
			     CharCode c, Unicode *u, int uLen) {
  GfxRGB rgb;

  if (state->getRender() & 1) {
    state->getStrokeRGB(&rgb);
  } else {
    state->getFillRGB(&rgb);
  }
  font->drawChar(pixmap, w, h, gc, xoutRound(x), xoutRound(y),
		 (int)(rgb.r * 65535), (int)(rgb.g * 65535),
		 (int)(rgb.b * 65535), c, u[0]);
}
#endif // !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

//------------------------------------------------------------------------
// XOutputServer8BitFont
//------------------------------------------------------------------------

XOutputServer8BitFont::XOutputServer8BitFont(Ref *idA, GString *xlfdFmt, 
					     UnicodeMap *xUMapA,
					     CharCodeToUnicode *fontUMap,
					     double m11OrigA, double m12OrigA,
					     double m21OrigA, double m22OrigA,
					     double m11A, double m12A,
					     double m21A, double m22A,
					     Display *displayA):
  XOutputFont(idA, m11OrigA, m12OrigA, m21OrigA, m22OrigA,
	      m11A, m12A, m21A, m22A, displayA)
{
  double size, ntm11, ntm12, ntm21, ntm22;
  GBool rotated;
  int startSize, sz;
  char fontName[500], fontSize[100];
  Unicode u;
  char buf;
  int i;

  // compute size and normalized transform matrix
  size = sqrt(m21*m21 + m22*m22);
  ntm11 = m11 / size;
  ntm12 = -m12 / size;
  ntm21 = m21 / size;
  ntm22 = -m22 / size;

  // try to get a rotated font?
  rotated = !(ntm11 > 0 && ntm22 > 0 &&
	      fabs(ntm11 / ntm22 - 1) < 0.2 &&
	      fabs(ntm12) < 0.01 &&
	      fabs(ntm21) < 0.01);

  // open X font -- if font is not found (which means the server can't
  // scale fonts), try progressively smaller and then larger sizes
  startSize = (int)size;
  if (rotated) {
    sprintf(fontSize, "[%s%0.2f %s%0.2f %s%0.2f %s%0.2f]",
	    ntm11<0 ? "~" : "", fabs(ntm11 * size),
	    ntm12<0 ? "~" : "", fabs(ntm12 * size),
	    ntm21<0 ? "~" : "", fabs(ntm21 * size),
	    ntm22<0 ? "~" : "", fabs(ntm22 * size));
  } else {
    sprintf(fontSize, "%d", startSize);
  }
  snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
  xFont = XLoadQueryFont(display, fontName);
  if (!xFont) {
    for (sz = startSize; sz >= startSize/2 && sz >= 1; --sz) {
      sprintf(fontSize, "%d", sz);
      snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
      if ((xFont = XLoadQueryFont(display, fontName)))
	break;
    }
    if (!xFont) {
      for (sz = startSize + 1; sz < startSize + 10; ++sz) {
	sprintf(fontSize, "%d", sz);
	snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
	if ((xFont = XLoadQueryFont(display, fontName))) {
	  break;
	}
      }
      if (!xFont) {
	sprintf(fontSize, "%d", startSize);
	snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
	error(-1, "Failed to open font: '%s'", fontName);
	return;
      }
    }
  }

  // Construct char code map.
  xUMap = xUMapA;
  for (i = 0; i < 256; ++i) {
    if (fontUMap->mapToUnicode((CID)i, &u, 1) == 1 &&
	xUMap->mapUnicode(u, &buf, 1) == 1) {
      map[i] = buf & 0xff;
    } else {
      map[i] = 0;
    }
  }
}

XOutputServer8BitFont::~XOutputServer8BitFont() {
  if (xFont) {
    XFreeFont(display, xFont);
  }
}

GBool XOutputServer8BitFont::isOk() {
  return xFont != NULL;
}

void XOutputServer8BitFont::updateGC(GC gc) {
  XSetFont(display, gc, xFont->fid);
}

void XOutputServer8BitFont::drawChar(GfxState *state, Pixmap pixmap,
				     int w, int h, GC gc,
				     double x, double y, double dx, double dy,
				     CharCode c, Unicode *u, int uLen) {
  Gushort c1;
  char buf[8];
  double dx1, dy1;
  int m, n, i, j, k;

  c1 = map[c];
  if (c1 > 0) {
    buf[0] = (char)c1;
    XDrawString(display, pixmap, gc, xoutRound(x), xoutRound(y), buf, 1);
  } else {
    // substituted character, using more than one character
    n = 0;
    for (i = 0; i < uLen; ++i) {
      n += xUMap->mapUnicode(u[i], buf, sizeof(buf));
    }
    dx1 = dx / n;
    dy1 = dy / n;
    k = 0;
    for (i = 0; i < uLen; ++i) {
      m = xUMap->mapUnicode(u[i], buf, sizeof(buf));
      for (j = 0; j < m; ++j) {
	XDrawString(display, pixmap, gc,
		    xoutRound(x + k*dx1), xoutRound(y + k*dy1),
		    buf + j, 1);
	++k;
      }
    }
  }
}

//------------------------------------------------------------------------
// XOutputServer16BitFont
//------------------------------------------------------------------------

XOutputServer16BitFont::XOutputServer16BitFont(Ref *idA, GString *xlfdFmt, 
					       UnicodeMap *xUMapA,
					       CharCodeToUnicode *fontUMap,
					       double m11OrigA,
					       double m12OrigA,
					       double m21OrigA,
					       double m22OrigA,
					       double m11A, double m12A,
					       double m21A, double m22A,
					       Display *displayA):
  XOutputFont(idA, m11OrigA, m12OrigA, m21OrigA, m22OrigA,
	      m11A, m12A, m21A, m22A, displayA)
{
  double size, ntm11, ntm12, ntm21, ntm22;
  GBool rotated;
  int startSize, sz;
  char fontName[500], fontSize[100];

  xUMap = xUMapA;
  xUMap->incRefCnt();

  // compute size and normalized transform matrix
  size = sqrt(m21*m21 + m22*m22);
  ntm11 = m11 / size;
  ntm12 = -m12 / size;
  ntm21 = m21 / size;
  ntm22 = -m22 / size;

  // try to get a rotated font?
  rotated = !(ntm11 > 0 && ntm22 > 0 &&
	      fabs(ntm11 / ntm22 - 1) < 0.2 &&
	      fabs(ntm12) < 0.01 &&
	      fabs(ntm21) < 0.01);

  // open X font -- if font is not found (which means the server can't
  // scale fonts), try progressively smaller and then larger sizes
  startSize = (int)size;
  if (rotated) {
    sprintf(fontSize, "[%s%0.2f %s%0.2f %s%0.2f %s%0.2f]",
	    ntm11<0 ? "~" : "", fabs(ntm11 * size),
	    ntm12<0 ? "~" : "", fabs(ntm12 * size),
	    ntm21<0 ? "~" : "", fabs(ntm21 * size),
	    ntm22<0 ? "~" : "", fabs(ntm22 * size));
  } else {
    sprintf(fontSize, "%d", startSize);
  }
  snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
  xFont = XLoadQueryFont(display, fontName);
  if (!xFont) {
    for (sz = startSize; sz >= startSize/2 && sz >= 1; --sz) {
      sprintf(fontSize, "%d", sz);
      snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
      if ((xFont = XLoadQueryFont(display, fontName)))
	break;
    }
    if (!xFont) {
      for (sz = startSize + 1; sz < startSize + 10; ++sz) {
	sprintf(fontSize, "%d", sz);
	snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
	if ((xFont = XLoadQueryFont(display, fontName))) {
	  break;
	}
      }
      if (!xFont) {
	sprintf(fontSize, "%d", startSize);
	snprintf(fontName, sizeof(fontName), xlfdFmt->getCString(), fontSize);
	error(-1, "Failed to open font: '%s'", fontName);
	return;
      }
    }
  }
}

XOutputServer16BitFont::~XOutputServer16BitFont() {
  xUMap->decRefCnt();
  if (xFont) {
    XFreeFont(display, xFont);
  }
}

GBool XOutputServer16BitFont::isOk() {
  return xFont != NULL;
}

void XOutputServer16BitFont::updateGC(GC gc) {
  XSetFont(display, gc, xFont->fid);
}

void XOutputServer16BitFont::drawChar(GfxState *state, Pixmap pixmap,
				      int w, int h, GC gc,
				      double x, double y, double dx, double dy,
				      CharCode c, Unicode *u, int uLen) {
  char buf[16];
  XChar2b c1;
  double dx1, dy1;
  int m, n, i, j, k;

  n = 0;
  for (i = 0; i < uLen; ++i) {
    n += xUMap->mapUnicode(u[i], buf, sizeof(buf));
  }
  if (n > 0) {
    dx1 = dx / n;
    dy1 = dy / n;
    k = 0;
    for (i = 0; i < uLen; ++i) {
      m = xUMap->mapUnicode(u[i], buf, sizeof(buf));
      for (j = 0; j+1 < m; j += 2) {
	c1.byte1 = buf[j];
	c1.byte2 = buf[j+1];
	XDrawString16(display, pixmap, gc,
		      xoutRound(x + k*dx1), xoutRound(y + k*dy1),
		      &c1, 1);
	++k;
      }
    }
  } else if (c != 0) {
    // some PDF files use CID 0, which is .notdef, so just ignore it
    error(-1, "Unknown character (CID=%d Unicode=%04x)",
	  c, uLen > 0 ? u[0] : (Unicode)0);
  }
}

//------------------------------------------------------------------------
// XOutputFontCache
//------------------------------------------------------------------------

#if HAVE_T1LIB_H
XOutputT1FontFile::~XOutputT1FontFile() {
  delete fontFile;
}
#endif

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
XOutputFTFontFile::~XOutputFTFontFile() {
  delete fontFile;
}
#endif

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
XOutputTTFontFile::~XOutputTTFontFile() {
  delete fontFile;
}
#endif

XOutputFontCache::XOutputFontCache(Display *displayA, Guint depthA,
				   FontRastControl t1libControlA,
				   FontRastControl freetypeControlA) {
  display = displayA;
  depth = depthA;

#if HAVE_T1LIB_H
  t1Engine = NULL;
  t1libControl = t1libControlA;
#endif

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  ftEngine = NULL;
#endif
#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  ttEngine = NULL;
#endif
#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
  freetypeControl = freetypeControlA;
#endif

  clear();
}

XOutputFontCache::~XOutputFontCache() {
  delFonts();
}

void XOutputFontCache::startDoc(int screenNum, Colormap colormap,
				GBool trueColor,
				int rMul, int gMul, int bMul,
				int rShift, int gShift, int bShift,
				Gulong *colors, int numColors) {
  delFonts();
  clear();

#if HAVE_T1LIB_H
  if (t1libControl != fontRastNone) {
    t1Engine = new T1FontEngine(display, DefaultVisual(display, screenNum),
				depth, colormap,
				t1libControl == fontRastAALow ||
				  t1libControl == fontRastAAHigh,
				t1libControl == fontRastAAHigh);
    if (t1Engine->isOk()) {
      if (trueColor) {
	t1Engine->useTrueColor(rMul, rShift, gMul, gShift, bMul, bShift);
      } else {
	t1Engine->useColorCube(colors, numColors);
      }
    } else {
      delete t1Engine;
      t1Engine = NULL;
    }
  }
#endif // HAVE_T1LIB_H

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  if (freetypeControl != fontRastNone) {
    ftEngine = new FTFontEngine(display, DefaultVisual(display, screenNum),
				depth, colormap,
				freetypeControl == fontRastAALow ||
				  freetypeControl == fontRastAAHigh);
    if (ftEngine->isOk()) {
      if (trueColor) {
	ftEngine->useTrueColor(rMul, rShift, gMul, gShift, bMul, bShift);
      } else {
	ftEngine->useColorCube(colors, numColors);
      }
    } else {
      delete ftEngine;
      ftEngine = NULL;
    }
  }
#endif // FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  if (freetypeControl != fontRastNone) {
    ttEngine = new TTFontEngine(display, DefaultVisual(display, screenNum),
				depth, colormap,
				freetypeControl == fontRastAALow ||
				  freetypeControl == fontRastAAHigh);
    if (ttEngine->isOk()) {
      if (trueColor) {
	ttEngine->useTrueColor(rMul, rShift, gMul, gShift, bMul, bShift);
      } else {
	ttEngine->useColorCube(colors, numColors);
      }
    } else {
      delete ttEngine;
      ttEngine = NULL;
    }
  }
#endif // !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
}

void XOutputFontCache::delFonts() {
  int i;

  for (i = 0; i < nFonts; ++i) {
    delete fonts[i];
  }

#if HAVE_T1LIB_H
  // delete Type 1 font files
  deleteGList(t1FontFiles, XOutputT1FontFile);
  if (t1Engine) {
    delete t1Engine;
  }
#endif

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  // delete FreeType font files
  deleteGList(ftFontFiles, XOutputFTFontFile);
  if (ftEngine) {
    delete ftEngine;
  }
#endif

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  // delete TrueType fonts
  deleteGList(ttFontFiles, XOutputTTFontFile);
  if (ttEngine) {
    delete ttEngine;
  }
#endif
}

void XOutputFontCache::clear() {
  int i;

  for (i = 0; i < xOutFontCacheSize; ++i) {
    fonts[i] = NULL;
  }
  nFonts = 0;

#if HAVE_T1LIB_H
  // clear Type 1 font files
  t1FontFiles = new GList();
#endif

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  // clear FreeType font cache
  ftFontFiles = new GList();
#endif

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  // clear TrueType font cache
  ttFontFiles = new GList();
#endif
}

XOutputFont *XOutputFontCache::getFont(XRef *xref, GfxFont *gfxFont,
				       double m11, double m12,
				       double m21, double m22) {
  XOutputFont *font;
  DisplayFontParam *dfp;
  GString *substName;
  double m11New, m12New, m21New, m22New;
  double w1, w2, v;
  double *fm;
  char *name;
  int index;
  int code;
  int i, j;

  // is it the most recently used font?
  if (nFonts > 0 && fonts[0]->matches(gfxFont->getID(), m11, m12, m21, m22)) {
    return fonts[0];
  }

  // is it in the cache?
  for (i = 1; i < nFonts; ++i) {
    if (fonts[i]->matches(gfxFont->getID(), m11, m12, m21, m22)) {
      font = fonts[i];
      for (j = i; j > 0; --j) {
	fonts[j] = fonts[j-1];
      }
      fonts[0] = font;
      return font;
    }
  }

  // try for a cached FontFile, an embedded font, or an external font
  // file
  font = NULL;
  switch (gfxFont->getType()) {
  case fontType1:
  case fontType1C:
#if HAVE_T1LIB_H
    if (t1libControl != fontRastNone) {
      font = tryGetT1Font(xref, gfxFont, m11, m12, m21, m22);
    }
#endif
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (!font) {
      if (freetypeControl != fontRastNone) {
	font = tryGetFTFont(xref, gfxFont, m11, m12, m21, m22);
      }
    }
#endif
    break;
  case fontTrueType:
  case fontCIDType2:
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (freetypeControl != fontRastNone) {
      font = tryGetFTFont(xref, gfxFont, m11, m12, m21, m22);
    }
#endif
#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (freetypeControl != fontRastNone) {
      font = tryGetTTFont(xref, gfxFont, m11, m12, m21, m22);
    }
#endif
    break;
  case fontCIDType0C:
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (freetypeControl != fontRastNone) {
      font = tryGetFTFont(xref, gfxFont, m11, m12, m21, m22);
    }
#endif
    break;
  default:
    break;
  }

  if (!font) {

    // search for a display font mapping
    dfp = NULL;
    if (gfxFont->isCIDFont()) {
      if (((GfxCIDFont *)gfxFont)->getCollection()) {
	dfp = globalParams->
	        getDisplayCIDFont(((GfxCIDFont *)gfxFont)->getCollection());
      } else {
	// this error (no CMap file) was already reported by GfxFont
	return NULL;
      }
    } else {
      if (gfxFont->getName()) {
	dfp = globalParams->getDisplayFont(gfxFont->getName());
      }
    }
    if (dfp) {
      font = tryGetFont(xref, dfp, gfxFont, m11, m12, m21, m22,
			m11, m12, m21, m22, gFalse);
    }

    // substitute a font (8-bit fonts only)
    if (!font && !gfxFont->isCIDFont()) {

      // choose a substitute font
      if (gfxFont->isFixedWidth()) {
	index = 8;
      } else if (gfxFont->isSerif()) {
	index = 4;
      } else {
	index = 0;
      }
      if (gfxFont->isBold()) {
	index += 2;
      }
      if (gfxFont->isItalic()) {
	index += 1;
      }
      substName = new GString(xOutSubstFonts[index].name);

      // adjust the font matrix -- compare the width of 'm' in the
      // original font and the substituted font
      m11New = m11;
      m12New = m12;
      m21New = m21;
      m22New = m22;
      for (code = 0; code < 256; ++code) {
	if ((name = ((Gfx8BitFont *)gfxFont)->getCharName(code)) &&
	    name[0] == 'm' && name[1] == '\0') {
	  break;
	}
      }
      if (code < 256) {
	w1 = ((Gfx8BitFont *)gfxFont)->getWidth(code);
	w2 = xOutSubstFonts[index].mWidth;
	if (gfxFont->getType() == fontType3) {
	  // This is a hack which makes it possible to substitute for some
	  // Type 3 fonts.  The problem is that it's impossible to know what
	  // the base coordinate system used in the font is without actually
	  // rendering the font.  This code tries to guess by looking at the
	  // width of the character 'm' (which breaks if the font is a
	  // subset that doesn't contain 'm').
	  if (w1 > 0 && (w1 > 1.1 * w2 || w1 < 0.9 * w2)) {
	    w1 /= w2;
	    m11New *= w1;
	    m12New *= w1;
	    m21New *= w1;
	    m22New *= w1;
	  }
	  fm = gfxFont->getFontMatrix();
	  v = (fm[0] == 0) ? 1 : (fm[3] / fm[0]);
	  m21New *= v;
	  m22New *= v;
	} else if (!gfxFont->isSymbolic()) {
	  // if real font is substantially narrower than substituted
	  // font, reduce the font size accordingly
	  if (w1 > 0.01 && w1 < 0.9 * w2) {
	    w1 /= w2;
	    m11New *= w1;
	    m21New *= w1;
	  }
	}
      }

      // get the font
      dfp = globalParams->getDisplayFont(substName);
      delete substName;
      if (!dfp) {
	// this should never happen since GlobalParams sets up default
	// mappings for the Base-14 fonts
	error(-1, "Couldn't find a font for '%s'",
	      gfxFont->getName()->getCString());
	return NULL;
      }
      font = tryGetFont(xref, dfp, gfxFont, m11, m12, m21, m22,
			m11New, m12New, m21New, m22New, gTrue);
    }
  }

  // check for error
  if (!font) {
    // This will happen if the user specifies a bogus font in the
    // config file (a non-existent font file or a font that requires a
    // rasterizer that is disabled or wasn't built in), or if a CID
    // font has no associated font in the config file.
    if (gfxFont->isCIDFont()) {
      error(-1, "Couldn't find a font for the '%s' character collection",
	    ((GfxCIDFont *)gfxFont)->getCollection()->getCString());
    } else {
      error(-1, "Couldn't find a font for '%s'",
	    gfxFont->getName()->getCString());
    }
    return NULL;
  }

  // insert font in cache
  if (nFonts == xOutFontCacheSize) {
    --nFonts;
    delete fonts[nFonts];
  }
  for (j = nFonts; j > 0; --j) {
    fonts[j] = fonts[j-1];
  }
  fonts[0] = font;
  ++nFonts;

  return font;
}

XOutputFont *XOutputFontCache::tryGetFont(XRef *xref, DisplayFontParam *dfp,
					  GfxFont *gfxFont,
					  double m11Orig, double m12Orig,
					  double m21Orig, double m22Orig,
					  double m11, double m12,
					  double m21, double m22,
					  GBool subst) {
  XOutputFont *font;

  font = NULL;

  // create the new font
  switch (dfp->kind) {

  case displayFontX:
    font = tryGetServerFont(dfp->x.xlfd, dfp->x.encoding, gfxFont,
			    m11Orig, m12Orig, m21Orig, m22Orig,
			    m11, m12, m21, m22);
    break;

  case displayFontT1:
#if HAVE_T1LIB_H
    if (t1libControl != fontRastNone) {
      font = tryGetT1FontFromFile(xref, dfp->t1.fileName, gfxFont,
				  m11Orig, m12Orig, m21Orig, m22Orig,
				  m11, m12, m21, m22, subst);
    }
#endif
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (!font) {
      if (freetypeControl != fontRastNone) {
	font = tryGetFTFontFromFile(xref, dfp->t1.fileName, gfxFont,
				    m11Orig, m12Orig, m21Orig, m22Orig,
				    m11, m12, m21, m22, subst);
      }
    }
#endif
#if !((FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)) || defined(HAVE_T1LIB_H))
    error(-1, "Config file specifies a Type 1 font,");
    error(-1, "but xpdf was not built with t1lib or FreeType2 support");
#endif
    break;

  case displayFontTT:
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (freetypeControl != fontRastNone) {
      font = tryGetFTFontFromFile(xref, dfp->tt.fileName, gfxFont,
				  m11Orig, m12Orig, m21Orig, m22Orig,
				  m11, m12, m21, m22, subst);
    }
#endif
#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    if (freetypeControl != fontRastNone) {
      font = tryGetTTFontFromFile(xref, dfp->tt.fileName, gfxFont,
				  m11Orig, m12Orig, m21Orig, m22Orig,
				  m11, m12, m21, m22, subst);
    }
#endif
#if !(HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
    error(-1, "Config file specifies a TrueType font,");
    error(-1, "but xpdf was not built with FreeType support");
    dfp = NULL;
#endif
    break;
  }

  return font;
}

#if HAVE_T1LIB_H
XOutputFont *XOutputFontCache::tryGetT1Font(XRef *xref,
					    GfxFont *gfxFont,
					    double m11, double m12,
					    double m21, double m22) {
  Ref *id;
  XOutputT1FontFile *xFontFile;
  XOutputFont *font;
  Ref embRef;
  GString *fileName;
  FILE *f;
  char *fontBuf;
  int fontLen;
  Type1CFontConverter *cvt;
  Object refObj, strObj;
  int c;
  int i;

  // check the already available font files
  id = gfxFont->getID();
  for (i = 0; i < t1FontFiles->getLength(); ++i) {
    xFontFile = (XOutputT1FontFile *)t1FontFiles->get(i);
    if (xFontFile->num == id->num && xFontFile->gen == id->gen &&
	!xFontFile->subst) {
      font = new XOutputT1Font(id, xFontFile->fontFile,
			       m11, m12, m21, m22,
			       m11, m12, m21, m22, display);
      if (!font->isOk()) {
	delete font;
	return NULL;
      }
      return font;
    }
  }

  // check for an embedded font
  if (gfxFont->getEmbeddedFontID(&embRef)) {

    // create the font file
    fileName = NULL;
    if (!openTempFile(&fileName, &f, "wb", NULL)) {
      error(-1, "Couldn't create temporary Type 1 font file");
      return NULL;
    }
    if (gfxFont->getType() == fontType1C) {
      if (!(fontBuf = gfxFont->readEmbFontFile(xref, &fontLen))) {
	fclose(f);
	return NULL;
      }
      cvt = new Type1CFontConverter(fontBuf, fontLen, f);
      cvt->convert();
      delete cvt;
      gfree(fontBuf);
    } else { // fontType1
      refObj.initRef(embRef.num, embRef.gen);
      refObj.fetch(xref, &strObj);
      refObj.free();
      strObj.streamReset();
      while ((c = strObj.streamGetChar()) != EOF) {
	fputc(c, f);
      }
      strObj.streamClose();
      strObj.free();
    }
    fclose(f);

    // create the Font
    font = tryGetT1FontFromFile(xref, fileName, gfxFont,
				m11, m12, m21, m22,
				m11, m12, m21, m22, gFalse);

    // remove the temporary file
    unlink(fileName->getCString());
    delete fileName;

  // check for an external font file
  } else if ((fileName = gfxFont->getExtFontFile())) {
    font = tryGetT1FontFromFile(xref, fileName, gfxFont,
				m11, m12, m21, m22,
				m11, m12, m21, m22, gFalse);

  } else {
    font = NULL;
  }

  return font;
}

XOutputFont *XOutputFontCache::tryGetT1FontFromFile(XRef *xref,
						    GString *fileName,
						    GfxFont *gfxFont,
						    double m11Orig,
						    double m12Orig,
						    double m21Orig,
						    double m22Orig,
						    double m11, double m12,
						    double m21, double m22,
						    GBool subst) {
  Ref *id;
  T1FontFile *fontFile;
  XOutputFont *font;

  // create the t1lib font file
  fontFile = new T1FontFile(t1Engine, fileName->getCString(),
			    ((Gfx8BitFont *)gfxFont)->getEncoding(),
			    gfxFont->getFontBBox());
  if (!fontFile->isOk()) {
    error(-1, "Couldn't create t1lib font from '%s'",
	  fileName->getCString());
    delete fontFile;
    return NULL;
  }

  // add to list
  id = gfxFont->getID();
  t1FontFiles->append(new XOutputT1FontFile(id->num, id->gen,
					    subst, fontFile));

  // create the Font
  font = new XOutputT1Font(gfxFont->getID(), fontFile,
			   m11Orig, m12Orig, m21Orig, m22Orig,
			   m11, m12, m21, m22, display);
  if (!font->isOk()) {
    delete font;
    return NULL;
  }
  return font;
}
#endif // HAVE_T1LIB_H

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
XOutputFont *XOutputFontCache::tryGetFTFont(XRef *xref,
					    GfxFont *gfxFont,
					    double m11, double m12,
					    double m21, double m22) {
  Ref *id;
  XOutputFTFontFile *xFontFile;
  XOutputFont *font;
  Ref embRef;
  GString *fileName;
  FILE *f;
#if 1 //~ need this until FT can handle fonts with missing tables
  char *fontBuf;
  int fontLen;
  TrueTypeFontFile *ff;
#endif
  Object refObj, strObj;
  int c;
  int i;

  // check the already available font files
  id = gfxFont->getID();
  for (i = 0; i < ftFontFiles->getLength(); ++i) {
    xFontFile = (XOutputFTFontFile *)ftFontFiles->get(i);
    if (xFontFile->num == id->num && xFontFile->gen == id->gen &&
	!xFontFile->subst) {
      font = new XOutputFTFont(id, xFontFile->fontFile,
			       m11, m12, m21, m22,
			       m11, m12, m21, m22, display);
      if (!font->isOk()) {
	delete font;
	return NULL;
      }
      return font;
    }
  }

  // check for an embedded font
  if (gfxFont->getEmbeddedFontID(&embRef)) {

    // create the font file
    fileName = NULL;
    if (!openTempFile(&fileName, &f, "wb", NULL)) {
      error(-1, "Couldn't create temporary TrueType font file");
      return NULL;
    }
#if 1 //~ need this until FT can handle fonts with missing tables
    if (gfxFont->getType() == fontTrueType ||
	gfxFont->getType() == fontCIDType2) {
      if (!(fontBuf = gfxFont->readEmbFontFile(xref, &fontLen))) {
	fclose(f);
	return NULL;
      }
      ff = new TrueTypeFontFile(fontBuf, fontLen);
      ff->writeTTF(f);
      delete ff;
      gfree(fontBuf);
    } else {
      refObj.initRef(embRef.num, embRef.gen);
      refObj.fetch(xref, &strObj);
      refObj.free();
      strObj.streamReset();
      while ((c = strObj.streamGetChar()) != EOF) {
	fputc(c, f);
      }
      strObj.streamClose();
      strObj.free();
    }
#else
    refObj.initRef(embRef.num, embRef.gen);
    refObj.fetch(xref, &strObj);
    refObj.free();
    strObj.streamReset();
    while ((c = strObj.streamGetChar()) != EOF) {
      fputc(c, f);
    }
    strObj.streamClose();
    strObj.free();
#endif
    fclose(f);

    // create the Font
    font = tryGetFTFontFromFile(xref, fileName, gfxFont,
				m11, m12, m21, m22,
				m11, m12, m21, m22, gFalse);

    // remove the temporary file
    unlink(fileName->getCString());
    delete fileName;

  // check for an external font file
  } else if ((fileName = gfxFont->getExtFontFile())) {
    font = tryGetFTFontFromFile(xref, fileName, gfxFont,
				m11, m12, m21, m22,
				m11, m12, m21, m22, gFalse);

  } else {
    font = NULL;
  }

  return font;
}

XOutputFont *XOutputFontCache::tryGetFTFontFromFile(XRef *xref,
						    GString *fileName,
						    GfxFont *gfxFont,
						    double m11Orig,
						    double m12Orig,
						    double m21Orig,
						    double m22Orig,
						    double m11, double m12,
						    double m21, double m22,
						    GBool subst) {
  Ref *id;
  FTFontFile *fontFile;
  XOutputFont *font;

  // create the FreeType font file
  if (gfxFont->isCIDFont()) {
    if (gfxFont->getType() == fontCIDType2) {
      fontFile = new FTFontFile(ftEngine, fileName->getCString(),
				((GfxCIDFont *)gfxFont)->getCIDToGID(),
				((GfxCIDFont *)gfxFont)->getCIDToGIDLen());
    } else { // fontCIDType0C
      fontFile = new FTFontFile(ftEngine, fileName->getCString());
    }
  } else {
    fontFile = new FTFontFile(ftEngine, fileName->getCString(),
			      ((Gfx8BitFont *)gfxFont)->getEncoding(),
			      ((Gfx8BitFont *)gfxFont)->getHasEncoding());
  }
  if (!fontFile->isOk()) {
    error(-1, "Couldn't create FreeType font from '%s'",
	  fileName->getCString());
    delete fontFile;
    return NULL;
  }

  // add to list
  id = gfxFont->getID();
  ftFontFiles->append(new XOutputFTFontFile(id->num, id->gen,
					    subst, fontFile));

  // create the Font
  font = new XOutputFTFont(gfxFont->getID(), fontFile,
			   m11Orig, m12Orig, m21Orig, m22Orig,
			   m11, m12, m21, m22, display);
  if (!font->isOk()) {
    delete font;
    return NULL;
  }
  return font;
}
#endif // FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
XOutputFont *XOutputFontCache::tryGetTTFont(XRef *xref,
					    GfxFont *gfxFont,
					    double m11, double m12,
					    double m21, double m22) {
  Ref *id;
  XOutputTTFontFile *xFontFile;
  XOutputFont *font;
  Ref embRef;
  GString *fileName;
  FILE *f;
  Object refObj, strObj;
  int c;
  int i;

  // check the already available font files
  id = gfxFont->getID();
  xFontFile = NULL;
  for (i = 0; i < ttFontFiles->getLength(); ++i) {
    xFontFile = (XOutputTTFontFile *)ttFontFiles->get(i);
    if (xFontFile->num == id->num && xFontFile->gen == id->gen &&
	!xFontFile->subst) {
      font = new XOutputTTFont(id, xFontFile->fontFile,
			       m11, m12, m21, m22,
			       m11, m12, m21, m22, display);
      if (!font->isOk()) {
	delete font;
	return NULL;
      }
      return font;
    }
  }

  // check for an embedded font
  if (gfxFont->getEmbeddedFontID(&embRef)) {

    // create the font file
    fileName = NULL;
    if (!openTempFile(&fileName, &f, "wb", NULL)) {
      error(-1, "Couldn't create temporary TrueType font file");
      return NULL;
    }
    refObj.initRef(embRef.num, embRef.gen);
    refObj.fetch(xref, &strObj);
    refObj.free();
    strObj.streamReset();
    while ((c = strObj.streamGetChar()) != EOF) {
      fputc(c, f);
    }
    strObj.streamClose();
    strObj.free();
    fclose(f);

    // create the Font
    font = tryGetTTFontFromFile(xref, fileName, gfxFont,
				m11, m12, m21, m22,
				m11, m12, m21, m22, gFalse);

    // remove the temporary file
    unlink(fileName->getCString());
    delete fileName;

  } else if ((fileName = gfxFont->getExtFontFile())) {
    font = tryGetTTFontFromFile(xref, fileName, gfxFont,
				m11, m12, m21, m22,
				m11, m12, m21, m22, gFalse);

  } else {
    font = NULL;
  }

  return font;
}

XOutputFont *XOutputFontCache::tryGetTTFontFromFile(XRef *xref,
						    GString *fileName,
						    GfxFont *gfxFont,
						    double m11Orig,
						    double m12Orig,
						    double m21Orig,
						    double m22Orig,
						    double m11, double m12,
						    double m21, double m22,
						    GBool subst) {
  Ref *id;
  TTFontFile *fontFile;
  XOutputFont *font;

  // create the FreeType font file
  if (gfxFont->isCIDFont()) {
    // fontCIDType2
    fontFile = new TTFontFile(ttEngine, fileName->getCString(),
			      ((GfxCIDFont *)gfxFont)->getCIDToGID(),
			      ((GfxCIDFont *)gfxFont)->getCIDToGIDLen());
  } else {
    fontFile = new TTFontFile(ttEngine, fileName->getCString(),
			      ((Gfx8BitFont *)gfxFont)->getEncoding(),
			      ((Gfx8BitFont *)gfxFont)->getHasEncoding());
  }
  if (!fontFile->isOk()) {
    error(-1, "Couldn't create FreeType font from '%s'",
	  fileName->getCString());
    delete fontFile;
    return NULL;
  }

  // add to list
  id = gfxFont->getID();
  ttFontFiles->append(new XOutputTTFontFile(id->num, id->gen,
					    subst, fontFile));

  // create the Font
  font = new XOutputTTFont(gfxFont->getID(), fontFile,
			   m11Orig, m12Orig, m21Orig, m22Orig,
			   m11, m12, m21, m22, display);
  if (!font->isOk()) {
    delete font;
    return NULL;
  }
  return font;
}
#endif // !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

XOutputFont *XOutputFontCache::tryGetServerFont(GString *xlfd,
						GString *encodingName,
						GfxFont *gfxFont,
						double m11Orig, double m12Orig,
						double m21Orig, double m22Orig,
						double m11, double m12,
						double m21, double m22) {
  XOutputFont *font;
  UnicodeMap *uMap;
  CharCodeToUnicode *ctu;

  uMap = globalParams->getUnicodeMap(encodingName);
  if (gfxFont->isCIDFont()) {
    ctu = ((GfxCIDFont *)gfxFont)->getToUnicode();
    font = new XOutputServer16BitFont(gfxFont->getID(), xlfd, uMap, ctu,
				      m11Orig, m12Orig, m21Orig, m22Orig,
				      m11, m12, m21, m22, display);
    ctu->decRefCnt();
  } else {
    ctu = ((Gfx8BitFont *)gfxFont)->getToUnicode();
    font = new XOutputServer8BitFont(gfxFont->getID(), xlfd, uMap, ctu,
				     m11Orig, m12Orig, m21Orig, m22Orig,
				     m11, m12, m21, m22, display);
    ctu->decRefCnt();
  }
  uMap->decRefCnt();
  if (!font->isOk()) {
    delete font;
    return NULL;
  }
  return font;
}

//------------------------------------------------------------------------
// XOutputDev
//------------------------------------------------------------------------

XOutputDev::XOutputDev(Display *displayA, Pixmap pixmapA, Guint depthA,
		       Colormap colormapA, unsigned long paperColor,
		       GBool installCmap, int rgbCubeSize) {
  XVisualInfo visualTempl;
  XVisualInfo *visualList;
  int nVisuals;
  Gulong mask;
  XGCValues gcValues;
  XColor xcolor;
  XColor *xcolors;
  int r, g, b, n, m, i;
  GBool ok;

  xref = NULL;

  // get display/pixmap info
  display = displayA;
  screenNum = DefaultScreen(display);
  pixmap = pixmapA;
  depth = depthA;
  colormap = colormapA;

  // check for TrueColor visual
  trueColor = gFalse;
  if (depth == 0) {
    depth = DefaultDepth(display, screenNum);
    visualList = XGetVisualInfo(display, 0, &visualTempl, &nVisuals);
    for (i = 0; i < nVisuals; ++i) {
      if (visualList[i].visual == DefaultVisual(display, screenNum)) {
	if (visualList[i].c_class == TrueColor) {
	  trueColor = gTrue;
	  for (mask = visualList[i].red_mask, rShift = 0;
	       mask && !(mask & 1);
	       mask >>= 1, ++rShift) ;
	  rMul = (int)mask;
	  for (mask = visualList[i].green_mask, gShift = 0;
	       mask && !(mask & 1);
	       mask >>= 1, ++gShift) ;
	  gMul = (int)mask;
	  for (mask = visualList[i].blue_mask, bShift = 0;
	       mask && !(mask & 1);
	       mask >>= 1, ++bShift) ;
	  bMul = (int)mask;
	}
	break;
      }
    }
    XFree((XPointer)visualList);
  }

  // allocate a color cube
  if (!trueColor) {

    // set colors in private colormap
    if (installCmap) {
      for (numColors = 6; numColors >= 2; --numColors) {
	m = numColors * numColors * numColors;
	if (XAllocColorCells(display, colormap, False, NULL, 0, colors, m))
	  break;
      }
      if (numColors >= 2) {
	m = numColors * numColors * numColors;
	xcolors = (XColor *)gmalloc(m * sizeof(XColor));
	n = 0;
	for (r = 0; r < numColors; ++r) {
	  for (g = 0; g < numColors; ++g) {
	    for (b = 0; b < numColors; ++b) {
	      xcolors[n].pixel = colors[n];
	      xcolors[n].red = (r * 65535) / (numColors - 1);
	      xcolors[n].green = (g * 65535) / (numColors - 1);
	      xcolors[n].blue = (b * 65535) / (numColors - 1);
	      xcolors[n].flags = DoRed | DoGreen | DoBlue;
	      ++n;
	    }
	  }
	}
	XStoreColors(display, colormap, xcolors, m);
	gfree(xcolors);
      } else {
	numColors = 1;
	colors[0] = BlackPixel(display, screenNum);
	colors[1] = WhitePixel(display, screenNum);
      }

    // allocate colors in shared colormap
    } else {
      if (rgbCubeSize > maxRGBCube)
	rgbCubeSize = maxRGBCube;
      ok = gFalse;
      for (numColors = rgbCubeSize; numColors >= 2; --numColors) {
	ok = gTrue;
	n = 0;
	for (r = 0; r < numColors && ok; ++r) {
	  for (g = 0; g < numColors && ok; ++g) {
	    for (b = 0; b < numColors && ok; ++b) {
	      if (n == 0) {
		colors[n++] = BlackPixel(display, screenNum);
	      } else {
		xcolor.red = (r * 65535) / (numColors - 1);
		xcolor.green = (g * 65535) / (numColors - 1);
		xcolor.blue = (b * 65535) / (numColors - 1);
		if (XAllocColor(display, colormap, &xcolor))
		  colors[n++] = xcolor.pixel;
		else
		  ok = gFalse;
	      }
	    }
	  }
	}
	if (ok)
	  break;
	XFreeColors(display, colormap, &colors[1], n-1, 0);
      }
      if (!ok) {
	numColors = 1;
	colors[0] = BlackPixel(display, screenNum);
	colors[1] = WhitePixel(display, screenNum);
      }
    }
  }

  // allocate GCs
  gcValues.foreground = BlackPixel(display, screenNum);
  gcValues.background = WhitePixel(display, screenNum);
  gcValues.line_width = 0;
  gcValues.line_style = LineSolid;
  strokeGC = XCreateGC(display, pixmap,
		       GCForeground | GCBackground | GCLineWidth | GCLineStyle,
                       &gcValues);
  fillGC = XCreateGC(display, pixmap,
		     GCForeground | GCBackground | GCLineWidth | GCLineStyle,
		     &gcValues);
  gcValues.foreground = paperColor;
  paperGC = XCreateGC(display, pixmap,
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle,
		      &gcValues);

  // no clip region yet
  clipRegion = NULL;

  // set up the font cache and fonts
  gfxFont = NULL;
  font = NULL;
  fontCache = new XOutputFontCache(display, depth,
				   globalParams->getT1libControl(),
				   globalParams->getFreeTypeControl());

  // empty state stack
  save = NULL;

  // create text object
  text = new TextPage(gFalse);

  type3Warning = gFalse;
}

XOutputDev::~XOutputDev() {
  delete fontCache;
  XFreeGC(display, strokeGC);
  XFreeGC(display, fillGC);
  XFreeGC(display, paperGC);
  if (clipRegion) {
    XDestroyRegion(clipRegion);
  }
  delete text;
}

void XOutputDev::startDoc(XRef *xrefA) {
  xref = xrefA;
  fontCache->startDoc(screenNum, colormap, trueColor, rMul, gMul, bMul,
		      rShift, gShift, bShift, colors, numColors);
}

void XOutputDev::startPage(int pageNum, GfxState *state) {
  XOutputState *s;
  XGCValues gcValues;
  XRectangle rect;

  // clear state stack
  while (save) {
    s = save;
    save = save->next;
    XFreeGC(display, s->strokeGC);
    XFreeGC(display, s->fillGC);
    XDestroyRegion(s->clipRegion);
    delete s;
  }
  save = NULL;

  // default line flatness
  flatness = 0;

  // reset GCs
  gcValues.foreground = BlackPixel(display, screenNum);
  gcValues.background = WhitePixel(display, screenNum);
  gcValues.line_width = 0;
  gcValues.line_style = LineSolid;
  XChangeGC(display, strokeGC,
	    GCForeground | GCBackground | GCLineWidth | GCLineStyle,
	    &gcValues);
  XChangeGC(display, fillGC,
	    GCForeground | GCBackground | GCLineWidth | GCLineStyle,
	    &gcValues);

  // clear clipping region
  if (clipRegion)
    XDestroyRegion(clipRegion);
  clipRegion = XCreateRegion();
  rect.x = rect.y = 0;
  rect.width = pixmapW;
  rect.height = pixmapH;
  XUnionRectWithRegion(&rect, clipRegion, clipRegion);
  XSetRegion(display, strokeGC, clipRegion);
  XSetRegion(display, fillGC, clipRegion);

  // clear font
  gfxFont = NULL;
  font = NULL;

  // clear window
  XFillRectangle(display, pixmap, paperGC, 0, 0, pixmapW, pixmapH);

  // clear text object
  text->clear();
}

void XOutputDev::endPage() {
  text->coalesce();
}

void XOutputDev::drawLink(Link *link, Catalog *catalog) {
  double x1, y1, x2, y2, w;
  GfxRGB rgb;
  XPoint points[5];
  int x, y;

  link->getBorder(&x1, &y1, &x2, &y2, &w);
  if (w > 0) {
    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 1;
    XSetForeground(display, strokeGC, findColor(&rgb));
    XSetLineAttributes(display, strokeGC, xoutRound(w),
		       LineSolid, CapRound, JoinRound);
    cvtUserToDev(x1, y1, &x, &y);
    points[0].x = points[4].x = x;
    points[0].y = points[4].y = y;
    cvtUserToDev(x2, y1, &x, &y);
    points[1].x = x;
    points[1].y = y;
    cvtUserToDev(x2, y2, &x, &y);
    points[2].x = x;
    points[2].y = y;
    cvtUserToDev(x1, y2, &x, &y);
    points[3].x = x;
    points[3].y = y;
    XDrawLines(display, pixmap, strokeGC, points, 5, CoordModeOrigin);
  }
}

void XOutputDev::saveState(GfxState *state) {
  XOutputState *s;
  XGCValues values;

  // save current state
  s = new XOutputState;
  s->strokeGC = strokeGC;
  s->fillGC = fillGC;
  s->clipRegion = clipRegion;

  // push onto state stack
  s->next = save;
  save = s;

  // create a new current state by copying
  strokeGC = XCreateGC(display, pixmap, 0, &values);
  XCopyGC(display, s->strokeGC, 0xffffffff, strokeGC);
  fillGC = XCreateGC(display, pixmap, 0, &values);
  XCopyGC(display, s->fillGC, 0xffffffff, fillGC);
  clipRegion = XCreateRegion();
  XUnionRegion(s->clipRegion, clipRegion, clipRegion);
  XSetRegion(display, strokeGC, clipRegion);
  XSetRegion(display, fillGC, clipRegion);
}

void XOutputDev::restoreState(GfxState *state) {
  XOutputState *s;

  if (save) {
    // kill current state
    XFreeGC(display, strokeGC);
    XFreeGC(display, fillGC);
    XDestroyRegion(clipRegion);

    // restore state
    flatness = state->getFlatness();
    strokeGC = save->strokeGC;
    fillGC = save->fillGC;
    clipRegion = save->clipRegion;
    XSetRegion(display, strokeGC, clipRegion);
    XSetRegion(display, fillGC, clipRegion);

    // pop state stack
    s = save;
    save = save->next;
    delete s;
  }
}

void XOutputDev::updateAll(GfxState *state) {
  updateLineAttrs(state, gTrue);
  updateFlatness(state);
  updateMiterLimit(state);
  updateFillColor(state);
  updateStrokeColor(state);
  updateFont(state);
}

void XOutputDev::updateCTM(GfxState *state, double m11, double m12,
			   double m21, double m22, double m31, double m32) {
  updateLineAttrs(state, gTrue);
}

void XOutputDev::updateLineDash(GfxState *state) {
  updateLineAttrs(state, gTrue);
}

void XOutputDev::updateFlatness(GfxState *state) {
  flatness = state->getFlatness();
}

void XOutputDev::updateLineJoin(GfxState *state) {
  updateLineAttrs(state, gFalse);
}

void XOutputDev::updateLineCap(GfxState *state) {
  updateLineAttrs(state, gFalse);
}

// unimplemented
void XOutputDev::updateMiterLimit(GfxState *state) {
}

void XOutputDev::updateLineWidth(GfxState *state) {
  updateLineAttrs(state, gFalse);
}

void XOutputDev::updateLineAttrs(GfxState *state, GBool updateDash) {
  double width;
  int cap, join;
  double *dashPattern;
  int dashLength;
  double dashStart;
  char dashList[20];
  int i;

  width = state->getTransformedLineWidth();
  switch (state->getLineCap()) {
  case 0: cap = CapButt; break;
  case 1: cap = CapRound; break;
  case 2: cap = CapProjecting; break;
  default:
    error(-1, "Bad line cap style (%d)", state->getLineCap());
    cap = CapButt;
    break;
  }
  switch (state->getLineJoin()) {
  case 0: join = JoinMiter; break;
  case 1: join = JoinRound; break;
  case 2: join = JoinBevel; break;
  default:
    error(-1, "Bad line join style (%d)", state->getLineJoin());
    join = JoinMiter;
    break;
  }
  state->getLineDash(&dashPattern, &dashLength, &dashStart);
#if 1 //~ work around a bug in XFree86 (???)
  if (dashLength > 0 && cap == CapProjecting) {
    cap = CapButt;
  }
#endif
  XSetLineAttributes(display, strokeGC, xoutRound(width),
		     dashLength > 0 ? LineOnOffDash : LineSolid,
		     cap, join);
  if (updateDash && dashLength > 0) {
    if (dashLength > 20)
      dashLength = 20;
    for (i = 0; i < dashLength; ++i) {
      dashList[i] = xoutRound(state->transformWidth(dashPattern[i]));
      if (dashList[i] == 0)
	dashList[i] = 1;
    }
    XSetDashes(display, strokeGC, xoutRound(dashStart), dashList, dashLength);
  }
}

void XOutputDev::updateFillColor(GfxState *state) {
  GfxRGB rgb;

  state->getFillRGB(&rgb);
  XSetForeground(display, fillGC, findColor(&rgb));
}

void XOutputDev::updateStrokeColor(GfxState *state) {
  GfxRGB rgb;

  state->getStrokeRGB(&rgb);
  XSetForeground(display, strokeGC, findColor(&rgb));
}

void XOutputDev::updateFont(GfxState *state) {
  double m11, m12, m21, m22;

  if (!(gfxFont = state->getFont())) {
    font = NULL;
    return;
  }
  state->getFontTransMat(&m11, &m12, &m21, &m22);
  m11 *= state->getHorizScaling();
  m12 *= state->getHorizScaling();
  font = fontCache->getFont(xref, gfxFont, m11, m12, m21, m22);
  if (font) {
    font->updateGC(fillGC);
    font->updateGC(strokeGC);
  }

  text->updateFont(state);

  // look for Type 3 font
  if (!type3Warning && gfxFont->getType() == fontType3) {
    error(-1, "This document uses Type 3 fonts - some text may not be correctly displayed");
    type3Warning = gTrue;
  }
}

void XOutputDev::stroke(GfxState *state) {
  XPoint *points;
  int *lengths;
  int n, size, numPoints, i, j;

  // transform points
  n = convertPath(state, &points, &size, &numPoints, &lengths, gFalse);

  // draw each subpath
  j = 0;
  for (i = 0; i < n; ++i) {
    XDrawLines(display, pixmap, strokeGC, points + j, lengths[i],
	       CoordModeOrigin);
    j += lengths[i];
  }

  // free points and lengths arrays
  if (points != tmpPoints)
    gfree(points);
  if (lengths != tmpLengths)
    gfree(lengths);
}

void XOutputDev::fill(GfxState *state) {
  doFill(state, WindingRule);
}

void XOutputDev::eoFill(GfxState *state) {
  doFill(state, EvenOddRule);
}

//
//  X doesn't color the pixels on the right-most and bottom-most
//  borders of a polygon.  This means that one-pixel-thick polygons
//  are not colored at all.  I think this is supposed to be a
//  feature, but I can't figure out why.  So after it fills a
//  polygon, it also draws lines around the border.  This is done
//  only for single-component polygons, since it's not very
//  compatible with the compound polygon kludge (see convertPath()).
//
void XOutputDev::doFill(GfxState *state, int rule) {
  XPoint *points;
  int *lengths;
  int n, size, numPoints, i, j;

  // set fill rule
  XSetFillRule(display, fillGC, rule);

  // transform points, build separate polygons
  n = convertPath(state, &points, &size, &numPoints, &lengths, gTrue);

  // fill them
  j = 0;
  for (i = 0; i < n; ++i) {
    XFillPolygon(display, pixmap, fillGC, points + j, lengths[i],
		 Complex, CoordModeOrigin);
    if (state->getPath()->getNumSubpaths() == 1) {
      XDrawLines(display, pixmap, fillGC, points + j, lengths[i],
		 CoordModeOrigin);
    }
    j += lengths[i] + 1;
  }

  // free points and lengths arrays
  if (points != tmpPoints)
    gfree(points);
  if (lengths != tmpLengths)
    gfree(lengths);
}

void XOutputDev::clip(GfxState *state) {
  doClip(state, WindingRule);
}

void XOutputDev::eoClip(GfxState *state) {
  doClip(state, EvenOddRule);
}

void XOutputDev::doClip(GfxState *state, int rule) {
  Region region, region2;
  XPoint *points;
  int *lengths;
  int n, size, numPoints, i, j;

  // transform points, build separate polygons
  n = convertPath(state, &points, &size, &numPoints, &lengths, gTrue);

  // construct union of subpath regions
  // (XPolygonRegion chokes if there aren't at least three points --
  // this happens if the PDF file does moveto/closepath/clip, which
  // sets an empty clipping region)
  if (lengths[0] > 2) {
    region = XPolygonRegion(points, lengths[0], rule);
  } else {
    region = XCreateRegion();
  }
  j = lengths[0] + 1;
  for (i = 1; i < n; ++i) {
    if (lengths[i] > 2) {
      region2 = XPolygonRegion(points + j, lengths[i], rule);
    } else {
      region2 = XCreateRegion();
    }
    XUnionRegion(region2, region, region);
    XDestroyRegion(region2);
    j += lengths[i] + 1;
  }

  // intersect region with clipping region
  XIntersectRegion(region, clipRegion, clipRegion);
  XDestroyRegion(region);
  XSetRegion(display, strokeGC, clipRegion);
  XSetRegion(display, fillGC, clipRegion);

  // free points and lengths arrays
  if (points != tmpPoints)
    gfree(points);
  if (lengths != tmpLengths)
    gfree(lengths);
}

//
// Transform points in the path and convert curves to line segments.
// Builds a set of subpaths and returns the number of subpaths.
// If <fillHack> is set, close any unclosed subpaths and activate a
// kludge for polygon fills:  First, it divides up the subpaths into
// non-overlapping polygons by simply comparing bounding rectangles.
// Then it connects subaths within a single compound polygon to a single
// point so that X can fill the polygon (sort of).
//
int XOutputDev::convertPath(GfxState *state, XPoint **points, int *size,
			    int *numPoints, int **lengths, GBool fillHack) {
  GfxPath *path;
  BoundingRect *rects;
  BoundingRect rect;
  int n, i, ii, j, k, k0;

  // get path and number of subpaths
  path = state->getPath();
  n = path->getNumSubpaths();

  // allocate lengths array
  if (n < numTmpSubpaths)
    *lengths = tmpLengths;
  else
    *lengths = (int *)gmalloc(n * sizeof(int));

  // allocate bounding rectangles array
  if (fillHack) {
    if (n < numTmpSubpaths)
      rects = tmpRects;
    else
      rects = (BoundingRect *)gmalloc(n * sizeof(BoundingRect));
  } else {
    rects = NULL;
  }

  // do each subpath
  *points = tmpPoints;
  *size = numTmpPoints;
  *numPoints = 0;
  for (i = 0; i < n; ++i) {

    // transform the points
    j = *numPoints;
    convertSubpath(state, path->getSubpath(i), points, size, numPoints);

    // construct bounding rectangle
    if (fillHack) {
      rects[i].xMin = rects[i].xMax = (*points)[j].x;
      rects[i].yMin = rects[i].yMax = (*points)[j].y;
      for (k = j + 1; k < *numPoints; ++k) {
	if ((*points)[k].x < rects[i].xMin)
	  rects[i].xMin = (*points)[k].x;
	else if ((*points)[k].x > rects[i].xMax)
	  rects[i].xMax = (*points)[k].x;
	if ((*points)[k].y < rects[i].yMin)
	  rects[i].yMin = (*points)[k].y;
	else if ((*points)[k].y > rects[i].yMax)
	  rects[i].yMax = (*points)[k].y;
      }
    }

    // close subpath if necessary
    if (fillHack && ((*points)[*numPoints-1].x != (*points)[j].x ||
		     (*points)[*numPoints-1].y != (*points)[j].y)) {
      addPoint(points, size, numPoints, (*points)[j].x, (*points)[j].y);
    }

    // length of this subpath
    (*lengths)[i] = *numPoints - j;

    // leave an extra point for compound fill hack
    if (fillHack)
      addPoint(points, size, numPoints, 0, 0);
  }

  // kludge: munge any points that are *way* out of bounds - these can
  // crash certain (buggy) X servers
  for (i = 0; i < *numPoints; ++i) {
    if ((*points)[i].x < -pixmapW) {
      (*points)[i].x = -pixmapW;
    } else if ((*points)[i].x > 2 * pixmapW) {
      (*points)[i].x = 2 * pixmapW;
    }
    if ((*points)[i].y < -pixmapH) {
      (*points)[i].y = -pixmapH;
    } else if ((*points)[i].y > 2 * pixmapH) {
      (*points)[i].y = 2 * pixmapH;
    }
  }

  // combine compound polygons
  if (fillHack) {
    i = j = k = 0;
    while (i < n) {

      // start with subpath i
      rect = rects[i];
      (*lengths)[j] = (*lengths)[i];
      k0 = k;
      (*points)[k + (*lengths)[i]] = (*points)[k0];
      k += (*lengths)[i] + 1;
      ++i;

      // combine overlapping polygons
      do {

	// look for the first subsequent subpath, if any, which overlaps
	for (ii = i; ii < n; ++ii) {
	  if (rects[ii].xMax > rects[i].xMin &&
	      rects[ii].xMin < rects[i].xMax &&
	      rects[ii].yMax > rects[i].yMin &&
	      rects[ii].yMin < rects[i].yMax) {
	    break;
	  }
	}

	// if there is an overlap, combine the polygons
	if (ii < n) {
	  for (; i <= ii; ++i) {
	    if (rects[i].xMin < rect.xMin)
	      rect.xMin = rects[j].xMin;
	    if (rects[i].xMax > rect.xMax)
	      rect.xMax = rects[j].xMax;
	    if (rects[i].yMin < rect.yMin)
	      rect.yMin = rects[j].yMin;
	    if (rects[i].yMax > rect.yMax)
	      rect.yMax = rects[j].yMax;
	    (*lengths)[j] += (*lengths)[i] + 1;
	    (*points)[k + (*lengths)[i]] = (*points)[k0];
	    k += (*lengths)[i] + 1;
	  }
	}
      } while (ii < n && i < n);

      ++j;
    }

    // free bounding rectangles
    if (rects != tmpRects)
      gfree(rects);

    n = j;
  }

  return n;
}

//
// Transform points in a single subpath and convert curves to line
// segments.
//
void XOutputDev::convertSubpath(GfxState *state, GfxSubpath *subpath,
				XPoint **points, int *size, int *n) {
  double x0, y0, x1, y1, x2, y2, x3, y3;
  int m, i;

  m = subpath->getNumPoints();
  i = 0;
  while (i < m) {
    if (i >= 1 && subpath->getCurve(i)) {
      state->transform(subpath->getX(i-1), subpath->getY(i-1), &x0, &y0);
      state->transform(subpath->getX(i), subpath->getY(i), &x1, &y1);
      state->transform(subpath->getX(i+1), subpath->getY(i+1), &x2, &y2);
      state->transform(subpath->getX(i+2), subpath->getY(i+2), &x3, &y3);
      doCurve(points, size, n, x0, y0, x1, y1, x2, y2, x3, y3);
      i += 3;
    } else {
      state->transform(subpath->getX(i), subpath->getY(i), &x1, &y1);
      addPoint(points, size, n, xoutRound(x1), xoutRound(y1));
      ++i;
    }
  }
}

//
// Subdivide a Bezier curve.  This uses floating point to avoid
// propagating rounding errors.  (The curves look noticeably more
// jagged with integer arithmetic.)
//
void XOutputDev::doCurve(XPoint **points, int *size, int *n,
			 double x0, double y0, double x1, double y1,
			 double x2, double y2, double x3, double y3) {
  double x[(1<<maxCurveSplits)+1][3];
  double y[(1<<maxCurveSplits)+1][3];
  int next[1<<maxCurveSplits];
  int p1, p2, p3;
  double xx1, yy1, xx2, yy2;
  double dx, dy, mx, my, d1, d2;
  double xl0, yl0, xl1, yl1, xl2, yl2;
  double xr0, yr0, xr1, yr1, xr2, yr2, xr3, yr3;
  double xh, yh;
  double flat;

  flat = (double)(flatness * flatness);
  if (flat < 1)
    flat = 1;

  // initial segment
  p1 = 0;
  p2 = 1<<maxCurveSplits;
  x[p1][0] = x0;  y[p1][0] = y0;
  x[p1][1] = x1;  y[p1][1] = y1;
  x[p1][2] = x2;  y[p1][2] = y2;
  x[p2][0] = x3;  y[p2][0] = y3;
  next[p1] = p2;

  while (p1 < (1<<maxCurveSplits)) {

    // get next segment
    xl0 = x[p1][0];  yl0 = y[p1][0];
    xx1 = x[p1][1];  yy1 = y[p1][1];
    xx2 = x[p1][2];  yy2 = y[p1][2];
    p2 = next[p1];
    xr3 = x[p2][0];  yr3 = y[p2][0];

    // compute distances from control points to midpoint of the
    // straight line (this is a bit of a hack, but it's much faster
    // than computing the actual distances to the line)
    mx = (xl0 + xr3) * 0.5;
    my = (yl0 + yr3) * 0.5;
    dx = xx1 - mx;
    dy = yy1 - my;
    d1 = dx*dx + dy*dy;
    dx = xx2 - mx;
    dy = yy2 - my;
    d2 = dx*dx + dy*dy;

    // if curve is flat enough, or no more divisions allowed then
    // add the straight line segment
    if (p2 - p1 <= 1 || (d1 <= flat && d2 <= flat)) {
      addPoint(points, size, n, xoutRound(xr3), xoutRound(yr3));
      p1 = p2;

    // otherwise, subdivide the curve
    } else {
      xl1 = (xl0 + xx1) * 0.5;
      yl1 = (yl0 + yy1) * 0.5;
      xh = (xx1 + xx2) * 0.5;
      yh = (yy1 + yy2) * 0.5;
      xl2 = (xl1 + xh) * 0.5;
      yl2 = (yl1 + yh) * 0.5;
      xr2 = (xx2 + xr3) * 0.5;
      yr2 = (yy2 + yr3) * 0.5;
      xr1 = (xh + xr2) * 0.5;
      yr1 = (yh + yr2) * 0.5;
      xr0 = (xl2 + xr1) * 0.5;
      yr0 = (yl2 + yr1) * 0.5;

      // add the new subdivision points
      p3 = (p1 + p2) / 2;
      x[p1][1] = xl1;  y[p1][1] = yl1;
      x[p1][2] = xl2;  y[p1][2] = yl2;
      next[p1] = p3;
      x[p3][0] = xr0;  y[p3][0] = yr0;
      x[p3][1] = xr1;  y[p3][1] = yr1;
      x[p3][2] = xr2;  y[p3][2] = yr2;
      next[p3] = p2;
    }
  }
}

//
// Add a point to the points array.  (This would use a generic resizable
// array type if C++ supported parameterized types in some reasonable
// way -- templates are a disgusting kludge.)
//
void XOutputDev::addPoint(XPoint **points, int *size, int *k, int x, int y) {
  if (*k >= *size) {
    *size += 32;
    if (*points == tmpPoints) {
      *points = (XPoint *)gmalloc(*size * sizeof(XPoint));
      memcpy(*points, tmpPoints, *k * sizeof(XPoint));
    } else {
      *points = (XPoint *)grealloc(*points, *size * sizeof(XPoint));
    }
  }
  (*points)[*k].x = x;
  (*points)[*k].y = y;
  ++(*k);
}

void XOutputDev::beginString(GfxState *state, GString *s) {
  text->beginString(state);
}

void XOutputDev::endString(GfxState *state) {
  text->endString();
}

void XOutputDev::drawChar(GfxState *state, double x, double y,
			  double dx, double dy,
			  double originX, double originY,
			  CharCode code, Unicode *u, int uLen) {
  double x1, y1, dx1, dy1;

  text->addChar(state, x, y, dx, dy, u, uLen);

  if (!font) {
    return;
  }

  // check for invisible text -- this is used by Acrobat Capture
  if ((state->getRender() & 3) == 3) {
    return;
  }

  x -= originX;
  y -= originY;
  state->transform(x, y, &x1, &y1);
  state->transformDelta(dx, dy, &dx1, &dy1);

  font->drawChar(state, pixmap, pixmapW, pixmapH,
		 (state->getRender() & 1) ? strokeGC : fillGC,
		 x1, y1, dx1, dy1, code, u, uLen);
}

inline Gulong XOutputDev::findColor(GfxRGB *x, GfxRGB *err) {
  double gray;
  int r, g, b;
  Gulong pixel;

  if (trueColor) {
    r = xoutRound(x->r * rMul);
    g = xoutRound(x->g * gMul);
    b = xoutRound(x->b * bMul);
    pixel = ((Gulong)r << rShift) +
            ((Gulong)g << gShift) +
            ((Gulong)b << bShift);
    err->r = x->r - (double)r / rMul;
    err->g = x->g - (double)g / gMul;
    err->b = x->b - (double)b / bMul;
  } else if (numColors == 1) {
    gray = 0.299 * x->r + 0.587 * x->g + 0.114 * x->b;
    if (gray < 0.5) {
      pixel = colors[0];
      err->r = x->r;
      err->g = x->g;
      err->b = x->b;
    } else {
      pixel = colors[1];
      err->r = x->r - 1;
      err->g = x->g - 1;
      err->b = x->b - 1;
    }
  } else {
    r = xoutRound(x->r * (numColors - 1));
    g = xoutRound(x->g * (numColors - 1));
    b = xoutRound(x->b * (numColors - 1));
    pixel = colors[(r * numColors + g) * numColors + b];
    err->r = x->r - (double)r / (numColors - 1);
    err->g = x->g - (double)g / (numColors - 1); 
    err->b = x->b - (double)b / (numColors - 1);
  }
  return pixel;
}

Gulong XOutputDev::findColor(GfxRGB *rgb) {
  int r, g, b;
  double gray;
  Gulong pixel;

  if (trueColor) {
    r = xoutRound(rgb->r * rMul);
    g = xoutRound(rgb->g * gMul);
    b = xoutRound(rgb->b * bMul);
    pixel = ((Gulong)r << rShift) +
            ((Gulong)g << gShift) +
            ((Gulong)b << bShift);
  } else if (numColors == 1) {
    gray = 0.299 * rgb->r + 0.587 * rgb->g + 0.114 * rgb->b;
    if (gray < 0.5)
      pixel = colors[0];
    else
      pixel = colors[1];
  } else {
    r = xoutRound(rgb->r * (numColors - 1));
    g = xoutRound(rgb->g * (numColors - 1));
    b = xoutRound(rgb->b * (numColors - 1));
#if 0 // this makes things worse as often as better
    // even a very light color shouldn't map to white
    if (r == numColors - 1 && g == numColors - 1 && b == numColors - 1) {
      if (color->getR() < 0.95)
	--r;
      if (color->getG() < 0.95)
	--g;
      if (color->getB() < 0.95)
	--b;
    }
#endif
    pixel = colors[(r * numColors + g) * numColors + b];
  }
  return pixel;
}

void XOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
			       int width, int height, GBool invert,
			       GBool inlineImg) {
  ImageStream *imgStr;
  XImage *image;
  double *ctm;
  GBool rot;
  double xScale, yScale, xShear, yShear;
  int tx, ty, scaledWidth, scaledHeight, xSign, ySign;
  int ulx, uly, llx, lly, urx, ury, lrx, lry;
  int ulx1, uly1, llx1, lly1, urx1, ury1, lrx1, lry1;
  int bx0, by0, bx1, by1, bw, bh;
  int cx0, cy0, cx1, cy1, cw, ch;
  int yp, yq, yt, yStep, lastYStep;
  int xp, xq, xt, xStep, xSrc;
  GfxRGB rgb;
  Guchar *pixBuf;
  int imgPix;
  double alpha;
  XColor xcolor;
  Gulong lastPixel;
  GfxRGB rgb2;
  double r0, g0, b0, r1, g1, b1;
  Gulong pix;
  Guchar *p;
  int x, y, x1, y1, x2, y2;
  int n, m, i, j;

  // get CTM, check for singular matrix
  ctm = state->getCTM();
  if (fabs(ctm[0] * ctm[3] - ctm[1] * ctm[2]) < 0.000001) {
    error(-1, "Singular CTM in drawImage");
    if (inlineImg) {
      j = height * ((width + 7) / 8);
      str->reset();
      for (i = 0; i < j; ++i) {
	str->getChar();
      }
      str->close();
    }
    return;
  }

  // compute scale, shear, rotation, translation parameters
  rot = fabs(ctm[1]) > fabs(ctm[0]);
  if (rot) {
    xScale = -ctm[1];
    yScale = -ctm[2] + (ctm[0] * ctm[3]) / ctm[1];
    xShear = ctm[3] / yScale;
    yShear = -ctm[0] / ctm[1];
  } else {
    xScale = ctm[0];
    yScale = -ctm[3] + (ctm[1] * ctm[2]) / ctm[0];
    xShear = -ctm[2] / yScale;
    yShear = ctm[1] / ctm[0];
  }
  tx = xoutRound(ctm[2] + ctm[4]);
  ty = xoutRound(ctm[3] + ctm[5]);
  // use ceil() to avoid gaps between "striped" images
  scaledWidth = (int)ceil(fabs(xScale));
  xSign = (xScale < 0) ? -1 : 1;
  scaledHeight = (int)ceil(fabs(yScale));
  ySign = (yScale < 0) ? -1 : 1;

  // compute corners in device space
  ulx1 = 0;
  uly1 = 0;
  urx1 = xSign * (scaledWidth - 1);
  ury1 = xoutRound(yShear * urx1);
  llx1 = xoutRound(xShear * ySign * (scaledHeight - 1));
  lly1 = ySign * (scaledHeight - 1) + xoutRound(yShear * llx1);
  lrx1 = xSign * (scaledWidth - 1) +
           xoutRound(xShear * ySign * (scaledHeight - 1));
  lry1 = ySign * (scaledHeight - 1) + xoutRound(yShear * lrx1);
  if (rot) {
    ulx = tx + uly1;    uly = ty - ulx1;
    urx = tx + ury1;    ury = ty - urx1;
    llx = tx + lly1;    lly = ty - llx1;
    lrx = tx + lry1;    lry = ty - lrx1;
  } else {
    ulx = tx + ulx1;    uly = ty + uly1;
    urx = tx + urx1;    ury = ty + ury1;
    llx = tx + llx1;    lly = ty + lly1;
    lrx = tx + lrx1;    lry = ty + lry1;
  }

  // bounding box:
  //   (bx0, by0) = upper-left corner
  //   (bx1, by1) = lower-right corner
  //   (bw, bh) = size
  bx0 = (ulx < urx) ? (ulx < llx) ? (ulx < lrx) ? ulx : lrx
                                  : (llx < lrx) ? llx : lrx
		    : (urx < llx) ? (urx < lrx) ? urx : lrx
                                  : (llx < lrx) ? llx : lrx;
  bx1 = (ulx > urx) ? (ulx > llx) ? (ulx > lrx) ? ulx : lrx
                                  : (llx > lrx) ? llx : lrx
		    : (urx > llx) ? (urx > lrx) ? urx : lrx
                                  : (llx > lrx) ? llx : lrx;
  by0 = (uly < ury) ? (uly < lly) ? (uly < lry) ? uly : lry
                                  : (lly < lry) ? lly : lry
		    : (ury < lly) ? (ury < lry) ? ury : lry
                                  : (lly < lry) ? lly : lry;
  by1 = (uly > ury) ? (uly > lly) ? (uly > lry) ? uly : lry
                                  : (lly > lry) ? lly : lry
		    : (ury > lly) ? (ury > lry) ? ury : lry
                                  : (lly > lry) ? lly : lry;
  bw = bx1 - bx0 + 1;
  bh = by1 - by0 + 1;

  // Bounding box clipped to pixmap, i.e., "valid" rectangle:
  //   (cx0, cy0) = upper-left corner of valid rectangle in Pixmap
  //   (cx1, cy1) = upper-left corner of valid rectangle in XImage
  //   (cw, ch) = size of valid rectangle
  // These values will be used to transfer the XImage from/to the
  // Pixmap.
  cw = (bx1 >= pixmapW) ? pixmapW - bx0 : bw;
  if (bx0 < 0) {
    cx0 = 0;
    cx1 = -bx0;
    cw += bx0;
  } else {
    cx0 = bx0;
    cx1 = 0;
  }
  ch = (by1 >= pixmapH) ? pixmapH - by0 : bh;
  if (by0 < 0) {
    cy0 = 0;
    cy1 = -by0;
    ch += by0;
  } else {
    cy0 = by0;
    cy1 = 0;
  }

  // check for tiny (zero width or height) images
  // and off-page images
  if (scaledWidth <= 0 || scaledHeight <= 0 || cw <= 0 || ch <= 0) {
    if (inlineImg) {
      j = height * ((width + 7) / 8);
      str->reset();
      for (i = 0; i < j; ++i) {
	str->getChar();
      }
      str->close();
    }
    return;
  }

  // compute Bresenham parameters for x and y scaling
  yp = height / scaledHeight;
  yq = height % scaledHeight;
  xp = width / scaledWidth;
  xq = width % scaledWidth;

  // allocate pixel buffer
  pixBuf = (Guchar *)gmalloc((yp + 1) * width * sizeof(Guchar));

  // allocate XImage and read from page pixmap
  image = XCreateImage(display, DefaultVisual(display, screenNum),
		       depth, ZPixmap, 0, NULL, bw, bh, 8, 0);
  image->data = (char *)gmalloc(bh * image->bytes_per_line);
  XGetSubImage(display, pixmap, cx0, cy0, cw, ch, (1 << depth) - 1, ZPixmap,
	       image, cx1, cy1);

  // get mask color
  state->getFillRGB(&rgb);
  r0 = rgb.r;
  g0 = rgb.g;
  b0 = rgb.b;

  // initialize background color
  // (the specific pixel value doesn't matter here, as long as
  // r1,g1,b1 correspond correctly to lastPixel)
  xcolor.pixel = lastPixel = 0;
  XQueryColor(display, colormap, &xcolor);
  r1 = (double)xcolor.red / 65535.0;
  g1 = (double)xcolor.green / 65535.0;
  b1 = (double)xcolor.blue / 65535.0;

  // initialize the image stream
  imgStr = new ImageStream(str, width, 1, 1);
  imgStr->reset();

  // init y scale Bresenham
  yt = 0;
  lastYStep = 1;

  for (y = 0; y < scaledHeight; ++y) {

    // y scale Bresenham
    yStep = yp;
    yt += yq;
    if (yt >= scaledHeight) {
      yt -= scaledHeight;
      ++yStep;
    }

    // read row(s) from image
    n = (yp > 0) ? yStep : lastYStep;
    if (n > 0) {
      p = pixBuf;
      for (i = 0; i < n; ++i) {
	for (j = 0; j < width; ++j) {
	  imgStr->getPixel(p);
	  if (invert) {
	    *p ^= 1;
	  }
	  ++p;
	}
      }
    }
    lastYStep = yStep;

    // init x scale Bresenham
    xt = 0;
    xSrc = 0;

    for (x = 0; x < scaledWidth; ++x) {

      // x scale Bresenham
      xStep = xp;
      xt += xq;
      if (xt >= scaledWidth) {
	xt -= scaledWidth;
	++xStep;
      }

      // x shear
      x1 = xSign * x + xoutRound(xShear * ySign * y);

      // y shear
      y1 = ySign * y + xoutRound(yShear * x1);

      // rotation
      if (rot) {
	x2 = y1;
	y2 = -x1;
      } else {
	x2 = x1;
	y2 = y1;
      }

      // compute the filtered pixel at (x,y) after the
      // x and y scaling operations
      n = yStep > 0 ? yStep : 1;
      m = xStep > 0 ? xStep : 1;
      p = pixBuf + xSrc;
      imgPix = 0;
      for (i = 0; i < n; ++i) {
	for (j = 0; j < m; ++j) {
	  imgPix += *p++;
	}
	p += width - m;
      }

      // x scale Bresenham
      xSrc += xStep;

      // blend image pixel with background
      alpha = (double)imgPix / (double)(n * m);
      xcolor.pixel = XGetPixel(image, tx + x2 - bx0, ty + y2 - by0);
      if (xcolor.pixel != lastPixel) {
	XQueryColor(display, colormap, &xcolor);
	r1 = (double)xcolor.red / 65535.0;
	g1 = (double)xcolor.green / 65535.0;
	b1 = (double)xcolor.blue / 65535.0;
	lastPixel = xcolor.pixel;
      }
      rgb2.r = r0 * (1 - alpha) + r1 * alpha;
      rgb2.g = g0 * (1 - alpha) + g1 * alpha;
      rgb2.b = b0 * (1 - alpha) + b1 * alpha;
      pix = findColor(&rgb2);

      // set pixel
      XPutPixel(image, tx + x2 - bx0, ty + y2 - by0, pix);
    }
  }

  // blit the image into the pixmap
  XPutImage(display, pixmap, fillGC, image, cx1, cy1, cx0, cy0, cw, ch);

  // free memory
  delete imgStr;
  gfree(pixBuf);
  gfree(image->data);
  image->data = NULL;
  XDestroyImage(image);
}

void XOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
			   int width, int height, GfxImageColorMap *colorMap,
			   int *maskColors, GBool inlineImg) {
  ImageStream *imgStr;
  XImage *image;
  int nComps, nVals, nBits;
  GBool dither;
  double *ctm;
  GBool rot;
  double xScale, yScale, xShear, yShear;
  int tx, ty, scaledWidth, scaledHeight, xSign, ySign;
  int ulx, uly, llx, lly, urx, ury, lrx, lry;
  int ulx1, uly1, llx1, lly1, urx1, ury1, lrx1, lry1;
  int bx0, by0, bx1, by1, bw, bh;
  int cx0, cy0, cx1, cy1, cw, ch;
  int yp, yq, yt, yStep, lastYStep;
  int xp, xq, xt, xStep, xSrc;
  GfxRGB *pixBuf;
  Guchar *alphaBuf;
  Guchar pixBuf2[gfxColorMaxComps];
  GfxRGB color2, err, errRight;
  GfxRGB *errDown;
  double r0, g0, b0, alpha;
  Gulong pix;
  GfxRGB *p;
  Guchar *q;
  int x, y, x1, y1, x2, y2;
  int n, m, i, j, k;

  // image parameters
  nComps = colorMap->getNumPixelComps();
  nVals = width * nComps;
  nBits = colorMap->getBits();
  dither = nComps > 1 || nBits > 1;

  // get CTM, check for singular matrix
  ctm = state->getCTM();
  if (fabs(ctm[0] * ctm[3] - ctm[1] * ctm[2]) < 0.000001) {
    error(-1, "Singular CTM in drawImage");
    if (inlineImg) {
      str->reset();
      j = height * ((nVals * nBits + 7) / 8);
      for (i = 0; i < j; ++i) {
	str->getChar();
      }
      str->close();
    }
    return;
  }

  // compute scale, shear, rotation, translation parameters
  rot = fabs(ctm[1]) > fabs(ctm[0]);
  if (rot) {
    xScale = -ctm[1];
    yScale = -ctm[2] + (ctm[0] * ctm[3]) / ctm[1];
    xShear = ctm[3] / yScale;
    yShear = -ctm[0] / ctm[1];
  } else {
    xScale = ctm[0];
    yScale = -ctm[3] + (ctm[1] * ctm[2]) / ctm[0];
    xShear = -ctm[2] / yScale;
    yShear = ctm[1] / ctm[0];
  }
  tx = xoutRound(ctm[2] + ctm[4]);
  ty = xoutRound(ctm[3] + ctm[5]);
  // use ceil() to avoid gaps between "striped" images
  scaledWidth = (int)ceil(fabs(xScale));
  xSign = (xScale < 0) ? -1 : 1;
  scaledHeight = (int)ceil(fabs(yScale));
  ySign = (yScale < 0) ? -1 : 1;

  // compute corners in device space
  ulx1 = 0;
  uly1 = 0;
  urx1 = xSign * (scaledWidth - 1);
  ury1 = xoutRound(yShear * urx1);
  llx1 = xoutRound(xShear * ySign * (scaledHeight - 1));
  lly1 = ySign * (scaledHeight - 1) + xoutRound(yShear * llx1);
  lrx1 = xSign * (scaledWidth - 1) +
           xoutRound(xShear * ySign * (scaledHeight - 1));
  lry1 = ySign * (scaledHeight - 1) + xoutRound(yShear * lrx1);
  if (rot) {
    ulx = tx + uly1;    uly = ty - ulx1;
    urx = tx + ury1;    ury = ty - urx1;
    llx = tx + lly1;    lly = ty - llx1;
    lrx = tx + lry1;    lry = ty - lrx1;
  } else {
    ulx = tx + ulx1;    uly = ty + uly1;
    urx = tx + urx1;    ury = ty + ury1;
    llx = tx + llx1;    lly = ty + lly1;
    lrx = tx + lrx1;    lry = ty + lry1;
  }

  // bounding box:
  //   (bx0, by0) = upper-left corner
  //   (bx1, by1) = lower-right corner
  //   (bw, bh) = size
  bx0 = (ulx < urx) ? (ulx < llx) ? (ulx < lrx) ? ulx : lrx
                                  : (llx < lrx) ? llx : lrx
		    : (urx < llx) ? (urx < lrx) ? urx : lrx
                                  : (llx < lrx) ? llx : lrx;
  bx1 = (ulx > urx) ? (ulx > llx) ? (ulx > lrx) ? ulx : lrx
                                  : (llx > lrx) ? llx : lrx
		    : (urx > llx) ? (urx > lrx) ? urx : lrx
                                  : (llx > lrx) ? llx : lrx;
  by0 = (uly < ury) ? (uly < lly) ? (uly < lry) ? uly : lry
                                  : (lly < lry) ? lly : lry
		    : (ury < lly) ? (ury < lry) ? ury : lry
                                  : (lly < lry) ? lly : lry;
  by1 = (uly > ury) ? (uly > lly) ? (uly > lry) ? uly : lry
                                  : (lly > lry) ? lly : lry
		    : (ury > lly) ? (ury > lry) ? ury : lry
                                  : (lly > lry) ? lly : lry;
  bw = bx1 - bx0 + 1;
  bh = by1 - by0 + 1;

  // Bounding box clipped to pixmap, i.e., "valid" rectangle:
  //   (cx0, cy0) = upper-left corner of valid rectangle in Pixmap
  //   (cx1, cy1) = upper-left corner of valid rectangle in XImage
  //   (cw, ch) = size of valid rectangle
  // These values will be used to transfer the XImage from/to the
  // Pixmap.
  cw = (bx1 >= pixmapW) ? pixmapW - bx0 : bw;
  if (bx0 < 0) {
    cx0 = 0;
    cx1 = -bx0;
    cw += bx0;
  } else {
    cx0 = bx0;
    cx1 = 0;
  }
  ch = (by1 >= pixmapH) ? pixmapH - by0 : bh;
  if (by0 < 0) {
    cy0 = 0;
    cy1 = -by0;
    ch += by0;
  } else {
    cy0 = by0;
    cy1 = 0;
  }

  // check for tiny (zero width or height) images
  // and off-page images
  if (scaledWidth <= 0 || scaledHeight <= 0 || cw <= 0 || ch <= 0) {
    if (inlineImg) {
      str->reset();
      j = height * ((nVals * nBits + 7) / 8);
      for (i = 0; i < j; ++i)
	str->getChar();
      str->close();
    }
    return;
  }

  // compute Bresenham parameters for x and y scaling
  yp = height / scaledHeight;
  yq = height % scaledHeight;
  xp = width / scaledWidth;
  xq = width % scaledWidth;

  // allocate pixel buffer
  pixBuf = (GfxRGB *)gmalloc((yp + 1) * width * sizeof(GfxRGB));
  if (maskColors) {
    alphaBuf = (Guchar *)gmalloc((yp + 1) * width * sizeof(Guchar));
  } else {
    alphaBuf = NULL;
  }

  // allocate XImage
  image = XCreateImage(display, DefaultVisual(display, screenNum),
		       depth, ZPixmap, 0, NULL, bw, bh, 8, 0);
  image->data = (char *)gmalloc(bh * image->bytes_per_line);

  // if the transform is anything other than a 0/90/180/270 degree
  // rotation/flip, or if there is color key masking, read the
  // backgound pixmap to fill in the corners
  if (!((ulx == llx && uly == ury) ||
	(uly == lly && ulx == urx)) ||
      maskColors) {
    XGetSubImage(display, pixmap, cx0, cy0, cw, ch, (1 << depth) - 1, ZPixmap,
		 image, cx1, cy1);
  }

  // allocate error diffusion accumulators
  if (dither) {
    errDown = (GfxRGB *)gmalloc(bw * sizeof(GfxRGB));
    for (j = 0; j < bw; ++j) {
      errDown[j].r = errDown[j].g = errDown[j].b = 0;
    }
  } else {
    errDown = NULL;
  }

  // initialize the image stream
  imgStr = new ImageStream(str, width, nComps, nBits);
  imgStr->reset();

  // init y scale Bresenham
  yt = 0;
  lastYStep = 1;

  for (y = 0; y < scaledHeight; ++y) {

    // initialize error diffusion accumulator
    errRight.r = errRight.g = errRight.b = 0;

    // y scale Bresenham
    yStep = yp;
    yt += yq;
    if (yt >= scaledHeight) {
      yt -= scaledHeight;
      ++yStep;
    }

    // read row(s) from image
    n = (yp > 0) ? yStep : lastYStep;
    if (n > 0) {
      p = pixBuf;
      q = alphaBuf;
      for (i = 0; i < n; ++i) {
	for (j = 0; j < width; ++j) {
	  imgStr->getPixel(pixBuf2);
	  colorMap->getRGB(pixBuf2, p);
	  ++p;
	  if (maskColors) {
	    *q = 1;
	    for (k = 0; k < nComps; ++k) {
	      if (pixBuf2[k] < maskColors[2*k] ||
		  pixBuf2[k] > maskColors[2*k]) {
		*q = 0;
		break;
	      }
	    }
	    ++q;
	  }
	}
      }
    }
    lastYStep = yStep;

    // init x scale Bresenham
    xt = 0;
    xSrc = 0;

    for (x = 0; x < scaledWidth; ++x) {

      // x scale Bresenham
      xStep = xp;
      xt += xq;
      if (xt >= scaledWidth) {
	xt -= scaledWidth;
	++xStep;
      }

      // x shear
      x1 = xSign * x + xoutRound(xShear * ySign * y);

      // y shear
      y1 = ySign * y + xoutRound(yShear * x1);

      // rotation
      if (rot) {
	x2 = y1;
	y2 = -x1;
      } else {
	x2 = x1;
	y2 = y1;
      }

      // compute the filtered pixel at (x,y) after the
      // x and y scaling operations
      n = yStep > 0 ? yStep : 1;
      m = xStep > 0 ? xStep : 1;
      p = pixBuf + xSrc;
      r0 = g0 = b0 = 0;
      q = alphaBuf + xSrc;
      alpha = 0;
      for (i = 0; i < n; ++i) {
	for (j = 0; j < m; ++j) {
	  r0 += p->r;
	  g0 += p->g;
	  b0 += p->b;
	  ++p;
	  if (maskColors) {
	    alpha += *q++;
	  }
	}
	p += width - m;
      }
      r0 /= n * m;
      g0 /= n * m;
      b0 /= n * m;
      alpha /= n * m;

      // x scale Bresenham
      xSrc += xStep;

      // compute pixel
      if (dither) {
	color2.r = r0 + errRight.r + errDown[tx + x2 - bx0].r;
	if (color2.r > 1) {
	  color2.r = 1;
	} else if (color2.r < 0) {
	  color2.r = 0;
	}
	color2.g = g0 + errRight.g + errDown[tx + x2 - bx0].g;
	if (color2.g > 1) {
	  color2.g = 1;
	} else if (color2.g < 0) {
	  color2.g = 0;
	}
	color2.b = b0 + errRight.b + errDown[tx + x2 - bx0].b;
	if (color2.b > 1) {
	  color2.b = 1;
	} else if (color2.b < 0) {
	  color2.b = 0;
	}
	pix = findColor(&color2, &err);
	errRight.r = errDown[tx + x2 - bx0].r = err.r / 2;
	errRight.g = errDown[tx + x2 - bx0].g = err.g / 2;
	errRight.b = errDown[tx + x2 - bx0].b = err.b / 2;
      } else {
	color2.r = r0;
	color2.g = g0;
	color2.b = b0;
	pix = findColor(&color2, &err);
      }

      // set pixel
      //~ this should do a blend when 0 < alpha < 1
      if (alpha < 0.75) {
	XPutPixel(image, tx + x2 - bx0, ty + y2 - by0, pix);
      }
    }
  }

  // blit the image into the pixmap
  XPutImage(display, pixmap, fillGC, image, cx1, cy1, cx0, cy0, cw, ch);

  // free memory
  delete imgStr;
  gfree(pixBuf);
  if (maskColors) {
    gfree(alphaBuf);
  }
  gfree(image->data);
  image->data = NULL;
  XDestroyImage(image);
  gfree(errDown);
}

GBool XOutputDev::findText(Unicode *s, int len, GBool top, GBool bottom,
			   int *xMin, int *yMin, int *xMax, int *yMax) {
  double xMin1, yMin1, xMax1, yMax1;
  
  xMin1 = (double)*xMin;
  yMin1 = (double)*yMin;
  xMax1 = (double)*xMax;
  yMax1 = (double)*yMax;
  if (text->findText(s, len, top, bottom, &xMin1, &yMin1, &xMax1, &yMax1)) {
    *xMin = xoutRound(xMin1);
    *xMax = xoutRound(xMax1);
    *yMin = xoutRound(yMin1);
    *yMax = xoutRound(yMax1);
    return gTrue;
  }
  return gFalse;
}

GString *XOutputDev::getText(int xMin, int yMin, int xMax, int yMax) {
  return text->getText((double)xMin, (double)yMin,
		       (double)xMax, (double)yMax);
}
