#include <stdio.h>
#include <string.h>

#include "../../hdrs/global_entities.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/print_funcs.h"

#include "../../hdrs/struct_MacroRow.h"


/*this struct aggregates everything we need for a single macro definition - its name and body
the body is cleverly dynamically allocated as it can vary in length greatly

*/

MacroRow *FindMacro(MacroList* this_list, const char *name)
{
    MacroRow *current;

    current = this_list->_head;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}



/*macros list entities*/

MacroList * MacroList_createBlankList(void){
    MacroList* this_list;

    this_list = ourMalloc(sizeof(MacroList), 0);
    this_list->_head = NULL;
    this_list->_tail = NULL;

    return this_list;
}

void MacroList_addNode(MacroList* this_list, char* mcro_name_ourmalloced /*assumed legal name*/, char* mcro_body_realloced){
    MacroRow* new_node;

    new_node = ourMalloc(sizeof(MacroRow), 0);
    new_node->name = mcro_name_ourmalloced;
    new_node->body = mcro_body_realloced;
    new_node->next = NULL;
    if(this_list->_head == NULL){
        this_list->_head = new_node;
    }
    else{
        this_list->_tail->next = new_node;
    }
    this_list->_tail = new_node;
}

void MacroList_freeMacroBodies(MacroList* this_list){
    MacroRow* curr_node;

    if(this_list == NULL)
        return;

    curr_node = this_list->_head;
    while(curr_node != NULL){
        /*it is assured that every macroRow that was added has a legal realloced macro-body that is freeable*/
        free(curr_node->body);
        curr_node->body = NULL;
        curr_node = curr_node->next;
    }
}