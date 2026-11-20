#ifndef EXPANDED_TO_PARSED_H
#define EXPANDED_TO_PARSED_H

#include <stdio.h>
#include "global_entities.h"

AsmLinesList* parseRawAsmFile(FILE* asm_file, int* err_flag_byRef);

#endif
