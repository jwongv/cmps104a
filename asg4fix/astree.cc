
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "stringset.h"
#include "lyutils.h"
#include "symtable.h"

SymbolTable* structtab = new SymbolTable(NULL);

astree* new_astree (int symbol, int filenr, int linenr,
                    int offset, const char* lexinfo) {
   astree* tree = new astree();
   tree->symbol = symbol;
   tree->filenr = filenr;
   tree->linenr = linenr;
   tree->offset = offset;
   tree->lexinfo = intern_stringset (lexinfo);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}

astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}

astree* adopt1sym (astree* root, astree* child, int symbol) {
   root = adopt1 (root, child);
   root->symbol = symbol;
   return root;
}


static void dump_node (FILE* outfile, astree* node) {
//   fprintf (outfile, "%p->{%s(%d) %ld:%ld.%03ld \"%s\" [",
//            node, get_yytname (node->symbol), node->symbol,
//            node->filenr, node->linenr, node->offset,
//            node->lexinfo->c_str());
   fprintf (outfile, "%3ld  %ld.%03ld  %-3d  %-15s (%s)  ",
            node->filenr, node->linenr, node->offset, node->symbol,
            get_yytname (node->symbol), node->lexinfo->c_str()); 
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
        ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
      fprintf (outfile, "%p", node->children.at(child));
   }
//   fprintf (outfile, "]}");
}

static void dump_astree_rec (FILE* outfile, astree* root,
                             int depth) {
   if (root == NULL) return;
//   fprintf (outfile, "%*s%s ", depth * 3, "",
//            root->lexinfo->c_str());
   dump_node (outfile, root);
   fprintf (outfile, "\n");
   for (size_t child = 0; child < root->children.size();
        ++child) {
      dump_astree_rec (outfile, root->children[child],
                       depth + 1);
   }
}

static void dump_astree_rec_new (FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
   int i = 0;
      while (i < depth){
         fprintf(outfile, "   ");
         i++;
      }
      fprintf(outfile, "%s", get_yytname(root->symbol));
      if(strcmp(root->lexinfo->c_str(), "") != 0)
         fprintf(outfile, " (%s)", root->lexinfo->c_str());
      fprintf(outfile, "\n");
   for (size_t child = 0; child < root->children.size(); ++child){
      dump_astree_rec_new (outfile, root->children[child], depth +1);
   }
}

void dump_astree (FILE* outfile, astree* root) {
   dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}

void dump_astree_new(FILE* outfile, astree*root) {
   dump_astree_rec_new (outfile, root, 0);
   fflush (NULL);
}

void scan_tree(astree* root, SymbolTable* symtab){
   if(root == NULL) return;
   for(size_t child = 0; child < root->children.size(); ++child){
      const char* currsymbol = get_yytname(root->children[child]->symbol);
      if(strcmp(currsymbol, "vardecl") != 0 && strcmp(currsymbol, "block") != 0 && strcmp(currsymbol, "function") != 0 && strcmp(currsymbol, "struct_") != 0)
         scan_tree(root->children[child], symtab);
      else if(strcmp(currsymbol, "vardecl") == 0){                          //compare node name and check if its a vardecl or a block??
         if(root->children[child]->children[0]->children[1] == NULL){
            symtab->addSymbol(root->children[child]->children[1]->lexinfo->c_str(),                            //variable identifier
                              root->children[child]->children[0]->children[0]->children[0]->lexinfo->c_str(),  //variable type
                              root->children[child]->children[1]->filenr,
                              root->children[child]->children[1]->linenr,
                              root->children[child]->children[1]->offset); 
         }else{
            string vartype = root->children[child]->children[0]->children[0]->children[0]->lexinfo->c_str();
            vartype = vartype + "[]";
            symtab->addSymbol(root->children[child]->children[1]->lexinfo->c_str(),
                              vartype,
                              root->children[child]->children[1]->filenr,
                              root->children[child]->children[1]->linenr,
                              root->children[child]->children[1]->offset);

         }
      }else if (strcmp(currsymbol, "struct_") == 0){
//         SymbolTable* structtab = new SymbolTable(NULL);
         string tablename;
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){
            string currchild = get_yytname(root->children[child]->children[child1]->symbol);
            if(currchild == "TOK_IDENT"){
               tablename = root->children[child]->children[child1]->lexinfo->c_str();
            }
         }
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){
            string currchild = get_yytname(root->children[child]->children[child1]->symbol);
            if(currchild == "decl"){
               structtab->addSymbol(tablename, root->children[child]->children[child1]->children[1]->lexinfo->c_str(),
                                    root->children[child]->children[child1]->children[1]->filenr,
                                    root->children[child]->children[child1]->children[1]->linenr,
                                    root->children[child]->children[child1]->children[1]->offset);
            }
         }
//         structtab->dump(stdout,0);
         

      }else if(strcmp(currsymbol, "block") == 0){       //if there is a block, then create a new block and start scanning that one
         scan_tree(root->children[child], root->children[child]->blockpt = symtab->enterBlock()); 
      }else if(strcmp(currsymbol, "function") == 0){       //found function
         char ident[50];
         char typedecl[100];
         int blocknum;
         int numdecl = 0;
         int funcfilenr = 0;
         int funclinenr = 0;
         int funcoffset = 0;
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){         //for all child nodes of function
            const char* currinnersymbol = get_yytname(root->children[child]->children[child1]->symbol);
            if(strcmp(currinnersymbol, "TOK_IDENT") == 0){   //find function identifier
               funcfilenr = root->children[child]->children[child1]->filenr;
               funclinenr = root->children[child]->children[child1]->linenr;
               funcoffset = root->children[child]->children[child1]->offset;
               strcpy(ident, root->children[child]->children[child1]->lexinfo->c_str());
            }
            if(strcmp(currinnersymbol, "type") == 0){    //add type to the type/decl string
               strcpy(typedecl, root->children[child]->children[child1]->children[0]->children[0]->lexinfo->c_str());
               strcat(typedecl, "(");
            }
            if(strcmp(currinnersymbol, "block") == 0)
               blocknum = child1;
         }
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){            //get function decl types
            if(strcmp(get_yytname(root->children[child]->children[child1]->symbol), "decl") == 0){
               if(numdecl != 0)
                  strcat(typedecl, ",");
               strcat(typedecl,root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str());
               ++numdecl;
            }
         }
         strcat(typedecl, ")");
         SymbolTable* funcsym = symtab->enterFunction(ident, typedecl, funcfilenr, funclinenr, funcoffset);
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){           //add func declarations
            if(strcmp(get_yytname(root->children[child]->children[child1]->symbol), "decl") == 0)
               funcsym->addSymbol(root->children[child]->children[child1]->children[1]->lexinfo->c_str(),
                                  root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str(),
                                  root->children[child]->children[child1]->children[1]->filenr,
                                  root->children[child]->children[child1]->children[1]->linenr,
                                  root->children[child]->children[child1]->children[1]->offset);
         }
         scan_tree(root->children[child]->children[blocknum], root->children[child]->children[blocknum]->blockpt = funcsym);
    }
  //    else
  //       scan_tree(root->children[child], symtab);
   }

}

string type_check(astree* root, SymbolTable* symtab){
   SymbolTable* currtable;
   if(root == NULL) return NULL;
   string symbolname = get_yytname(root->symbol);
   if(symbolname == "program") {
      for(size_t child = 0; child < root->children.size(); ++child){
         if(strcmp(get_yytname(root->children[child]->symbol), "block") == 0){
            currtable = root->children[child]->blockpt;
            type_check(root->children[child], currtable);
         }else
            type_check(root->children[child], symtab);
      }
   }

   if(symbolname == "block"){
      for(size_t child = 0; child < root->children.size(); ++child){
         type_check(root->children[child], root->children[child]->blockpt);
      }
   }

   if(symbolname == "while_" || symbolname == "if_"){
      if(type_check(root->children[0],symtab) != "bool") errprintf("Expression does not return a bool!\n");
 //        return "";
//      else printf("Expression does not return a bool!\n");
      type_check(root->children[1], symtab);
      if(root->children[2] != NULL)
         type_check(root->children[2],symtab);
   }
//   string symbolname = get_yytname(root->symbol);
   if(symbolname == "constant") return type_check(root->children[0],symtab);    //if found constant, typecheck it
   if(symbolname == "TOK_INTCON") return "int";                         //if found integer constant, return int
   if(symbolname == "TOK_CHARCON") return "char";
   if(symbolname == "TOK_STRINGCON") return "string";
   if(symbolname == "TOK_INT") return "int";
   if(symbolname == "TOK_CHAR") return "char";
   if(symbolname == "TOK_STRING") return "string";
   if(symbolname == "TOK_BOOL") return "bool";
   if(symbolname == "TOK_FALSE" || symbolname == "TOK_TRUE") return "bool";
   if(symbolname == "TOK_NULL") return "null";
/*
   if(symbolname == "TOK_NEG") return type_check(root->children[0],symtab);
   if(symbolname == "TOK_POS") return type_check(root->children[0],symtab);
   if(symbolname == "'!'") return type_check(root->children[0],symtab);
   if(symbolname == "TOK_CHR") return type_check(root->children[0],symtab);
   if(symbolname == "TOK_ORD") return type_check(root->children[0],symtab);
*/
   if(symbolname == "return_"){
      if(root->children[0] != NULL)
         return type_check(root->children[0],symtab);
      else return "";
   }
   if(symbolname == "TOK_IDENT") return symtab->lookup(root->lexinfo->c_str());  //if found an identifier, return its type
   if(symbolname == "variable"){
      if(root->children[1] == NULL) return type_check(root->children[0],symtab);
      else{
         string child1 = get_yytname(root->children[1]->children[0]->symbol);
         if(child1 == "TOK_INTCON") return "char";
         else if(type_check(root->children[1],symtab) == "basetype[]") return "basetype";
         if(structtab->lookup(root->children[0]->children[0]->lexinfo->c_str()) != root->children[1]->lexinfo->c_str())
            errprintf("Not an element of this struct\n");
      }

   }
   if(symbolname == "type") return type_check(root->children[0], symtab);
   if(symbolname == "basetype") return type_check(root->children[0], symtab);

   if(symbolname == "allocator_"){
      if(root->children[2] != NULL){
         if(type_check(root->children[2], symtab) == "int") return "basetype[]";
      }else return "basetype";
   }


   if(symbolname == "vardecl"){
      string child2 = get_yytname(root->children[2]->symbol);
//      printf("%s++++%s\n",type_check(root->children[0], symtab).c_str(), type_check(root->children[2],symtab).c_str());
      if(child2 != "allocator_"){
         if(type_check(root->children[0],symtab) == type_check(root->children[2],symtab)) return "";
         else errprintf("Declaring incorrect type!\n");
      }
      else{ 
         if (root->children[0]->children[1] != NULL && child2 == "allocator_") return "";
         else if(type_check(root->children[0],symtab) == type_check(root->children[2]->children[1]->children[0], symtab)) return "";
         else errprintf("Declaring incorrect type!\n");
      }
   }
   //BINARY OPERATORS
   if(symbolname == "binop"){
      string child1 = get_yytname(root->children[1]->symbol);
      if(child1 == "'+'" || child1 == "'-'" || child1 == "'*'" || child1 == "'/'" || child1 == "'%'"){
         if(type_check(root->children[0], symtab) == "int" && type_check(root->children[0], symtab) == type_check(root->children[2], symtab))
            return "int";        
         else
            errprintf("One or more inputs is not of type int\n");
      }
      if(child1 == "TOK_EQ" || child1 == "TOK_NE" || child1 == "TOK_LT" || child1 == "TOK_LE" || child1 == "TOK_GT" || child1 == "TOK_GE"){
         if(type_check(root->children[0], symtab) == type_check(root->children[2], symtab))
            return "bool";
         else
            errprintf("Comparison of different types!\n");
      } 
      if(strcmp(get_yytname(root->children[1]->symbol), "'='") == 0){
         if(type_check(root->children[0], symtab) == type_check(root->children[2], symtab))
            return type_check(root->children[0], symtab);
         else
            errprintf("Trying to set variable to different type!\n");
      }
   }

   //UNARY OPERATORS
   if(symbolname == "unop"){
      string child0 = get_yytname(root->children[0]->symbol);
      if(child0 == "TOK_POS" || child0 == "TOK_NEG"){
         if(type_check(root->children[0]->children[0], symtab) == "int") return "int";
         else errprintf("Tried adding sign to non-int type!\n");
      }
      if(child0 == "'!'"){
         if(type_check(root->children[0]->children[0], symtab) == "bool") return "bool";
         else errprintf("Tried negating a non-bool!\n");
      }
      if(child0 == "TOK_ORD"){
   //      errprintf("%s\n", type_check(root->children[0]->children[0],symtab).c_str());
         if(type_check(root->children[0]->children[0], symtab) == "char") return "int";
         else errprintf("Tried using ord on non-char type!\n");
      }
      if(child0 == "TOK_CHR"){
         if(type_check(root->children[0]->children[0], symtab) == "int") return "char";
         else errprintf("Tried using chr on non-int type!\n");
      }
   }

   if(symbolname == "function"){
      string funcreturn;
      string returntype;
      SymbolTable* currblock;
      for(size_t child = 0; child < root->children.size(); ++child){
         string currchild = get_yytname(root->children[child]->symbol);
         if(currchild == "TOK_IDENT"){
            funcreturn = symtab->lookup(root->children[child]->lexinfo->c_str());
         }
         if(currchild == "block") currblock = root->children[child]->blockpt;
      }
      astree* returnroot = funcsearch(root);
//    if(returnroot == NULL) printf("hi\n");   
      if(returnroot->children[0] == NULL) 
         returntype = "void";
      else returntype = type_check(returnroot, currblock);
//      errprintf("%s\n", funcreturn.substr(0,3).c_str());
      if(returntype != funcreturn.substr(0,returntype.length())){
         errprintf("Incorrect return type!\n");
      }
   }

   if(symbolname == "call"){
      string funcoper = "(";
      string funcname;
      string funcops;
      size_t check = -1;
      int begin;
      int comcounter = 0;
      int firstparen;
      for(size_t child = 0; child < root->children.size(); ++child){
         string currchild = get_yytname(root->children[child]->symbol);
         if(currchild == "variable"){
            if (comcounter != 0) funcoper = funcoper + ",";
            string currop = type_check(root->children[child], symtab);
            funcoper = funcoper + currop;
            comcounter++;
         }
         if(currchild == "TOK_IDENT") funcname = root->children[child]->lexinfo->c_str();
      }
      funcoper = funcoper + ")";
      funcops = symtab->lookup(funcname);
//      printf("%s\n", funcops.c_str());
      begin = funcops.length() - funcoper.length();
      if(funcops.find(funcoper) == check)
         errprintf("Incorrect arguments passed to function!\n");
      firstparen = funcops.find_first_of('(', 0);
      return funcops.substr(0,firstparen);
   }









   return "";

}

astree* funcsearch(astree* root){
  if(root == NULL) return NULL;
  astree* retnode;
  for(size_t child = 0; child < root->children.size(); ++child){
     string currsym = get_yytname(root->children[child]->symbol);
     if(currsym == "return_"){
        return root->children[child];
     }
  }
  for(size_t child = 0; child < root->children.size(); ++child){
     if((retnode = funcsearch(root->children[child])) != NULL){
        return retnode; //funcsearch(root->children[child]);
     } 
  }
  return NULL;

}



void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep) {
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      fprintf (outfile, "%s(%d)\n",
               get_yytname (toknum), toknum);
   }
   fflush (NULL);
}


void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%p]-> %d:%d.%d: %s: \"%s\")\n",
           root, root->filenr, root->linenr, root->offset,
           get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}

void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}

RCSC("$Id: astree.cc,v 1.1 2013-10-28 03:34:20-07 - - $")

