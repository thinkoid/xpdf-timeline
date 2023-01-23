//========================================================================
//
// BuiltinFont.cc
//
// Copyright 2001-2002 Glyph & Cog, LLC
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include <aconf.h>
#include <stdlib.h>
#include <string.h>
#include "gmem.h"
#include "FontEncodingTables.h"
#include "BuiltinFont.h"

//------------------------------------------------------------------------

BuiltinFontWidths::BuiltinFontWidths(BuiltinFontWidth *widths, int sizeA) {
  int i, h;

  size = sizeA;
  tab = (BuiltinFontWidth **)gmalloc(size * sizeof(BuiltinFontWidth *));
  for (i = 0; i < size; ++i) {
    tab[i] = NULL;
  }
  for (i = 0; i < sizeA; ++i) {
    h = hash(widths[i].name);
    widths[i].next = tab[h];
    tab[h] = &widths[i];
  }
}

BuiltinFontWidths::~BuiltinFontWidths() {
  gfree(tab);
}

GBool BuiltinFontWidths::getWidth(char *name, Gushort *width) {
  int h;
  BuiltinFontWidth *p;

  h = hash(name);
  for (p = tab[h]; p; p = p->next) {
    if (!strcmp(p->name, name)) {
      *width = p->width;
      return gTrue;
    }
  }
  return gFalse;
}

int BuiltinFontWidths::hash(char *name) {
  char *p;
  unsigned int h;

  h = 0;
  for (p = name; *p; ++p) {
    h = 17 * h + (int)(*p & 0xff);
  }
  return (int)(h % size);
}
