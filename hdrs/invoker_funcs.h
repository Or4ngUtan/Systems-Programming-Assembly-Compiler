#ifndef INVOKER_FUNCS_H
#define INVOKER_FUNCS_H

#include "global_entities.h"

FileList *populateFilesList(int argc, char *argv[]);
void invokePreprocess_perFile(FileNode *current_file_node, int *err_flag_byRef);
AsmLinesList *invokeFirstPass_perFile(FileNode *current_file_node, int *err_flag_byRef);
void invokeSecondPass_perFile(FileNode *current_file_node, AsmLinesList *parsed_lines_list, int *err_flag_byRef);
void invokeUsageValidation_perFile(FileNode *current_file_node);
void invokeOutputFilesGeneration_perFile(FileNode *current_file_node, AsmLinesList *parsed_lines_list, int *err_flag_byRef);
void invokePreLinker_all_files_combined(FileList *file_list);
void populate_files_list_from_args(FileList *files_list, int argc, char *argv[]);

#endif