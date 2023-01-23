//========================================================================
//
// ltkbuild.cc
//
// Read an LTKbuild file from stdin and write C++ window construction
// function(s) to stdout.
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#define VERSION "0.7a"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <gtypes.h>
#include <gmem.h>
#include <GString.h>

enum ArgKind {
  argVal,			// arg with value
  argSel,			// selection arg
  argLastSel			// last in list of selections
};

struct ArgDesc {
  char *tag;			// tag used in ltk file
  ArgKind kind;			// kind of arg
  GBool required;
  char *val;			// default for argVal; value for argSel
};

struct BlockDesc {
  char *name;			// name used in ltk file
  char *type;			// corresponding C++ type
  ArgDesc *args;		// list of legal args
};

struct Arg {
  char s[256];
};

struct Block {
  char *name;
  char *type;
  Arg *args;
  int numArgs;
};

#include "ltkbuild-widgets.h"

static GBool readTopLevel();
static GBool readWindow(Block *block);
static void readBox(int indent);
static void readWidget(int indent);
static GBool readMenu(Block *block);
static void readMenuItem(int indent);
static Block *readBlock(BlockDesc *tab, char *err);
static void freeBlock(Block *block);
static void initLexer();
static char *getToken();
static GBool skipSpace();
static GBool checkToken(char *s, char *msg = NULL);
static void error(char *msg, ...);

static char line[256];
static char *nextChar;
static int lineNum;
static char tokenBuf[256];
static int numErrors;

//------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  initLexer();
  printf("// This file was generated by ltkbuild %s\n\n", VERSION);
  while (readTopLevel()) ;
  if (numErrors > 0)
    return 1;
  return 0;
}

//------------------------------------------------------------------------

static GBool readTopLevel() {
  Block *block;
  GBool ok;

  if (!(block = readBlock(topLevelTab, "top level block")))
    return gFalse;
  if (!strcmp(block->type, windowType)) {
    ok = readWindow(block);
  } else if (!strcmp(block->type, menuType)) {
    ok = readMenu(block);
  } else {
    error("Internal: top-level is not Window or Menu");
    freeBlock(block);
    ok = gFalse;
  }
  return ok;
}

static GBool readWindow(Block *block) {
  int i;

  printf("%s *%s(LTKApp *app) {\n", block->type, block->args[0].s);
  printf("  return new %s(app, ", block->type);
  for (i = 1; i < block->numArgs; ++i) {
    printf("%s,%c", block->args[i].s,
	   i < block->numArgs - 1 ? ' ' : '\n');
  }
  freeBlock(block);

  checkToken("{");
  readBox(4);
  checkToken("}");
  printf("\n");

  printf("  );\n");
  printf("}\n\n");

  return gTrue;
}

static void readBox(int indent) {
  Block *block;
  int x, y;
  int i, j;

  if (!(block = readBlock(boxTab, "box block")))
    return;
  x = atoi(block->args[boxArgX].s);
  y = atoi(block->args[boxArgY].s);

  printf("%*snew %s(", indent, "", block->type);
  for (i = 0; i < block->numArgs; ++i) {
    printf("%s,%c", block->args[i].s,
	   i < block->numArgs - 1 ? ' ' : '\n');
  }
  freeBlock(block);

  checkToken("{");
  for (i = 0; i < x; ++i) {
    for (j = 0; j < y; ++j) {
      readWidget(indent + 2);
      if (i == x-1 && j == y-1)
	printf("\n");
      else
	printf(",\n");
    }
  }
  checkToken("}", "wrong box size?");
  printf("%*s)", indent, "");
}

static void readWidget(int indent) {
  Block *block;
  int x, y;
  int i, j;

  if (!(block = readBlock(widgetTab, "widget block")))
    return;

  printf("%*snew %s(", indent, "", block->type);
  for (i = 0; i < block->numArgs; ++i) {
    printf("%s%s", block->args[i].s,
	   i < block->numArgs - 1 ? ", " : "");
  }

  if (!strcmp(block->type, boxType)) {
    x = atoi(block->args[boxArgX].s);
    y = atoi(block->args[boxArgY].s);
    printf(",\n");
    checkToken("{");
    for (i = 0; i < x; ++i) {
      for (j = 0; j < y; ++j) {
	readWidget(indent + 2);
	if (i == x-1 && j == y-1)
	  printf("\n");
	else
	  printf(",\n");
      }
    }
    checkToken("}", "wrong box size?");
    printf("%*s)", indent, "");
  } else {
    printf(")");
  }
  freeBlock(block);
}

static GBool readMenu(Block *block) {
  int n, i;

  printf("%s *%s() {\n", block->type, block->args[0].s);
  printf("  return new %s(", block->type);
  for (i = 1; i < block->numArgs; ++i) {
    printf("%s,%c", block->args[i].s,
	   i < block->numArgs - 1 ? ' ' : '\n');
  }
  n = atoi(block->args[menuArgN].s);
  freeBlock(block);

  checkToken("{");
  for (i = 0; i < n; ++i) {
    readMenuItem(4);
    if (i < n - 1)
      printf(",\n");
    else
      printf("\n");
  }
  checkToken("}", "wrong menu size?");
  printf("\n");

  printf("  );\n");
  printf("}\n\n");

  return gTrue;
}

static void readMenuItem(int indent) {
  Block *block;
  int n, i;

  if (!(block = readBlock(menuItemTab, "menu item block")))
    return;

  printf("%*snew %s(", indent, "", block->type);
  for (i = 1; i < block->numArgs; ++i) {
    printf("%s,", block->args[i].s);
    if (i < block->numArgs - 1)
      printf(" ");
  }
  n = atoi(block->args[0].s);
  freeBlock(block);

  if (n > 0) {
    printf("\n");
    printf("%*snew %s(NULL, %d,\n", indent + 2, "", menuType, n);
    checkToken("{");
    for (i = 0; i < n; ++i) {
      readMenuItem(indent + 4);
      if (i < n - 1)
	printf(",\n");
      else
	printf("\n");
    }
    checkToken("}", "wrong menu size?");
    printf("%*s)\n", indent + 2, "");
    printf("%*s)", indent, "");

  } else {
    printf(" NULL)");
  }
}

static Block *readBlock(BlockDesc *tab, char *err) {
  Block *block;
  char *name, *tag, *val;
  BlockDesc *bd;
  ArgDesc *ad;
  int n;
  GBool isVal;

  // get name and find block descriptor
  if (!(name = getToken()))
    return NULL;
  for (bd = tab; bd->name; ++bd) {
    if (!strcmp(name, bd->name))
      break;
  }
  if (!bd->name) {
    error("Expected %s, got '%s'", err, name);
    return NULL;
  }

  // skip paren
  checkToken("(");

  // allocate block
  block = (Block *)gmalloc(sizeof(Block));
  block->name = bd->name;
  block->type = bd->type;

  // count args and allocate array
  block->numArgs = 0;
  for (ad = bd->args; ad->tag; ++ad) {
    if (ad->kind == argVal || ad->kind == argLastSel)
      ++block->numArgs;
  }
  block->args = (Arg *)gmalloc(block->numArgs * sizeof(Arg));

  // initialize args to defaults
  n = 0;
  isVal = gTrue;
  for (ad = bd->args; ad->tag; ++ad) {
    if (ad->kind == argVal) {
      strcpy(block->args[n++].s, ad->val);
      isVal = gTrue;
    } else if (isVal && ad->kind == argSel) {
      strcpy(block->args[n++].s, ad->val);
      isVal = gFalse;
    } else if (ad->kind == argLastSel) {
      isVal = gTrue;
    }
  }

  // read args
  while (1) {
    if (!(tag = getToken()))
      break;
    if (!strcmp(tag, ")"))
      break;
    n = strlen(tag);
    if (tag[n-1] == ':') {
      tag[n-1] = '\0';
      isVal = gTrue;
    } else {
      isVal = gFalse;
    }
    n = 0;
    for (ad = bd->args; ad->tag; ++ad) {
      if (!strcmp(ad->tag, tag))
	break;
      if (ad->kind == argVal || ad->kind == argLastSel)
	++n;
    }
    if (ad->tag) {
      if (isVal) {
	val = getToken();
	if (ad->kind != argVal)
	  error("Tag '%s' in '%s' block is not a value tag", tag, block->name);
      } else {
	val = ad->val;
	if (ad->kind == argVal)
	  error("Tag '%s' in '%s' block is a value tag", tag, block->name);
      }
      strcpy(block->args[n].s, val);
    } else {
      error("Unknown tag '%s' in '%s' block", tag, block->name);
      if (isVal)
	getToken();
    }
  }
  if (!tag)
    error("Unclosed '%s' block", block->name);

  return block;
}

static void freeBlock(Block *block) {
  gfree(block->args);
  gfree(block);
}

//------------------------------------------------------------------------

static void initLexer() {
  line[0] = '\0';
  nextChar = line;
  lineNum = 0;
  numErrors = 0;
}

static char *getToken() {
  char *p;
  int numBrackets;

  if (!skipSpace())
    return NULL;

  p = tokenBuf;

  if (*nextChar == '(' || *nextChar == ')' ||
      *nextChar == '{' || *nextChar == '}') {
    *p++ = *nextChar++;
    *p = '\0';

  } else if (*nextChar == '"') {
    *p++ = *nextChar;
    do {
      ++nextChar;
      if (!*nextChar)
	break;
      *p++ = *nextChar;
    } while (!(*nextChar == '"' && nextChar[-1] != '\\'));
    *p = '\0';
    if (*nextChar == '"')
      ++nextChar;
    else
      error("Unclosed quoted string");

  } else if (*nextChar == '[') {
    numBrackets = 1;
    while (1) {
      ++nextChar;
      if (!*nextChar)
	break;
      if (*nextChar == '[' && nextChar[-1] != '\\')
	++numBrackets;
      else if (*nextChar == ']' && nextChar[-1] != '\\')
	--numBrackets;
      if (numBrackets == 0)
	break;
      *p++ = *nextChar;
    }
    *p = '\0';
    if (*nextChar == ']')
      ++nextChar;
    else
      error("Unclosed bracketed expression");

  } else {
    while (!isspace(*nextChar) &&
	   *nextChar != '(' && *nextChar != ')' &&
	   *nextChar != '{' && *nextChar != '}' &&
	   *nextChar != ':' && *nextChar)
      *p++ = *nextChar++;
    if (*nextChar == ':')
      *p++ = *nextChar++;
    *p = '\0';
  }

  return tokenBuf;
}

static GBool skipSpace() {
  while (1) {
    if (!*nextChar || *nextChar == '#') {
      if (!fgets(line, sizeof(line), stdin)) {
	line[0] = '\0';
	nextChar = line;
	return gFalse;
      }
      ++lineNum;
      nextChar = line;
    } else if (isspace(*nextChar)) {
      ++nextChar;
    } else {
      break;
    }
  }
  return gTrue;
}

static GBool checkToken(char *s, char *msg) {
  char *tok;

  tok = getToken();
  if (tok && !strcmp(tok, s))
    return gTrue;
  if (msg)
    error("Expected '%s', got '%s' (%s)", s, tok, msg);
  else
    error("Expected '%s', got '%s'", s, tok);
  return gFalse;
}

static void error(char *msg, ...) {
  va_list args;

  fprintf(stderr, "Error at line %d: ", lineNum);
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
  ++numErrors;
}
