
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
FILE* oiloutputfile;
int icount = 1;
int bcount = 1;
int pcount = 1;
int controlcount = 1;


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
         if(root->children[child]->children[0]->children.size() < 2){//[1] == NULL){
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
         SymbolTable* currtab;
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){
            string currchild = get_yytname(root->children[child]->children[child1]->symbol);
            if(currchild == "TOK_IDENT"){
               tablename = root->children[child]->children[child1]->lexinfo->c_str();
               structtab->addStruct(tablename);
               currtab = structtab->lookup2(tablename);
            }
         }
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){
            string currchild = get_yytname(root->children[child]->children[child1]->symbol);
            if(currchild == "decl"){
               currtab->addSymbol(  root->children[child]->children[child1]->children[1]->lexinfo->c_str(),
                                    root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str(),
                                    root->children[child]->children[child1]->children[1]->filenr,
                                    root->children[child]->children[child1]->children[1]->linenr,
                                    root->children[child]->children[child1]->children[1]->offset);
            }
         }
//           currtab->dump(stdout,0);
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
               if(root->children[child]->children[child1]->children.size() == 2){
               strcpy(typedecl, root->children[child]->children[child1]->children[0]->children[0]->lexinfo->c_str());
               strcat(typedecl, "[]");
               strcat(typedecl, "(");
               }else{
               strcpy(typedecl, root->children[child]->children[child1]->children[0]->children[0]->lexinfo->c_str());
               strcat(typedecl, "(");
               }
            }
            if(strcmp(currinnersymbol, "block") == 0)
               blocknum = child1;
         }
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){            //get function decl types
            if(strcmp(get_yytname(root->children[child]->children[child1]->symbol), "decl") == 0){
               if(numdecl != 0)
                  strcat(typedecl, ",");
               if(root->children[child]->children[child1]->children[0]->children.size() == 2){
                  strcat(typedecl,root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str());
                  strcat(typedecl,"[]");
               }else{
               strcat(typedecl,root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str());
               }
               ++numdecl;
            }
         }
         strcat(typedecl, ")");
         SymbolTable* funcsym = symtab->enterFunction(ident, typedecl, funcfilenr, funclinenr, funcoffset);
         for(size_t child1 = 0; child1 < root->children[child]->children.size(); ++child1){           //add func declarations
            if(strcmp(get_yytname(root->children[child]->children[child1]->symbol), "decl") == 0){
               if(root->children[child]->children[child1]->children[0]->children.size() == 2){
                  string typearray = root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str();
                  typearray = typearray + "[]";
                  funcsym->addSymbol(root->children[child]->children[child1]->children[1]->lexinfo->c_str(),
                                     typearray,
                                     root->children[child]->children[child1]->children[1]->filenr,
                                     root->children[child]->children[child1]->children[1]->linenr,
                                     root->children[child]->children[child1]->children[1]->offset);
                }
               else{
               funcsym->addSymbol(root->children[child]->children[child1]->children[1]->lexinfo->c_str(),
                                  root->children[child]->children[child1]->children[0]->children[0]->children[0]->lexinfo->c_str(),
                                  root->children[child]->children[child1]->children[1]->filenr,
                                  root->children[child]->children[child1]->children[1]->linenr,
                                  root->children[child]->children[child1]->children[1]->offset);
               }
            }
         }
         scan_tree(root->children[child]->children[blocknum], root->children[child]->children[blocknum]->blockpt = funcsym);
    }
  //    else
  //       scan_tree(root->children[child], symtab);
   }

}

string type_check(astree* root, SymbolTable* symtab){
   if(root == NULL) return NULL;
   string symbolname = get_yytname(root->symbol);

   if(symbolname == "TOK_INT" || symbolname == "TOK_INTCON") return "int";
   if(symbolname == "TOK_CHAR" || symbolname == "TOK_CHARCON") return "char";
   if(symbolname == "TOK_STRING" || symbolname == "TOK_STRINGCON") return "string";
   if(symbolname == "TOK_BOOL" || symbolname == "TOK_TRUE" || symbolname == "TOK_FALSE") return "bool";
   if(symbolname == "TOK_NULL") return "null";

   if(symbolname == "TOK_IDENT") return symtab->lookup(root->lexinfo->c_str());
   if(symbolname == "constant") return type_check(root->children[0], symtab);
   if(symbolname == "type"){
      if(root->children.size() == 2){
         string type = type_check(root->children[0],symtab);
         type = type + "[]";
         return type;
      }
    return type_check(root->children[0], symtab);
   }
   if(symbolname == "basetype"){
      string child0 = get_yytname(root->children[0]->symbol);
      string ident = root->children[0]->lexinfo->c_str();
      if(child0 == "TOK_IDENT"){
         if (structtab->lookup2(ident) != NULL) return ident;
      }
      return type_check(root->children[0], symtab);

   }
   if(symbolname == "variable") {
      if(root->children.size() == 1) return type_check(root->children[0], symtab);
      if(root->children.size() == 2){
         string child1 = get_yytname(root->children[1]->symbol);
         SymbolTable* curstruct;
         if(child1 == "TOK_IDENT"){
            string child0 = type_check(root->children[0],symtab);//        root->children[0]->children[0]->lexinfo->c_str();
            string ident = root->children[1]->lexinfo->c_str();
            if(structtab->lookup2(child0) != NULL){
               curstruct = structtab->lookup2(child0);
               if (curstruct == NULL){ 
                  errprintf("Not a variable of this struct!\n");
                  return "";
               }
//               printf("%s\n", curstruct->lookup(ident).c_str());
               return curstruct->lookup(ident);
            }
         }else{
            if(type_check(root->children[1],symtab) == "int" && type_check(root->children[0],symtab) == "string") return "char"; 
            if(type_check(root->children[1],symtab) == "int" && type_check(root->children[0],symtab) == "int[]") return "int";
            if(type_check(root->children[1],symtab) == "int" && type_check(root->children[0],symtab) == "bool[]") return "bool";
            if(type_check(root->children[1],symtab) == "int" && type_check(root->children[0],symtab) == "char[]") return "char";
            if(type_check(root->children[1],symtab) == "int" && type_check(root->children[0],symtab) == "struct[]") return "struct";
            if(type_check(root->children[1],symtab) == "int" && type_check(root->children[0],symtab) == "string[]") return "string";
         }
      }

   }

   if(symbolname == "program"){
      for(size_t child = 0; child < root->children.size(); ++ child){
         type_check(root->children[child], symtab);
      }
   }

   if(symbolname == "block"){
      SymbolTable* currtable = root->blockpt;
      for(size_t child = 0; child < root->children.size(); ++child){
         type_check(root->children[child], currtable);
      }
   }

   if(symbolname == "vardecl"){
      string c0 = type_check(root->children[0],symtab);
//      if(root->children[0]->children.size() == 2) c0 = c0 + "[]";  ///////////////
      string c2 = type_check(root->children[2],symtab);
      if(c0 != c2) errprintf("Declaring incorrect type!\n");
      return "";
   }

   if(symbolname == "while_" || symbolname == "if_"){
      if(type_check(root->children[0],symtab) != "bool") errprintf("Expression does not return a bool!\n");
      type_check(root->children[1], symtab);
      if(root->children[2] != NULL) type_check(root->children[2], symtab);
      return "";
   }

   if(symbolname == "function"){
      SymbolTable* currblock;
      astree* blockroot;
      string funcreturn;
      for(size_t child = 0; child < root->children.size(); ++child){
         string currchild = get_yytname(root->children[child]->symbol);
         if(currchild == "type"){
            funcreturn = type_check(root->children[child], symtab);
         }
         if(currchild == "block"){
            currblock = root->children[child]->blockpt;
            blockroot = root->children[child];
         }
      }

//      string funcreturn = type_check(root->children[1], symtab);
//      printf("rawrrr%s\n", funcreturn.c_str());
      if(!funcsearch2(blockroot, funcreturn, currblock)) errprintf("Incorrect return type!\n");
   }

   if(symbolname == "allocator_"){
      if(root->children.size() < 3) return type_check(root->children[1],symtab);
      else{
         string bracket = get_yytname(root->children[3]->symbol);
         string result = type_check(root->children[1],symtab);
         if(bracket == "'['"){
            if(type_check(root->children[2],symtab) == "int"){
               result = result + "[]";
//               printf("%s\n", result.c_str());
               return result;
            }else errprintf("Need int in brackets!\n");
         }else return result;
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
         if(currchild != "TOK_IDENT"){
            if (comcounter != 0) funcoper = funcoper + ",";
            string currop = type_check(root->children[child], symtab);
            funcoper = funcoper + currop;
            comcounter++;
         }
         if(currchild == "TOK_IDENT") funcname = root->children[child]->lexinfo->c_str();
      }
      funcoper = funcoper + ")";
//      printf("%s\n", funcoper.c_str());
      funcops = symtab->lookup(funcname);
//      printf("%s\n", funcops.c_str());
      begin = funcops.length() - funcoper.length();
      if(funcops.find(funcoper) == check)
         errprintf("Incorrect arguments passed to function!\n");
      firstparen = funcops.find_first_of('(', 0);
//      printf("%s\n", funcops.substr(0,firstparen).c_str());
      return funcops.substr(0,firstparen);
   }


   if(symbolname == "binop"){
      string child1 = get_yytname(root->children[1]->symbol);
      if(child1 == "'+'" || child1 == "'-'" || child1 == "'*'" || child1 == "'/'" || child1 == "'%'"){
         if(type_check(root->children[0],symtab) == "int" && type_check(root->children[0], symtab) == type_check(root->children[2],symtab))
            return "int";
         else errprintf("One or more inputs is not of type int\n");
      }
      if(child1 == "TOK_LT" || child1 == "TOK_LE" || child1 == "TOK_GT" || child1 == "TOK_GE"){
         string c0 = type_check(root->children[0], symtab);
         string c2 = type_check(root->children[2], symtab);
         if((c0 == "int" || c0 == "char" || c0 == "bool") && c0 == c2 )
            return "bool";
         else
            errprintf("Comparison of different or incorrect types!\n");
      }
      if(child1 == "TOK_NE" || child1 == "TOK_EQ"){
         string c0 = type_check(root->children[0], symtab);
         string c2 = type_check(root->children[2], symtab);
         if(c0 == c2 || (c0 != "bool" && c0 != "int" && c0 != "char" && c2 == "null"))
            return "bool";
         else
            errprintf("Comparison of different types!\n");
      }
      if(child1 == "'='"){
         string c0 = type_check(root->children[0], symtab);
         string c2 = type_check(root->children[2], symtab);
         if(c0 == c2 || (c0 != "bool" && c0 != "int" && c0 != "char" && c2 == "null"))
            return c0;
         else
            errprintf("Trying to set variable to different type!\n");
      }
   }

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
         if(type_check(root->children[0]->children[0], symtab) == "char") return "int";
         else errprintf("Tried using ord on non-char type!\n");
      }
      if(child0 == "TOK_CHR"){
         if(type_check(root->children[0]->children[0], symtab) == "int") return "char";
         else errprintf("Tried using chr on non-int type!\n");
      }
   }



   return ""; //if it doesn't find anything return empty?

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

bool funcsearch2(astree* root, string type, SymbolTable* symtab){
   if(root == NULL) return true;
   SymbolTable* currtab = symtab;
   for(size_t child = 0; child < root->children.size(); ++child){
      string currsym = get_yytname(root->children[child]->symbol);
      if(currsym == "return_"){
         if(root->children[child]->children.size() != 0 && type == "void") return false;
         if(root->children[child]->children.size() == 1){
            string roottype = type_check(root->children[child]->children[0], symtab);
            if(roottype != type) return false; 
         }
      }
      if(currsym == "block"){
         currtab = root->children[child]->blockpt;
      }
      if(!funcsearch2(root->children[child], type, currtab)) return false;
   }
   return true;
}

void code_gen(astree* root, SymbolTable* symtab){
   if(root == NULL) return;
   string symbolname = get_yytname(root->symbol);

   if(symbolname == "program"){
      for(size_t child = 0; child < root->children.size(); ++child){
         string currsymbol = get_yytname(root->children[child]->symbol);
//         code_gen(root->children[child], symtab);
         if(currsymbol == "vardecl"){
            string child2 = get_yytname(root->children[child]->children[2]->symbol);
//            if(child2 != "constant" && child2 != "variable") code_gen(root->children[child]->children[2], symtab);
            string ident = root->children[child]->children[1]->lexinfo->c_str();
            string declt = root->children[child]->children[0]->children[0]->children[0]->lexinfo->c_str();
            string newtype = type_convert(declt);
            if(root->children[child]->children[0]->children.size() == 2){
               declt = declt + "[]";
               newtype = type_convert(declt);
               fprintf(oiloutputfile, "%s__%s;\n", newtype.c_str(), ident.c_str());
            }else{
               fprintf(oiloutputfile, "%s__%s;\n", newtype.c_str(), ident.c_str());
            }
         }
         if(currsymbol == "struct_") code_gen(root->children[child], symtab);
      }

//print ocmain 
      fprintf(oiloutputfile, "void __ocmain ()\n{\n");

      for(size_t child = 0; child < root->children.size(); ++child){
         string currsymbol = get_yytname(root->children[child]->symbol);
         if(currsymbol != "vardecl" && currsymbol != "struct_") code_gen(root->children[child], symtab);
/*         if(currsymbol == "vardecl"){
            string ident = root->children[child]->children[1]->lexinfo->c_str();
            string declt = root->children[child]->children[0]->children[0]->children[0]->lexinfo->c_str();
            string newtype = type_convert(declt);
            if(root->children[child]->children[0]->children.size() == 2){
               declt = declt + "[]";
               newtype = type_convert(declt);
               fprintf(oiloutputfile, "%s__%s;\n", newtype.c_str(), ident.c_str());
            }else{
               fprintf(oiloutputfile, "%s__%s;\n", newtype.c_str(), ident.c_str());
            }
         }*/
      }
      fprintf(oiloutputfile, "}\n");
   }

   if(symbolname == "vardecl"){
      string ident = root->children[1]->lexinfo->c_str();
      string dect = type_check(root->children[1], symtab);
      if (root->children[0]->children.size() == 2) dect = dect + "[]";
      dect = type_convert(dect);
      string child2 = get_yytname(root->children[2]->symbol);
      if(child2 == "constant")
         fprintf(oiloutputfile, "%s_%d_%s = __%s\n", dect.c_str(),symtab->retnum(), ident.c_str(), root->children[2]->children[0]->lexinfo->c_str());
      if(child2 == "variable")
         fprintf(oiloutputfile, "%s_%d_%s = _%d_%s\n", dect.c_str(),symtab->retnum(), ident.c_str(), symtab->retnum(), root->children[2]->children[0]->lexinfo->c_str());

//      code_gen(root->children[2],symtab);
   }

   if(symbolname == "binop"){
      string op = root->children[1]->lexinfo->c_str();
      if(op == "+" || op == "-" || op == "/" || op == "%" || op == "*"){
         int counter = icount++;
         string child0 = get_yytname(root->children[0]->symbol);
         string child2 = get_yytname(root->children[2]->symbol);
         if(child0 != "constant" && child0 != "constant"){
            code_gen(root->children[0], symtab);
            fprintf(oiloutputfile, "i%d = i%d ", counter, counter-1);
         }
         else if(child2 != "constant" && child2 != "constant"){
            code_gen(root->children[2], symtab);
            fprintf(oiloutputfile, "i%d = i%d ", counter, counter-1);
         }else fprintf(oiloutputfile, "i%d = ", counter);

         if(child0 == "constant") fprintf(oiloutputfile, "%s ", root->children[0]->children[0]->lexinfo->c_str());
         if(child0 == "variable") fprintf(oiloutputfile, "_%d_%s ", symtab->retnum(), root->children[0]->children[0]->lexinfo->c_str());
         fprintf(oiloutputfile, "%s ", root->children[1]->lexinfo->c_str());
         if(child2 == "constant") fprintf(oiloutputfile, "%s;\n", root->children[2]->children[0]->lexinfo->c_str());
         if(child2 == "variable") fprintf(oiloutputfile, "_%d_%s;\n", symtab->retnum(), root->children[2]->children[0]->lexinfo->c_str());

      
      }
      if(op == ">" || op == "<" || op == ">=" || op == "<=" || op == "!=" || op == "=="){
         int counter = bcount++;
         string child0 = get_yytname(root->children[0]->symbol);
         string child2 = get_yytname(root->children[2]->symbol);
         if(child0 != "constant" && child0 != "constant"){
            code_gen(root->children[0], symtab);
            fprintf(oiloutputfile, "b%d = b%d ", counter, counter-1);
         }
         else if(child2 != "constant" && child2 != "constant"){
            code_gen(root->children[2], symtab);
            fprintf(oiloutputfile, "b%d = b%d ", counter, counter-1);
         }else fprintf(oiloutputfile, "b%d = ", counter);

         if(child0 == "constant") fprintf(oiloutputfile, "%s ", root->children[0]->children[0]->lexinfo->c_str());
         if(child0 == "variable") fprintf(oiloutputfile, "_%d_%s ", symtab->retnum(), root->children[0]->children[0]->lexinfo->c_str());
         fprintf(oiloutputfile, "%s ", root->children[1]->lexinfo->c_str());
         if(child2 == "constant") fprintf(oiloutputfile, "%s;\n", root->children[2]->children[0]->lexinfo->c_str());
         if(child2 == "variable") fprintf(oiloutputfile, "_%d_%s;\n", symtab->retnum(), root->children[2]->children[0]->lexinfo->c_str());
      }


   }


   if(symbolname == "while_"){
      int currb = bcount;
      int currcount = controlcount++;
//      SymbolTable* currblock = root->children[1]->blockpt;
      fprintf(oiloutputfile, "while_%d:;\n", currcount);
      code_gen(root->children[0], symtab);
      fprintf(oiloutputfile, "if(!b%d) goto break_%d;\n", currb, currcount);
      code_gen(root->children[1],symtab);
      fprintf(oiloutputfile, "goto while_%d;\n", currcount);
      fprintf(oiloutputfile, "break_%d:;\n", currcount);
   }

   if(symbolname == "if_"){
      int currb = bcount;
      int currcount = controlcount++;
      code_gen(root->children[0],symtab);
      fprintf(oiloutputfile, "if(!b%d) goto else_%d;\n", currb, currcount);
      code_gen(root->children[1],symtab);
      fprintf(oiloutputfile, "goto fi_%d;\n", currcount);
      fprintf(oiloutputfile, "else_%d:;\n", currcount);
      if(root->children.size() == 3) code_gen(root->children[2],symtab);
      fprintf(oiloutputfile, "fi_%d:;\n", currcount);
   }




   if(symbolname == "block"){
      SymbolTable* currblock = root->blockpt;
//      fprintf(oiloutputfile, "\n{\n");
      for(size_t child = 0; child < root->children.size(); ++child){
         code_gen(root->children[child], currblock);
      }
//      fprintf(oiloutputfile, "\n}\n");
   }

   if(symbolname == "struct_"){
      string structname;
      for(size_t child = 0; child < root->children.size(); ++child){
         string currchild = get_yytname(root->children[child]->symbol);  
         if(currchild == "TOK_IDENT"){
            structname = root->children[child]->lexinfo->c_str();
         }
      }
      fprintf(oiloutputfile, "struct %s{\n", structname.c_str());

      for(size_t child = 0; child < root->children.size(); ++child){
         string currchild = get_yytname(root->children[child]->symbol);
         if(currchild == "decl"){
            string ident = root->children[child]->children[1]->lexinfo->c_str();
            string declt = root->children[child]->children[0]->children[0]->children[0]->lexinfo->c_str();
            string newtype = type_convert(declt);
            if(root->children[child]->children[0]->children.size() == 2){
               declt = declt + "[]";
               newtype = type_convert(declt);
               fprintf(oiloutputfile, "        %s%s;\n", newtype.c_str(), ident.c_str());
            }else{
               fprintf(oiloutputfile, "        %s%s;\n", newtype.c_str(), ident.c_str());
            }
         }
      }
      fprintf(oiloutputfile, "};\n");
   }

   if(symbolname == "decl"){
      string ident = root->children[1]->lexinfo->c_str();
      string declt = root->children[0]->children[0]->children[0]->lexinfo->c_str();
      string newtype = type_convert(declt);
      if(root->children[0]->children.size() == 2){
         declt = declt + "[]";
         newtype = type_convert(declt);
         fprintf(oiloutputfile, "%s__%s;\n", newtype.c_str(), ident.c_str());
      }else{
         fprintf(oiloutputfile, "%s__%s;\n", newtype.c_str(), ident.c_str());
      }
   }

   if(symbolname == "function"){
      string funcname;
      string rettype;
      SymbolTable* currblock;
      astree* blockroot;
      for(size_t child = 0; child < root->children.size(); ++child){
         string currchild = get_yytname(root->children[child]->symbol);
         if(currchild == "TOK_IDENT"){
            funcname = root->children[child]->lexinfo->c_str();
            rettype = type_check(root->children[child], symtab);
            int firstparen = rettype.find_first_of('(',0);
            rettype = rettype.substr(0,firstparen);
         }
         if(currchild == "block"){
            currblock = root->children[child]->blockpt;
            blockroot = root->children[child];
            if (blockroot->children.size() == 0) return;
         }
      }
      fprintf(oiloutputfile, "%s\n__%s(", type_convert(rettype).c_str(), funcname.c_str());

      for(size_t child = 0; child < root->children.size(); ++child){
         int counter = 0;
         string currchild = get_yytname(root->children[child]->symbol);
         if(currchild == "decl"){
            string ident = root->children[child]->children[1]->lexinfo->c_str();
            string dectype = type_check(root->children[child]->children[1], currblock);
            if(root->children[child]->children[0]->children.size() == 2) dectype = dectype + "[]";
            dectype = type_convert(dectype);
            if(counter != 0) fprintf(oiloutputfile, ",");
            fprintf(oiloutputfile, "\n   %s_%d_%s", dectype.c_str(), currblock->retnum(), ident.c_str());

         }
      }
      fprintf(oiloutputfile, ")\n");
      code_gen(blockroot, symtab);

   }





}

string type_convert(string type){
   if(type == "bool" || type == "char") return "ubyte ";
   if(type == "int") return "int ";
   if(type == "string" || type == "bool[]" || type == "char[]") return "ubyte *";
   if(type == "int[]") return "int *";
   if(type == "string[]") return "ubyte **";
   if(type == "void") return "void";
   size_t check = -1;
   if(type.find("[]") != check) return "struct " + type + " *";
   else return "struct " + type + " **";

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

