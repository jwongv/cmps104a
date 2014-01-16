// $Id: stringset.h,v 1.2 2013-10-14 22:24:58-07 - - $
//Jacky Wong
//jagwong@ucsc.edu
//ID: 1281439

#ifndef __STRINGSET__
#define __STRINGSET__

#include <string>
#include <unordered_set>
using namespace std;

#include <stdio.h>

const string* intern_stringset (const char*);

void dump_stringset (FILE*);

#endif
