#include <stdio.h>
#include <stdlib.h>

#include "../../hdrs/mem_management.h"

/*general logic - explanation:*/
/*
We wish to implement a more automatic way to allocate and free dynamic memory.
Normally, each time we use malloc(), we have to manually track the pointer, and free it only after it is no longer required. this is tedious.

Instead, we will use a special wrapper function: "ourMalloc()" and 2 global arrays: "glob_garbageList", "glob_garbageList_long_term", to manage the allocations;

Every time we use "ourMalloc", we call malloc() inside, BUT, also put the pointer for this allocation to a special list: glob_garbageList, that is a list of "garbageNodes", that will keep track of it.
like this, we can afterwards iterate over this list, and free every allocation that was ever made using ourMalloc, in one go.
note that the the "garbageNodes" themselves also require dynamic-allocations: we allocate and free memory for them also.

this is a sort of a "garbage collection" mechanism, only that it works in cycles, per the user's demand: 
we will initialize the garbageList before working on each .as file, and when we are finished with this current file, we "flush" all the allocations using "flushGarbageCollector" method

note that there are allocations that must be kept to a longer-term, like the symbols-table of each file (we compare all the symbols-tables between themselves after iterating over all files, to check compatibility between
.exter/.entry declarations between files (we call this "the prelinker")).
For these cases we use a separate list: "glob_garbageList_long_term" - when using ourMalloc(1), the pointers are tracked by this special list, that will be flushed only when the entire program is finished
(as opposed to "glob_garbageList" that is flushed after finishing the treatment of each individual .as file)
*/

garbageList * glob_garbageList = NULL;
garbageList * glob_garbageList_long_term = NULL;

int initGarbageCollectors()
{ /* initialize garbage collector on launch / flush */
    garbageList **g_list_ptr_ptr;
    int i;
    for (i = 0; i < 2; i++)
    {
        g_list_ptr_ptr = (i == 1 ? &glob_garbageList_long_term : &glob_garbageList);
        *g_list_ptr_ptr = (garbageList *)malloc(sizeof(garbageList));
        if (*g_list_ptr_ptr == NULL)
        {
            /* if we failed to allocate the second one, free the first (if allocated) */
            if (i == 1)
            {
                if (glob_garbageList != NULL)
                {
                    free(glob_garbageList);
                    glob_garbageList = NULL;
                }
            }
            return 1;
        }
        (*g_list_ptr_ptr)->_head = NULL;
        (*g_list_ptr_ptr)->_tail = NULL;
    }
    return 0;
}

void *ourMalloc(int size, int is_long_term)
{
    garbageList *g_list;
    garbageNode *gNodePtr;
    void *ptr;


    if (size == 0)
        size = 1; /* normalize zero-size */

    ptr = malloc((size_t)size);
    if (ptr == NULL)
    {
        return NULL; /* primary allocation failed */
    }

    /* ensure the requested list exists; if not try to init. If init fails, return ptr untracked. */
    g_list = is_long_term ? glob_garbageList_long_term : glob_garbageList;
    if (g_list == NULL)
    {
        if (initGarbageCollectors() != 0)
        {
            /* cannot track, but return allocated pointer (safer than freeing it) */
            return ptr;
        }
        g_list = is_long_term ? glob_garbageList_long_term : glob_garbageList;
        if (g_list == NULL)
        {
            return ptr; /* defensive: still not available */
        }
    }

    gNodePtr = (garbageNode *)malloc(sizeof(garbageNode));
    if (gNodePtr == NULL)
    {
        /* cannot create tracking node; return allocated pointer untracked (do NOT free it) */
        return ptr;
    }

    gNodePtr->_internalPtr = ptr;
    gNodePtr->_nextNode = NULL;

    if (g_list->_tail == NULL)
    { /* list empty */
        g_list->_head = gNodePtr;
    }
    else
    {
        g_list->_tail->_nextNode = gNodePtr;
    }
    g_list->_tail = gNodePtr;

    return ptr;
}

void flushGarbageCollector(int is_long_term)
{
    garbageList *g_list;
    garbageNode *n1, *n2;


    g_list = is_long_term ? glob_garbageList_long_term : glob_garbageList;
    if (g_list == NULL || g_list->_head == NULL)
        return;

    n1 = g_list->_head;
    do
    {
        free(n1->_internalPtr);
        n2 = n1->_nextNode;
        free(n1);
        n1 = n2;
    } while (n1 != NULL);

    g_list->_head = NULL;
    g_list->_tail = NULL;
}

void freeGarbageCollectors()
{
    flushGarbageCollector(0);
    if (glob_garbageList != NULL)
    {
        free(glob_garbageList);
        glob_garbageList = NULL;
    }
    flushGarbageCollector(1);
    if (glob_garbageList_long_term != NULL)
    {
        free(glob_garbageList_long_term);
        glob_garbageList_long_term = NULL;
    }
}
