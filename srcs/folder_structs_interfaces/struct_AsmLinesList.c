#include <stdio.h>
#include "../../hdrs/global_entities.h"
#include "../../hdrs/mem_management.h"


/*this file provides methods for the AsmLinesList object
it will consist of parsedNodes that in their turn consist of parsedLines.
A parsedLine contains all the information about a certain line:
is it instruct/directive, is it labled, what is the type of each operand, what is the value of each operand...
we use special structs, including a union, to be able to accomodate different data types as an Operand-Value
*/



AsmLinesList* AsmLinesList_createAsmLinesList()
{
    AsmLinesList *this_asm_lines_list;
    this_asm_lines_list = ourMalloc(sizeof(AsmLinesList), TEMP_MEM);
    this_asm_lines_list->_head = NULL;
    this_asm_lines_list->_tail = NULL;
    this_asm_lines_list->IC_section_len = 0;
    this_asm_lines_list->DC_section_len = 0;
    return this_asm_lines_list;
}

void AsmLinesList_addNode(AsmLinesList *this_asm_lines_list, ParsedLine *this_parsed_line)
{
    AsmLineNode *new__node_ptr;
    new__node_ptr = ourMalloc(sizeof(AsmLineNode), TEMP_MEM);
    new__node_ptr->_parsed_line = this_parsed_line;
    new__node_ptr->_next_node = NULL;

    if (this_asm_lines_list->_head == NULL)
        this_asm_lines_list->_head = new__node_ptr;
    else
        this_asm_lines_list->_tail->_next_node = new__node_ptr;
    this_asm_lines_list->_tail = new__node_ptr;
}

int AsmLinesList_getLen(AsmLinesList *this_asm_lines_list)
{
    int len;
    AsmLineNode *curr_node;

    len = 0;    
    curr_node = this_asm_lines_list->_head;
    while (curr_node != NULL)
    {
        len++;
        curr_node = curr_node->_next_node;
    }

    return len;
}





void appendLineNodeToTail(AsmLinesList *parsed_lines_list, AsmLineNode *curr_line_node)
{
    if (parsed_lines_list->_head == NULL) /*if list was empty*/
        parsed_lines_list->_head = curr_line_node;
    else
        parsed_lines_list->_tail->_next_node = curr_line_node;

    parsed_lines_list->_tail = curr_line_node;
    curr_line_node->_next_node = NULL;
}

AsmLinesList *sortAsmLinesList_by_rel_addrss(AsmLinesList *parsed_lines_list)
{ /* destroys the original list in the process */
    AsmLinesList *IC_lines_list, *DC_lines_list;
    AsmLineNode *curr_line_node, *next_line_node;
    enum OccupyType curr_occup_typ;

    
    if (parsed_lines_list->IC_section_len == 0 || parsed_lines_list->DC_section_len == 0) /* if it is only 1 of IC/DC that populates the entire machine-code, the list is already sorted*/
        return parsed_lines_list;
    
    IC_lines_list = AsmLinesList_createAsmLinesList();
    DC_lines_list = AsmLinesList_createAsmLinesList();
    curr_line_node = parsed_lines_list->_head;
    /* printf("inside sort b4 while\n");*/
    while (curr_line_node != NULL)
    {
        next_line_node = curr_line_node->_next_node; /* we gotta save this, as we might change curr node's -next to be null when we insert it to a sub-list*/
        curr_occup_typ = curr_line_node->_parsed_line->_occupy_type;
        if (curr_occup_typ != OCCUP_NONE)
        {
            appendLineNodeToTail(curr_occup_typ == OCCUP_IC ? IC_lines_list : DC_lines_list, curr_line_node);
        }

        curr_line_node = next_line_node;
    }
    /* printf("inside sort after while\n");*/
    /* merge the two sub-lists: it is promised that all IC addresses are lower the the lowest DC address*/
    IC_lines_list->_tail->_next_node = DC_lines_list->_head;
    IC_lines_list->_tail = DC_lines_list->_tail;
    IC_lines_list->_tail = DC_lines_list->_tail->_next_node = NULL;
    /* printf("b4 ret\n");*/
    return IC_lines_list; /* the united list*/
}