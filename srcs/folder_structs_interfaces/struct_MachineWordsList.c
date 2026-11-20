#include <stdio.h>
#include <string.h>

#include "../../hdrs/global_entities.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/mem_management.h"


/*a machine-word consists of the bits-image of individual machine words
we also use them to indicate if they need refill after symbol table has been completed
*/

/* returns a ptr to the internal machine-word-struct within the newly created node*/
MachineWord *MachineWordsList_addNode(MachineWordsList *this_mchn_list)
{
    MachineWordNode *new_node_ptr;
    new_node_ptr = ourMalloc(sizeof(MachineWordNode), TEMP_MEM);
    /* new_node_ptr->_mchn_word = ; no need - the machine-word-struct is automatically allocated when allocating for the node - it is not a pointer*/
    new_node_ptr->_next_node = NULL;
    (new_node_ptr->_mchn_word)._is_label_ref = 0; /*default! has to be changed for labels*/

    if (this_mchn_list->_head == NULL)
        this_mchn_list->_head = new_node_ptr;
    else
        this_mchn_list->_tail->_next_node = new_node_ptr;
    this_mchn_list->_tail = new_node_ptr;

    nullifyCharArr(new_node_ptr->_mchn_word._bits_str, MACHINE_WORD_ARR_SIZE); /*crucial for putting an end to the string at all*/

    return &(new_node_ptr->_mchn_word);
}

/* this will provide the count of machine words for a parsed-line*/
int MachineWordsList_getLen(MachineWordsList *this_mchn_list)
{
    int len;
    MachineWordNode *curr_node;

    if(this_mchn_list == NULL)
        return 0;
    len = 0;

    curr_node = this_mchn_list->_head;
    while (curr_node != NULL)
    {
        len++;
        curr_node = curr_node->_next_node;
    }

    return len;
}


/*mainly for debugging purposes but the principle is the same when writing to the final output files*/
void printSingle_mchnWord(char * bits_str){
    int i;
    for(i = 0; i < 10; i++){
        printf("%c", bits_str[i]);
        if(i == 3 || i == 7)
            printf("-");
    }
}

void MachineWordsList_printList(MachineWordsList *this_mchn_list)
{
    int len;
    MachineWordNode *curr_node;

    if(this_mchn_list == NULL){
        printf("null;\n");
        return;
    }
    
    len = 0;
    curr_node = this_mchn_list->_head;
    while (curr_node != NULL)
    {   
        printSingle_mchnWord((curr_node->_mchn_word)._bits_str);
        printf(" (adrs %d)", (curr_node->_mchn_word)._relative_addrss);
        printf("\n");
        len++;
        curr_node = curr_node->_next_node;
    }
}
