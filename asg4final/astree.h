#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"
#include "symtable.h"

struct astree {
   int symbol;               // token code
   size_t filenr;            // index into filename stack
   size_t linenr;            // line number from source code
   size_t offset;            // offset of token with current line
   SymbolTable* blockpt;
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node
};


astree* new_astree (int symbol, int filenr, int linenr,
                    int offset, const char* lexinfo);
astree* adopt1 (astree* root, astree* child);
astree* adopt2 (astree* root, astree* left, astree* right);
astree* adopt1sym (astree* root, astree* child, int symbol);
void dump_astree (FILE* outfile, astree* root);
void dump_astree_new (FILE* outfile, astree* root);
void scan_tree (astree* root, SymbolTable* symtab);
string type_check (astree* root, SymbolTable* symtab);
astree* funcsearch(astree* root);
bool funcsearch2(astree* root, string type, SymbolTable* symtab);
void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep);
void free_ast (astree* tree);
void free_ast2 (astree* tree1, astree* tree2);

RCSH("$Id: astree.h,v 1.1 2013-10-28 03:34:20-07 - - $")
#endif
