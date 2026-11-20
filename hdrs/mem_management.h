#ifndef MEM_MANAGMENT_H
#define MEM_MANAGMENT_H

#include <stdlib.h>

#define TEMP_MEM 0
#define LONG_MEM 1


typedef struct garbageNode
{
    void *_internalPtr;
    struct garbageNode *_nextNode;
} garbageNode;

typedef struct
{
    garbageNode *_head;
    garbageNode *_tail;
} garbageList; /* needs initiallization via main; will point to a dynamically-sized list whose elements are of type garbageNode*/


/*global vars*/
extern garbageList *glob_garbageList;
extern garbageList *glob_garbageList_long_term;

int initGarbageCollectors();
void* ourMalloc(int size, int is_long_term);
void flushGarbageCollector(int is_long_term);
void freeGarbageCollectors();



#endif