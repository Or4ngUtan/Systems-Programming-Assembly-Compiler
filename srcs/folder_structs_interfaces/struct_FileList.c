#include <stdio.h>
#include <string.h>

#include "../../hdrs/struct_FileList.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/print_funcs.h"

/*The FileList struct helps us iterate through multiple files, and keep track of them until the end when we have to perform cross-files tests*/

FileList *FileList_createFileList(void)
{
    FileList *new_list;

    new_list = (FileList *)ourMalloc(sizeof(FileList), LONG_MEM);
    if (new_list == NULL)
    {
        return NULL;
    }
    new_list->head = NULL;
    new_list->tail = NULL;
    return new_list;
}


int isFilenameAlrdyExists(FileList *files_list, char * f_name)
{
    FileNode * curr_f_node;
    int f_name_len, curr_bigger_len; /*we gotta compare number of characters based on the larger string, as 2 files such as "test", "test1" are a vaild case*/

    if(files_list == NULL)
        return 0;

    curr_f_node = files_list->head;
    f_name_len = strlen(f_name);
    while(curr_f_node != NULL){
        curr_bigger_len = strlen(curr_f_node->file_name);
        if(curr_bigger_len < f_name_len)
            curr_bigger_len = f_name_len;

        if(strncmp(curr_f_node->file_name,f_name, curr_bigger_len) == 0)
            return 1;
        
        curr_f_node = curr_f_node->next_node;
    }
    
    return 0;
}



FileNode *FileList_addFile(FileList *file_list, char *file_name)
{
    FileNode *new_node;


    if (file_list == NULL || file_name == NULL)
    {
        return NULL;
    }

    if(isFilenameAlrdyExists(file_list, file_name)){
         printErrWarn_from_3way_str("file name: ", file_name, " was passed several times as argument. ignoring duplicates", 1, -1);
         return NULL;
     }

    new_node = (FileNode *)ourMalloc(sizeof(FileNode), LONG_MEM);
    if (new_node == NULL)
    {
        return NULL;
    }

    new_node->file_name = (char *)ourMalloc(strlen(file_name) + 1, LONG_MEM);
    if (new_node->file_name == NULL)
    {
        return NULL;
    }
    strcpy(new_node->file_name, file_name);
    new_node->file_name[strlen(file_name)] = '\0';
    new_node->symbolTable = NULL; /* Initialize symbol table to NULL */
    new_node->next_node = NULL;
    /* init flags that indicate whether we have encountered entry definition / extern usage, to be able to decide making ent/ext files or not */
    new_node->_is_ent_seen = 0;
    new_node->_is_ext_usage_seen = 0;
    new_node->_macro_list = NULL;

    if (file_list->head == NULL)
    {
        file_list->head = new_node;
        file_list->tail = new_node;
    }
    else
    {
        file_list->tail->next_node = new_node;
        file_list->tail = new_node;
    }
    return new_node;
}

/* void FileList_freeFileList(FileList *file_list)*/

int FileList_contains(FileList *file_list, char *file_name) {
    FileNode *current_node;

    current_node = file_list->head;
    while (current_node != NULL) {
        if (strcmp(current_node->file_name, file_name) == 0) {
            return 1;
        }
        current_node = current_node->next_node;
    }
    return 0;
}


FileNode *FileList_findFile(FileList *file_list, const char *file_name)
{
    FileNode *current;

    if (file_list == NULL || file_name == NULL)
    {
        return NULL;
    }

    current = file_list->head;
    while (current != NULL)
    {
        if (strcmp(current->file_name, file_name) == 0)
        {
            return current;
        }
        current = current->next_node;
    }
    return NULL;
}