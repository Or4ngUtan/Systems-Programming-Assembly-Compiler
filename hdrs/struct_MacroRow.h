#ifndef STRUCT_MACROROW_H
#define STRUCT_MACROROW_H

#include "global_entities.h"

MacroRow *FindMacro(MacroList* this_list, const char *name);
MacroList * MacroList_createBlankList(void);
void MacroList_addNode(MacroList* this_list, char* mcro_name_ourmalloced /*assumed legal name*/, char* mcro_body_realloced);
void MacroList_freeMacroBodies(MacroList* this_list);

#endif
