#ifndef STRUCT_FILELIST_H
#define STRUCT_FILELIST_H

#include "global_entities.h"

FileList *FileList_createFileList(void);
FileNode * FileList_addFile(FileList *file_list, char *file_name);
int FileList_contains(FileList *file_list, char *file_name);
/* void FileList_freeFileList(FileList *file_list);*/
FileNode *FileList_findFile(FileList *file_list, const char *file_name);

#endif
