//========================================================================
//
// GfxFont.h
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#ifndef GFXFONT_H
#define GFXFONT_H

#ifdef __GNUC__
#pragma interface
#endif

#include <gtypes.h>
#include <GString.h>
#include "Object.h"

class Dict;

//------------------------------------------------------------------------
// GfxFontEncoding
//------------------------------------------------------------------------

#define gfxFontEncHashSize 419

class GfxFontEncoding {
public:

  // Construct an empty encoding.
  GfxFontEncoding();

  // Construct an encoding from an array of char names.
  GfxFontEncoding(char **encoding1, int encSize);

  // Destructor.
  ~GfxFontEncoding();

  // Add a char to the encoding.
  void addChar(int code, char *name);

  // Return the character name associated with <code>.
  char *getCharName(int code) { return encoding[code]; }

  // Return the code associated with <name>.
  int getCharCode(char *name);

private:

  int hash(char *name);
  void addChar1(int code, char *name);

  char **encoding;		// code --> name mapping
  GBool freeEnc;		// should we free the encoding array?
  short				// name --> code hash table
    hashTab[gfxFontEncHashSize];
};

//------------------------------------------------------------------------
// GfxFont
//------------------------------------------------------------------------

#define fontFixedWidth (1 << 0)
#define fontSerif      (1 << 1)
#define fontSymbolic   (1 << 2)
#define fontItalic     (1 << 6)
#define fontBold       (1 << 18)

enum GfxFontType {
  fontUnknownType,
  fontType1,
  fontType3,
  fontTrueType
};

class GfxFont {
public:

  // Constructor.
  GfxFont(char *tag1, Ref id1, Dict *fontDict);

  // Destructor.
  ~GfxFont();

  // Get font tag.
  GString *getTag() { return tag; }

  // Get font dictionary ID.
  Ref getID() { return id; }

  // Does this font match the tag?
  GBool matches(char *tag1) { return !tag->cmp(tag1); }

  // Get base font name.
  GString *getName() { return name; }

  // Get font type.
  GfxFontType getType() { return type; }

  // Get embedded font ID, i.e., a ref for the font file stream.
  // Returns false if there is no embedded font.
  GBool getEmbeddedFontID(Ref *embID)
    { *embID = embFontID; return embFontID.num >= 0; }

  // Get the PostScript font name for the embedded font.  Returns
  // NULL if there is no embedded font.
  char *getEmbeddedFontName()
    { return embFontName ? embFontName->getCString() : (char *)NULL; }

  // Get the name of the external font file.  Returns NULL if there
  // is no external font file.
  char *getExtFontFile()
    { return extFontFile ? extFontFile->getCString() : (char *)NULL; }

  // Get font descriptor flags.
  GBool isFixedWidth() { return flags & fontFixedWidth; }
  GBool isSerif() { return flags & fontSerif; }
  GBool isSymbolic() { return flags & fontSymbolic; }
  GBool isItalic() { return flags & fontItalic; }
  GBool isBold() { return flags & fontBold; }

  // Get width of a character.
  double getWidth(Guchar c) { return widths[c]; }
  double getWidth(GString *s);

  // Return the character name associated with <code>.
  char *getCharName(int code) { return encoding->getCharName(code); }

  // Return the code associated with <name>.
  int getCharCode(char *charName) { return encoding->getCharCode(charName); }

  // Return the font matrix.
  double *getFontMatrix() { return fontMat; }

private:

  void makeEncoding(Dict *fontDict, GfxFontEncoding *builtinEncoding);
  GfxFontEncoding *makeEncoding1(Object obj, Dict *fontDesc,
				 GfxFontEncoding *builtinEncoding);
  void getType1Encoding(Stream *str);
  void makeWidths(Dict *fontDict, GfxFontEncoding *builtinEncoding,
		  Gushort *builtinWidths);
  GBool getTrueTypeWidths();

  GString *tag;			// PDF font tag
  Ref id;			// reference (used as unique ID)
  GString *name;		// font name
  int flags;			// font descriptor flags
  GfxFontType type;		// type of font
  GString *embFontName;		// name of embedded font
  Ref embFontID;		// ref to embedded font file stream
  GString *extFontFile;		// external font file name
  double fontMat[6];		// font matrix
  double widths[256];		// width of each char
  GfxFontEncoding *encoding;	// font encoding
};

//------------------------------------------------------------------------
// GfxFontDict
//------------------------------------------------------------------------

class GfxFontDict {
public:

  // Build the font dictionary, given the PDF font dictionary.
  GfxFontDict(Dict *fontDict);

  // Destructor.
  ~GfxFontDict();

  // Get the specified font.
  GfxFont *lookup(char *tag);

  // Iterative access.
  int getNumFonts() { return numFonts; }
  GfxFont *getFont(int i) { return fonts[i]; }

private:

  GfxFont **fonts;		// list of fonts
  int numFonts;			// number of fonts
};

#endif
