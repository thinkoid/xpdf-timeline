//========================================================================
//
// pdftops.cc
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "parseargs.h"
#include "GString.h"
#include "gmem.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "PSOutputDev.h"
#include "Error.h"
#include "config.h"

static int firstPage = 1;
static int lastPage = 0;
static GBool level1 = gFalse;
static GBool level1Sep = gFalse;
static GBool level2Sep = gFalse;
static GBool doEPS = gFalse;
static GBool doForm = gFalse;
#if OPI_SUPPORT
static GBool doOPI = gFalse;
#endif
static GBool noEmbedT1Fonts = gFalse;
static GBool noEmbedTTFonts = gFalse;
static char paperSize[15] = "";
static int paperWidth = 0;
static int paperHeight = 0;
static GBool duplex = gFalse;
static char ownerPassword[33] = "";
static char userPassword[33] = "";
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

static ArgDesc argDesc[] = {
  {"-f",      argInt,      &firstPage,      0,
   "first page to print"},
  {"-l",      argInt,      &lastPage,       0,
   "last page to print"},
  {"-level1", argFlag,     &level1,         0,
   "generate Level 1 PostScript"},
  {"-level1sep", argFlag,  &level1Sep,      0,
   "generate Level 1 separable PostScript"},
  {"-level2sep", argFlag,  &level2Sep,      0,
   "generate Level 2 separable PostScript"},
  {"-eps",    argFlag,     &doEPS,          0,
   "generate Encapsulated PostScript (EPS)"},
  {"-form",   argFlag,     &doForm,         0,
   "generate a PostScript form"},
#if OPI_SUPPORT
  {"-opi",    argFlag,     &doOPI,          0,
   "generate OPI comments"},
#endif
  {"-noembt1", argFlag,     &noEmbedT1Fonts, 0,
   "don't embed Type 1 fonts"},
  {"-noembtt", argFlag,    &noEmbedTTFonts, 0,
   "don't embed TrueType fonts"},
  {"-paper",  argString,   paperSize,       sizeof(paperSize),
   "paper size (letter, legal, A4, A3)"},
  {"-paperw", argInt,      &paperWidth,     0,
   "paper width, in points"},
  {"-paperh", argInt,      &paperHeight,    0,
   "paper height, in points"},
  {"-duplex", argFlag,     &duplex,         0,
   "enable duplex printing"},
  {"-opw",    argString,   ownerPassword,   sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",    argString,   userPassword,    sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-q",      argFlag,     &quiet,          0,
   "don't print any messages or errors"},
  {"-cfg",        argString,      cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",      argFlag,     &printVersion,   0,
   "print copyright and version info"},
  {"-h",      argFlag,     &printHelp,      0,
   "print usage information"},
  {"-help",   argFlag,     &printHelp,      0,
   "print usage information"},
  {"--help",  argFlag,     &printHelp,      0,
   "print usage information"},
  {"-?",      argFlag,     &printHelp,      0,
   "print usage information"},
  {NULL}
};

int main(int argc, char *argv[]) {
  PDFDoc *doc;
  GString *fileName;
  GString *psFileName;
  PSLevel level;
  PSOutMode mode;
  GString *ownerPW, *userPW;
  PSOutputDev *psOut;
  GBool ok;
  char *p;

  // parse args
  ok = parseArgs(argDesc, &argc, argv);
  if (!ok || argc < 2 || argc > 3 || printVersion || printHelp) {
    fprintf(stderr, "pdftops version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdftops", "<PDF-file> [<PS-file>]", argDesc);
    }
    exit(1);
  }
  if ((level1 && level1Sep) ||
      (level1Sep && level2Sep) ||
      (level1 && level2Sep)) {
    fprintf(stderr, "Error: use only one of -level1, -level1sep, and -level2sep.\n");
    exit(1);
  }
  if (doEPS && doForm) {
    fprintf(stderr, "Error: use only one of -eps and -form\n");
    exit(1);
  }
  if (doForm && (level1 || level1Sep)) {
    fprintf(stderr, "Error: forms are only available with Level 2 output.\n");
    exit(1);
  }
  fileName = new GString(argv[1]);
  level = level1 ? psLevel1
                 : level1Sep ? psLevel1Sep
                             : level2Sep ? psLevel2Sep
                                         : psLevel2;
  mode = doEPS ? psModeEPS
               : doForm ? psModeForm
                        : psModePS;

  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (paperSize[0]) {
    if (!globalParams->setPSPaperSize(paperSize)) {
      fprintf(stderr, "Invalid paper size\n");
      exit(1);
    }
  } else {
    if (paperWidth) {
      globalParams->setPSPaperWidth(paperWidth);
    }
    if (paperHeight) {
      globalParams->setPSPaperHeight(paperHeight);
    }
  }
  if (duplex) {
    globalParams->setPSDuplex(duplex);
  }
  if (level1 || level1Sep || level2Sep) {
    globalParams->setPSLevel(level);
  }
  if (noEmbedT1Fonts) {
    globalParams->setPSEmbedType1(!noEmbedT1Fonts);
  }
  if (noEmbedTTFonts) {
    globalParams->setPSEmbedTrueType(!noEmbedTTFonts);
  }
#if OPI_SUPPORT
  if (doOPI) {
    globalParams->setPSOPI(doOPI);
  }
#endif
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }

  // open PDF file
  if (ownerPassword[0]) {
    ownerPW = new GString(ownerPassword);
  } else {
    ownerPW = NULL;
  }
  if (userPassword[0]) {
    userPW = new GString(userPassword);
  } else {
    userPW = NULL;
  }
  doc = new PDFDoc(fileName, ownerPW, userPW);
  if (userPW) {
    delete userPW;
  }
  if (ownerPW) {
    delete ownerPW;
  }
  if (!doc->isOk()) {
    goto err1;
  }

  // check for print permission
  if (!doc->okToPrint()) {
    error(-1, "Printing this document is not allowed.");
    goto err1;
  }

  // construct PostScript file name
  if (argc == 3) {
    psFileName = new GString(argv[2]);
  } else {
    p = fileName->getCString() + fileName->getLength() - 4;
    if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")) {
      psFileName = new GString(fileName->getCString(),
			       fileName->getLength() - 4);
    } else {
      psFileName = fileName->copy();
    }
    psFileName->append(doEPS ? ".eps" : ".ps");
  }

  // get page range
  if (firstPage < 1) {
    firstPage = 1;
  }
  if (lastPage < 1 || lastPage > doc->getNumPages()) {
    lastPage = doc->getNumPages();
  }

  // check for multi-page EPS or form
  if ((doEPS || doForm) && firstPage != lastPage) {
    error(-1, "EPS and form files can only contain one page.");
    goto err2;
  }

  // write PostScript file
  psOut = new PSOutputDev(psFileName->getCString(), doc->getXRef(),
			  doc->getCatalog(), firstPage, lastPage, mode);
  if (psOut->isOk()) {
    doc->displayPages(psOut, firstPage, lastPage, 72, 0, gFalse);
  }
  delete psOut;

  // clean up
 err2:
  delete psFileName;
 err1:
  delete doc;
  delete globalParams;

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return 0;
}
