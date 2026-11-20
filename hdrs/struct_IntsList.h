#ifndef STRUCT_INTSLIST_H
#define STRUCT_INTSLIST_H

#include "global_entities.h"

IntsList *IntsList_createIntsList(void);
void IntsList_addInt(IntsList *this_int_list, int num);
int IntsList_getLen(IntsList *this_int_list);
void IntsList_printIntsList(IntsList *this_int_list);

#endif