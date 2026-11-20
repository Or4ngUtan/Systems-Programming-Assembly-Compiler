#ifndef STRUCT_ASMLINESLIST_H
#define STRUCT_ASMLINESLIST_H

#include "global_entities.h"

AsmLinesList *AsmLinesList_createAsmLinesList();
void AsmLinesList_addNode(AsmLinesList *this_asm_lines_list, ParsedLine *this_parsed_line);
int AsmLinesList_getLen(AsmLinesList *this_asm_lines_list);

void appendLineNodeToTail(AsmLinesList *parsed_lines_list, AsmLineNode *curr_line_node);
AsmLinesList *sortAsmLinesList_by_rel_addrss(AsmLinesList *parsed_lines_list);

#endif
