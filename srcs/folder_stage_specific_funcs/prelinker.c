#include <stdio.h>
#include <string.h>

#include "../../hdrs/global_entities.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/parsing_support.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/print_funcs.h"
#include "../../hdrs/struct_SymbolTable.h"
#include "../../hdrs/struct_FileList.h"


/*
This file implements our "prelinker"
its purpose is to check compatibility between the .extern, .entry declarations between files
for example, we expect that if file "a.as" has ".entry A", then some other file should have ".extern A"
and vice versa.

We included this part even though the maman is ambiguous about needing to check these things
*/

/*we used the just-created ent/ext files to read all of the symbols */
void addSymbolsToPool_per_file(FileNode* this_fnode, EntrySymbol **ents_pool, ExternSymbol **exts_pool) {
    Symbol* curr_sym;
    curr_sym = this_fnode->symbolTable;

    while(curr_sym != NULL){
        if(curr_sym->origin == ORIGIN_ENTRY)
            *ents_pool = appendEntrySymbol(curr_sym->string_name, this_fnode->file_name, *ents_pool);
        else if(curr_sym->origin == ORIGIN_EXTERN)
            *exts_pool = appendExternSymbol(curr_sym->string_name, this_fnode->file_name, *exts_pool);
        
        curr_sym = curr_sym->next;
    }
}


/* cross-file checks - this is the actual for between incompatibilities between externs and entries */
int crossCheck(EntrySymbol *ents_pool, ExternSymbol *exts_pool) {
    
    /* printf("in x check\n");*/
    EntrySymbol *curr_ent;
    ExternSymbol *curr_ext;
    int is_warn, found;

    is_warn = 0;
    
    /*we use a very primitive nested loop here - compare each symbol to all the others, once for entreies, once for externs*/
    /*discarded the test for externs - we only test if all entries have externs*/
    
    for (curr_ent = ents_pool; curr_ent != NULL; curr_ent = curr_ent->next) {
        found = 0;
        for (curr_ext = exts_pool; curr_ext != NULL; curr_ext = curr_ext->next) {
            /* printf("b4 compare: ent: %s, ext: %s\n", curr_ent->name, curr_ext->name);*/
            if (strncmp(curr_ext->name, curr_ent->name, strlen(curr_ext->name)) == 0){
                found = 1;
                break;
            }
        }


        if(!found) {
            printf("  ");
            printErrWarn_intro_only(1, -1);
            printf("%s.as: ", curr_ent->base_filename);
            printf(".entry '%s' has no matching .extern in any other file\n", curr_ent->name);
            is_warn = 1;
        }
    }
    
    return is_warn;
}

/*checking if an entry with the same name is declared in two different files*/
int checkDuplicateEntries(EntrySymbol *entries) {
    EntrySymbol *ent_outer, *ent_inner;
    int is_warn, curr_see_count;
    char* outer_name;

    is_warn = 0;

    /*problem - when iterating primitively, all errors are printed with duplications*/
    /*solution - we test only for the first occurance of this symbol, by putting a condition:*/
    /*if the first-encountered symbol with the same name is not the actual pointer itself, then this symbol has been already checked for:*/

    for(ent_outer = entries; ent_outer != NULL; ent_outer = ent_outer->next){
        outer_name = ent_outer->name;
        curr_see_count = 0;

        for(ent_inner = entries; ent_inner != NULL; ent_inner = ent_inner->next){
            if(strcmp(ent_outer->name, ent_inner->name) == 0){
                
                if(curr_see_count == 0 && ent_outer != ent_inner){ /*condition for preventing duplicates*/
                    break;
                }
                curr_see_count++;
            }

            if(curr_see_count > 1){ /*1 time will always be encountered even in a legal situation*/
                printf("  ");
                printErrWarn_from_3way_str(".entry ", outer_name, " is declared in multiple files", 1, -1);
                is_warn = 1;
                break;
            }
        }
    }

    return is_warn;
}


/*prelinker - check compatibility between the .extern, .entry declarations between files*/
int runPrelinker(FileList *file_list) {
    FileNode *curr_f_node;
    EntrySymbol *ents_pool;
    ExternSymbol *exts_pool;
    int is_warn;

    curr_f_node = file_list->head;
    ents_pool = NULL; 
    exts_pool = NULL;
    is_warn = 0;

    while (curr_f_node != NULL) {
        addSymbolsToPool_per_file(curr_f_node, &ents_pool, &exts_pool);
        curr_f_node = curr_f_node->next_node;
    }


    if(ents_pool != NULL ){
        is_warn = checkDuplicateEntries(ents_pool);
        if(exts_pool != NULL)
            is_warn = crossCheck(ents_pool, exts_pool) || is_warn;
    }

    return is_warn;
}