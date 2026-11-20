#ifndef STRUCT_MACHINEWORDSLIST_H
#define STRUCT_MACHINEWORDSLIST_H

#include "global_entities.h"

MachineWord *MachineWordsList_addNode(MachineWordsList *this_mchn_list);
int MachineWordsList_getLen(MachineWordsList *this_mchn_list);
void MachineWordsList_printList(MachineWordsList *this_mchn_list);

#endif
