#include <stdio.h>
#include "../../hdrs/global_entities.h"
#include "../../hdrs/mem_management.h"

/*a simple int-list struct for implementing int sequences (for example for the operands of .mat / .data)*/

IntsList *IntsList_createIntsList()
{
    IntsList *this_int_list;
    this_int_list = ourMalloc(sizeof(IntsList), TEMP_MEM);
    this_int_list->_head = NULL;
    this_int_list->_tail = NULL;
    return this_int_list;
}

void IntsList_addInt(IntsList *list, int value)
{
    IntNode *new_node = (IntNode *)ourMalloc(sizeof(IntNode), TEMP_MEM);
    if (new_node == NULL)
    {
        return;
    }
    new_node->_num = value;
    new_node->_next_node = NULL;

    if (list->_head == NULL)
    {
        list->_head = new_node;
        list->_tail = new_node;
    }
    else
    {
        list->_tail->_next_node = new_node;
        list->_tail = new_node;
    }
}

int IntsList_getLen(IntsList *this_int_list)
{
    int len;
    IntNode *curr_node;

    len = 0;
    curr_node = this_int_list->_head;
    while (curr_node != NULL)
    {
        len++;
        curr_node = curr_node->_next_node;
    }
    return len;
}

void IntsList_printIntsList(IntsList *this_int_list) /**! check if needed in prod */
{
    IntNode *curr_node;
    curr_node = this_int_list->_head;
    while (curr_node != NULL)
    {
        printf("%d", curr_node->_num);
        if (curr_node->_next_node != NULL)
        {
            printf(" ,");
        }
        curr_node = curr_node->_next_node;
    }
}