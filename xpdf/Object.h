//========================================================================
//
// Object.h
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#ifndef OBJECT_H
#define OBJECT_H

#pragma interface

#include <stdio.h>
#include <string.h>
#include <stypes.h>
#include <mem.h>
#include "XRef.h"

class Array;
class Dict;
class Stream;

//------------------------------------------------------------------------
// Ref
//------------------------------------------------------------------------

struct Ref {
  int num;			// object number
  int gen;			// generation number
};

//------------------------------------------------------------------------
// object types
//------------------------------------------------------------------------

typedef int ObjType;

// simple objects
#define objBool     0		// boolean
#define objInt      1		// integer
#define objReal     2		// real
#define objString   3		// string
#define objName     4		// name
#define objNull     5		// null

// complex objects
#define objArray    6		// array
#define objDict     7		// dictionary
#define objStream   8		// stream
#define objRef      9		// indirect reference

// special objects
#define objCmd      10		// command name
#define objError    11		// error return from Lexer
#define objEOF      12		// end of file return from Lexer
#define objNone     13		// uninitialized object

// misc
#define numObjTypes 14		// number of object types

//------------------------------------------------------------------------
// Object
//------------------------------------------------------------------------

class Object {
public:

  // Default constructor.
  Object():
    type(objNone) {}

  // Initialize an object.
  Object *initBool(Boolean booln1)
    { type = objBool; booln = booln1;
      ++numAlloc[objBool]; return this; }
  Object *initInt(int intg1)
    { type = objInt; intg = intg1;
      ++numAlloc[objInt]; return this; }
  Object *initReal(double real1)
    { type = objReal; real = real1;
      ++numAlloc[objReal]; return this; }
  Object *initString(char *string1)
    { type = objString; string = copyString(string1);
      ++numAlloc[objString]; return this; }
  Object *initName(char *name1)
    { type = objName; name = copyString(name1);
      ++numAlloc[objName]; return this; }
  Object *initNull()
    { type = objNull;
      ++numAlloc[objNull]; return this; }
  Object *initArray();
  Object *initDict();
  Object *initStream(Stream *stream1);
  Object *initRef(int num1, int gen1)
    { type = objRef; ref.num = num1; ref.gen = gen1;
      ++numAlloc[objRef]; return this; }
  Object *initCmd(char *cmd1)
    { type = objCmd; cmd = copyString(cmd1);
      ++numAlloc[objCmd]; return this; }
  Object *initError()
    { type = objError;
      ++numAlloc[objError]; return this; }
  Object *initEOF()
    { type = objEOF;
      ++numAlloc[objEOF]; return this; }

  // Copy an object.
  Object *copy(Object *obj);

  // If object is a Ref, fetch and return the referenced object.
  // Otherwise, return a copy of the object.
  Object *fetch(Object *obj)
    { return (type == objRef && xref) ?
	     xref->fetch(ref.num, ref.gen, obj) : copy(obj); }

  // Free object contents.
  void free();

  // Type checking.
  Boolean isBool() { return type == objBool; }
  Boolean isInt() { return type == objInt; }
  Boolean isReal() { return type == objReal; }
  Boolean isNum() { return type == objInt || type == objReal; }
  Boolean isString() { return type == objString; }
  Boolean isName() { return type == objName; }
  Boolean isNull() { return type == objNull; }
  Boolean isArray() { return type == objArray; }
  Boolean isDict() { return type == objDict; }
  Boolean isStream() { return type == objStream; }
  Boolean isRef() { return type == objRef; }
  Boolean isCmd() { return type == objCmd; }
  Boolean isError() { return type == objError; }
  Boolean isEOF() { return type == objEOF; }
  Boolean isNone() { return type == objNone; }

  // Special type checking.
  Boolean isName(char *name1)
    { return type == objName && !strcmp(name, name1); }
  Boolean isDict(char *dictType);
  Boolean isStream(char *dictType);
  Boolean isCmd(char *cmd1)
    { return type == objCmd && !strcmp(cmd, cmd1); }

  // Accessors.  NB: these assume object is of correct type.
  Boolean getBool() { return booln; }
  int getInt() { return intg; }
  double getReal() { return real; }
  double getNum() { return type == objInt ? (double)intg : real; }
  char *getString() { return string; }
  char *getName() { return name; }
  Array *getArray() { return array; }
  Dict *getDict() { return dict; }
  Stream *getStream() { return stream; }
  int getRefNum() { return ref.num; }
  int getRefGen() { return ref.gen; }

  // Array accessors.
  int arrayGetLength();
  void arrayAdd(Object *elem);
  Object *arrayGet(int i, Object *obj);
  Object *arrayGetNF(int i, Object *obj);

  // Dict accessors.
  int dictGetLength();
  void dictAdd(char *key, Object *val);
  Boolean dictIs(char *dictType);
  Object *dictLookup(char *key, Object *obj);
  Object *dictLookupNF(char *key, Object *obj);
  char *dictGetKey(int i);
  Object *dictGetVal(int i, Object *obj);
  Object *dictGetValNF(int i, Object *obj);

  // Stream accessors.
  Boolean streamIs(char *dictType);
  void streamReset();
  int streamGetChar();
  int streamGetPos();
  void streamSetPos(int pos);
  FILE *streamGetFile();
  Dict *streamGetDict();
  Boolean streamCheckHeader();

  // Output.
  char *getTypeName();
  void print(FILE *f = stdout);

  // Memory testing.
  static void memCheck(FILE *f);

private:

  ObjType type;			// object type
  union {			// value for each type:
    Boolean booln;		//   boolean
    int intg;			//   integer
    double real;		//   real
    char *string;		//   string
    char *name;			//   name
    Array *array;		//   array
    Dict *dict;			//   dictionary
    Stream *stream;		//   stream
    Ref ref;			//   indirect reference
    char *cmd;			//   command
  };

  static int			// number of each type of object
    numAlloc[numObjTypes];	//   currently allocated
};

//------------------------------------------------------------------------
// Array accessors.
//------------------------------------------------------------------------

#include "Array.h"

__inline int Object::arrayGetLength()
  { return array->getLength(); }

__inline void Object::arrayAdd(Object *elem)
  { array->add(elem); }

__inline Object *Object::arrayGet(int i, Object *obj)
  { return array->get(i, obj); }

__inline Object *Object::arrayGetNF(int i, Object *obj)
  { return array->getNF(i, obj); }

//------------------------------------------------------------------------
// Dict accessors.
//------------------------------------------------------------------------

#include "Dict.h"

__inline int Object::dictGetLength()
  { return dict->getLength(); }

__inline void Object::dictAdd(char *key, Object *val)
  { dict->add(key, val); }

__inline Boolean Object::dictIs(char *dictType)
  { return dict->is(dictType); }

__inline Boolean Object::isDict(char *dictType)
  { return type == objDict && dictIs(dictType); }

__inline Object *Object::dictLookup(char *key, Object *obj)
  { return dict->lookup(key, obj); }

__inline Object *Object::dictLookupNF(char *key, Object *obj)
  { return dict->lookupNF(key, obj); }

__inline char *Object::dictGetKey(int i)
  { return dict->getKey(i); }

__inline Object *Object::dictGetVal(int i, Object *obj)
  { return dict->getVal(i, obj); }

__inline Object *Object::dictGetValNF(int i, Object *obj)
  { return dict->getValNF(i, obj); }

//------------------------------------------------------------------------
// Stream accessors.
//------------------------------------------------------------------------

#include "Stream.h"

__inline Boolean Object::streamIs(char *dictType)
  { return stream->getDict()->is(dictType); }

__inline Boolean Object::isStream(char *dictType)
  { return type == objStream && streamIs(dictType); }

__inline void Object::streamReset()
  { stream->reset(); }

__inline int Object::streamGetChar()
  { return stream->getChar(); }

__inline int Object::streamGetPos()
  { return stream->getPos(); }

__inline void Object::streamSetPos(int pos)
  { stream->setPos(pos); }

__inline FILE *Object::streamGetFile()
  { return stream->getFile(); }

__inline Dict *Object::streamGetDict()
  { return stream->getDict(); }

__inline Boolean Object::streamCheckHeader()
  { return stream->checkHeader(); }

#endif
