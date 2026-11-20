#include <stdio.h>
#include <string.h>

#include "../../hdrs/global_entities.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/print_funcs.h"

#include "../../hdrs/preprocess.h"
#include "../../hdrs/expanded_to_parsed.h"
#include "../../hdrs/parsing_support.h"
#include "../../hdrs/parsed_to_machine.h"
#include "../../hdrs/prelinker.h"


#include "../../hdrs/struct_FileList.h"
#include "../../hdrs/struct_AsmLinesList.h"
#include "../../hdrs/struct_SymbolTable.h"


/*
this file provides the major functions for the main.c file
Each function here represents a stage of the assembly process (macros-expansion, first-pass, second-pass..),
 and deals with activating support-functions from other files, to complete the operation that was requested from main, for the current file.

 Also enables invokation of our "pre-linker" that is executed once for the entire run.
*/


FileList *populateFilesList(int argc, char *argv[])
{
    FileList *files_list;
    int i;

    if (argc < 2)
    {
        printf("Usage is: %s <input_file1> [<input_file2> ...]\n", argv[0]);
        return NULL;
    }

    files_list = FileList_createFileList(); /* creates a blank files-list*/
    if (files_list == NULL)
    {
        printf("Failed to create files list\n");
        return NULL;
    }

    for (i = 1; i < argc; i++)
    {
        FileList_addFile(files_list, argv[i]);
    }

    return files_list;
}




/*invoker of preprocessing (macros expansion)*/
void invokePreprocess_perFile(FileNode *current_file_node, int *err_flag_byRef)
{
    printIntro("Preprocessing", current_file_node->file_name);
    *err_flag_byRef = preprocessFile(current_file_node); /* curr_expanded_f comes opened!*/
    printConclusion("Preprocessing", current_file_node->file_name, *err_flag_byRef);
    if (!*err_flag_byRef)
    {
        printf("Note: from now, lines will be referred to in terms of *after macros expansion*\n");
    }
}


/*invoker of first-pass: including parsing, initial convertion to machine code, symbol-table fill*/
AsmLinesList *invokeFirstPass_perFile(FileNode *current_file_node, int *err_flag_byRef)
{
    char *plain_src_f_name;
    int local_err_flag;
    FILE *expanded_f_ptr;
    AsmLinesList *parsed_lines_list;

    plain_src_f_name = current_file_node->file_name;
    printIntro("First Pass", plain_src_f_name);

    /* Now, opening the macro-expanded file*/
    expanded_f_ptr = fopen(appendExtension_to_f_name(plain_src_f_name, "am"), "r");

    /* Now, parsing the macro-expanded file onto an AsmLinesList struct*/
    parsed_lines_list = parseRawAsmFile(expanded_f_ptr, &local_err_flag); /*local_err_flag is updated by reference*/
    fclose(expanded_f_ptr);

    if (parsed_lines_list->_head == NULL) /* if file (after macro expansion) is effectivley empty*/
    {
        *err_flag_byRef = 0;
        printErrWarn_from_3way_str("File ", appendExtension_to_f_name(plain_src_f_name, "am"), " is effectively empty", 1, -1);
        printConclusion("First Pass", plain_src_f_name, 0);
        return parsed_lines_list;
    }

    /* Now, initiallizing machine code (no errors can happen here)*/
    if (!local_err_flag) /*re-activate on production*/
    {
        initMachineCode_per_ParsedLinesList(parsed_lines_list); /* no errors should occur here; the parsed data is legal and ready for convertion to machine code*/
    }

    /* for debug purposes*/
    /*   AsmLinesList_printList(parsed_lines_list); */

    /* Now, filling the symbol table*/
    if (!local_err_flag)
    {
        local_err_flag = fillSymbolTable(current_file_node, parsed_lines_list);
    }

    /* Now, validating that all .entry definitions have received an address*/
    if (!local_err_flag)
    {
        local_err_flag = validateSymbolTable(current_file_node) == SYMBOL_ERROR;
    }

    printConclusion("First Pass", plain_src_f_name, local_err_flag);
    *err_flag_byRef = local_err_flag;
    return local_err_flag ? NULL : parsed_lines_list; /* NULL if errored*/
}


/*invoker of second-pass: completion of machine code according to the filled symbol-table*/
void invokeSecondPass_perFile(FileNode *current_file_node, AsmLinesList *parsed_lines_list, int *err_flag_byRef)
{
    /* now we wish to fill in every label-address that's missing in the machine code*/

    AsmLineNode *this_line_node;
    ParsedLine *this_parsed_line;
    MachineWordsList *this_mchn_list;
    MachineWordNode *this_mchn_node;
    MachineWord *this_mchn_word;
    Symbol *fetched_symbol;
    int is_extern, is_curr_err, is_any_err;
    char str_buff_errs[MAX_ERR_MSG_LEN + 1] = {'\0'};

    is_any_err = is_curr_err = 0;
    this_line_node = parsed_lines_list->_head;

    printIntro("Second Pass", current_file_node->file_name);
    nullifyAllWarns(parsed_lines_list); /* for all lines - discard previous warnings from the parsing stage*/

    while (this_line_node != NULL)
    {
        is_curr_err = 0;
        this_parsed_line = this_line_node->_parsed_line;
        this_mchn_list = this_parsed_line->_mchn_words_lst;
        if (this_mchn_list != NULL)
        {
            this_mchn_node = this_mchn_list->_head;
            while (this_mchn_node != NULL)
            {
                this_mchn_word = &(this_mchn_node->_mchn_word);
                if (this_mchn_word->_is_label_ref)
                { /* an actual machine word with a label was found*/
                    fetched_symbol = fetchByName(current_file_node,
                                                 this_mchn_word->_lbl_ref_name,
                                                 this_mchn_word->_relative_addrss);

                    /* printf("tryna fetch: %s; is null: %d\n", this_mchn_word->_lbl_ref_name, fetched_symbol == NULL);*/

                    if (fetched_symbol == NULL)
                    {
                        /* snprintf(str_buff_errs, MAX_ERR_MSG_LEN, "Symbol \"%s\" is never defined", this_mchn_word->_lbl_ref_name);*/
                        is_curr_err = 1;
                        nullifyCharArr(str_buff_errs, MAX_ERR_MSG_LEN);
                        strncat(str_buff_errs, "Symbol '", MAX_ERR_MSG_LEN);
                        strncat(str_buff_errs, this_mchn_word->_lbl_ref_name, MAX_ERR_MSG_LEN);
                        strncat(str_buff_errs, "' is never defined", MAX_ERR_MSG_LEN);
                        addErrWarn(this_parsed_line, str_buff_errs, 0);
                    }
                    else
                    { /* symbol fetched successfully, now insert its address onto the machine-word*/
                        is_extern = fetched_symbol->origin == ORIGIN_EXTERN;
                        populateBitsWithNum_byDecimal(this_mchn_word->_bits_str, STRPOS_ALL_BUT_ARE_FROM, STRPOS_ALL_BUT_ARE_TO, is_extern ? 0 : fetched_symbol->int_decimal_value); /* filling the first 8 bits with the address of this symbol*/
                        populateBitsWithNum_byDecimal(this_mchn_word->_bits_str, STRPOS_INSTRCT_ARE_FROM, STRPOS_INSTRCT_ARE_TO, is_extern ? ARE_EXT : ARE_RE);                                 /* filling the ARE bits: E (external) - 01, R (reallocatable) - 10*/
                    }
                }
                this_mchn_node = this_mchn_node->_next_node;
            }
        }

        is_any_err = is_any_err || is_curr_err;
        this_line_node = this_line_node->_next_node;
    }

    printErrsWarns_per_AsmLinesList(parsed_lines_list);
    /* logSymbolTable(current_file_node, "logSymbolTable.txt");*/
    
    printConclusion("Second Pass", current_file_node->file_name, is_any_err);
    *err_flag_byRef = is_any_err;
}

void invokeUsageValidation_perFile(FileNode *current_file_node){
    int is_warn;
    
    printIntro("Labels Usage tests", current_file_node->file_name);
    is_warn = validateIsUsed(current_file_node); /*change validateIsUsed to return 1 or 0*/
    printf("Labels Usage tests %s\n", is_warn ? "have finished with warnings" : " successful");
}


/*invoker of output-files generators: ob, ent, ext (if required)*/
void invokeOutputFilesGeneration_perFile(FileNode *current_file_node, AsmLinesList *parsed_lines_list, int *err_flag_byRef)
{
    /* now, all machine words are generated and filled (even the labels)*/
    /* for the .ob file, we have to:*/
    /* 1. create a file with the correct name*/
    /* 2. fill its first line with lengths of IC/DC sections*/
    /* 3. loop over all machine words - IC first, then DC and print them onto the file*/
    /* problem - the lines-list is not sorted by relative address - IC and DC rows can be intertwined*/
    /* solution - between stages 2,3, create a sorted list by detaching each dc node from the main list, put it temporarily to the tail of another list*/
    /* in the end we just attach the tail of the original list (it contains only ICs now) to the head of the temporary list (in contains only DCs now), and print this final list to the file*/

    FILE *ob_f_ptr; /* without suffix!*/
    AsmLineNode *this_line_node;
    MachineWordsList *this_mchn_list;
    MachineWordNode *this_mchn_node;
    MachineWord *this_mchn_word;



    printIntro("Output files generation", current_file_node->file_name);
    ob_f_ptr = fopen(appendExtension_to_f_name(current_file_node->file_name, "ob") , "w");

    /* line with lengths of IC/DC sections*/
    fprintf(ob_f_ptr, "%s\t", intToBase4str_compact(parsed_lines_list->IC_section_len));
    fprintf(ob_f_ptr, "%s ", intToBase4str_compact(parsed_lines_list->DC_section_len));

    /*
    fprintf(ob_f_ptr, "; IC_count: %d, ", parsed_lines_list->IC_section_len);
    fprintf(ob_f_ptr, "DC_count: %d", parsed_lines_list->DC_section_len);
    */

    parsed_lines_list = sortAsmLinesList_by_rel_addrss(parsed_lines_list); /* parsed_lines_list is a new object, of the 2 sorted-sub-lists merged*/
    this_line_node = parsed_lines_list->_head;

    while (this_line_node != NULL)
    {
        this_mchn_list = this_line_node->_parsed_line->_mchn_words_lst;
        if (this_mchn_list != NULL)
        {
            this_mchn_node = this_mchn_list->_head;
            while (this_mchn_node != NULL)
            {
                this_mchn_word = &(this_mchn_node->_mchn_word);
                fprintf(ob_f_ptr, "\n");
                fprintf(ob_f_ptr, "%s\t", intToBase4str_compact(this_mchn_word->_relative_addrss));
                fprintf(ob_f_ptr, "%s",
                        this_mchn_word->_bits_str[0] == '?' ? "?????" : machineWordToBase4str(this_mchn_word->_bits_str));

                /*
                fprintf(ob_f_ptr, " ; %d ", this_mchn_word->_relative_addrss);
                fprintf(ob_f_ptr, "%s", this_mchn_word->_bits_str);
                */

                this_mchn_node = this_mchn_node->_next_node;
            }
        }

        this_line_node = this_line_node->_next_node;
    }
    fclose(ob_f_ptr);

    /*Lastly: generate the ent/ext files (if required) based on the symbols-table*/
    /* printf("b4 entext gen\n");*/
    createSymbolOutputFiles(current_file_node, err_flag_byRef);

    printConclusion("Output files generation", current_file_node->file_name, *err_flag_byRef);
}


/*invoker of our "pre-linker": we test compatibillity of .extern/.entry declarations between files*/
void invokePreLinker_all_files_combined(FileList *file_list){
    int is_warn;

    if(file_list == NULL || file_list->head == NULL || file_list->head == file_list->tail) /*no need for prelinking if only a single file..*/
        return;

    printf("------------------------\n");
    printf("Started Compatibility Check between ents/exts\n");
    is_warn = runPrelinker(file_list);
    printf("Compatibility Check between ents/exts %s\n", is_warn ? "had finished with warnings" : "successful");
    printf("------------------------\n\n");
}