#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../hdrs/mem_management.h"
#include "../../hdrs/global_entities.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/print_funcs.h"
#include "../../hdrs/invoker_funcs.h"

#include "../../hdrs/struct_FileList.h"


int main(int argc, char *argv[])
{
    FileList *files_list;
    int err_curr_file, err_any_file;
    char *curr_f_name_plain;
    FileNode *current_file_node;
    AsmLinesList *curr_lines_list;

    
    initGarbageCollectors();

    files_list = populateFilesList(argc, argv);
    if (files_list == NULL)
    {
        freeGarbageCollectors();
        return EXIT_FAILURE;
    }

    printf("================= Program start =================\n\n");
   

    err_any_file = 0;
    current_file_node = files_list->head;
    curr_lines_list = NULL;

    while (current_file_node != NULL) /*loop over each received .as file*/
    {
        curr_f_name_plain = current_file_node->file_name;
        err_curr_file = 0;

        printf("------------------------\n");
        printIntro("Assembly", curr_f_name_plain);

        /* main operations for each file START*/

        /*invoker of preprocessing (macros expansion)*/
        if (!err_curr_file)
            invokePreprocess_perFile(current_file_node, &err_curr_file);

        /*invoker of first-pass: including parsing, initial convertion to machine code, symbol-table fill*/
        if (!err_curr_file)
            curr_lines_list = invokeFirstPass_perFile(current_file_node, &err_curr_file);

        /*invoker of second-pass: completion of machine code according to the filled symbol-table*/
        if (!err_curr_file)
            invokeSecondPass_perFile(current_file_node, curr_lines_list, &err_curr_file); 
        
        /*invoker of internal label-usage-tests (detection of unused labels)*/
        if (!err_curr_file)
            invokeUsageValidation_perFile(current_file_node);

        /*invoker of output-files generators: ob, ent, ext (if required)*/
        if (!err_curr_file)
            invokeOutputFilesGeneration_perFile(current_file_node, curr_lines_list, &err_curr_file);

        /* main operations for each file END*/

        printConclusion("Assembly", curr_f_name_plain, err_curr_file);
        printf("------------------------\n\n");

        flushGarbageCollector(0); /* flushing short-term allocations (long term allocations such as the symbol table for current file are kept, for the pre linker)*/
        err_any_file = err_any_file || err_curr_file;
        current_file_node = current_file_node->next_node;
    }

    /*invoker of our "pre-linker": we test compatibillity of .extern/.entry declarations between files*/
    if (!err_any_file)
        invokePreLinker_all_files_combined(files_list);

    printf("Overall, Assembly operation %s\n", err_any_file ? "finished with errors" : "successful");
    printf("================= Program end =================\n");

    freeGarbageCollectors(); /* flushing both long and short-term allocations*/
    return err_any_file ? EXIT_FAILURE : EXIT_SUCCESS;
}
