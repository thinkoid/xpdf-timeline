//========================================================================
//
// Params.h
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#ifndef PARAMS_H
#define PARAMS_H

// Size of RGB color cube.
extern int rgbCubeSize;

// Print commands as they're executed.
extern GBool printCommands;

// Send error messages to /dev/tty instead of stderr.
extern GBool errorsToTTY;

// Font search path.
extern char **fontPath;

// Mapping from PDF font name to device font name.
struct DevFontMapEntry {
  char *pdfFont;
  char *devFont;
};
extern DevFontMapEntry *devFontMap;

//------------------------------------------------------------------------

// Initialize font path and font map, and read configuration file,
// if present.
extern void initParams(char *configFile);

// Free memory used for font path and font map.
extern void freeParams();

#endif