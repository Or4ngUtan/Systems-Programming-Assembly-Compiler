#ifndef STRUCT_SYMBOLTABLE_H
#define STRUCT_SYMBOLTABLE_H

#include "global_entities.h"


EntrySymbol *appendEntrySymbol(char *name_malloced, char *base_filename_malloced, EntrySymbol *head);
ExternSymbol *appendExternSymbol(char *name_malloced, char *base_filename_malloced, ExternSymbol *head);

Symbol *FindSymbol(FileNode * this_f_node, const char *name);
void initSymbolTable(FileNode * this_f_node);
int addSymbol(FileNode * this_f_node, const char *name, int value, enum OriginType origin, int line_number);
void addUsage(struct Usage **head, int line_number);
Symbol *fetchByName(FileNode * this_f_node, const char *name, int line_number);
int getAddressByLabelName(FileNode * this_f_node, const char *label_str);
void freeSymbolTable(FileNode * this_f_node);
void logSymbolTable(FileNode * this_f_node, const char *filename);
enum SymbolStatus validateSymbolTable(FileNode * this_f_node);
int validateIsUsed(FileNode * this_f_node);
int printSymbolToFile(FILE* f_ptr, char* sym_str, int machine_addrss);
void createSymbolOutputFiles(FileNode * this_f_node, int* err_flag_byRef);
int fillSymbolTable(FileNode *current_file_node, AsmLinesList *parsed_lines_list);


#endif

