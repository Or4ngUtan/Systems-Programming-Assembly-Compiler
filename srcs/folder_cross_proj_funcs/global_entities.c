#include "../../hdrs/global_entities.h"
#include <stdio.h>


/*
Here, we initiallize global arrays that specify the available instructions/directives,
the number of operands for each of them,
and the available operand-types for each of the above.
These are hardcoded specifications - there is not necessarily a predictable way to generate them and they depand on the maman's definitions.

We tried to make it as human-readable and maintainance-friendly as possible
*/


/*initiallization of global arrays START*/

char *glob_instrctNamesArr[NUM_INSTRCT_TYPES] = {
    "mov",
    "cmp",
    "add",
    "sub",
    "lea",
    "clr",
    "not",
    "inc",
    "dec",
    "jmp",
    "bne",
    "jsr",
    "red",
    "prn",
    "rts",
    "stop"};

char *glob_drctvNamesArr[NUM_DRCTV_TYPES] = {
    ".data",
    ".mat",
    ".extern",
    ".string",
    ".entry"};

char *glob_INSTRCTS_legalOprndTypes[NUM_INSTRCT_TYPES][2] = {
    /* 2 operands per command at most*/
    /* 0-instant, 1-direct, 2-matCell, 3-reg*/
    {"0123", "123"},  /* mov*/
    {"0123", "0123"}, /* cmp*/
    {"0123", "123"},  /* add*/
    {"0123", "123"},  /* sub*/
    {"12", "123"},    /* lea*/
    {"123", NULL},    /* clr*/
    {"123", NULL},    /* not*/
    {"123", NULL},    /* inc*/
    {"123", NULL},    /* dec*/
    {"123", NULL},    /* jmp*/
    {"123", NULL},    /* bne*/
    {"123", NULL},    /* jsr*/
    {"123", NULL},    /* red*/
    {"0123", NULL},   /* prn*/
    {NULL, NULL},     /* rts*/
    {NULL, NULL}      /* stop*/
};

/*we consider the tokens after a .directive as "operands" too - be it a label, a string-constant, or a sequence of integers (single integer is also seen as a sequence of integers)*/
char *glob_DRCTVS_legalOprndTypes[NUM_INSTRCT_TYPES][2] = {
    /* 2 operands per command at most*/
    /* 0-instant, 1-direct, 2-matCell, 3-reg, 4-intSeq, 5-strConst, 6-matDims*/
    {"4", NULL}, /*.data*/
    {"6", "4"},  /*.mat //can a mat be declared empty? i.e. SOMENAME: .mat [1][2] ? without int sequence - and then fill all cells with blanks; theoretically possible >> this is possible, pdf p.39 */
    {"1", NULL}, /*.extern*/
    {"5", NULL}, /*.string*/
    {"1", NULL}  /*.entry*/
};
