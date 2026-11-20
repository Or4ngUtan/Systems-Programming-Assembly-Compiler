#ifndef PARSED_TO_MACHINE_H
#define PARSED_TO_MACHINE_H
#include "global_entities.h"

/*constants for machine-words-encoding*/

#define STRPOS_GENERAL_FROM 0 /* relevant for data-words such as chars of str-const, single ints from int-sequence*/
#define STRPOS_GENERAL_TO 9
/**/
#define STRPOS_INSTRCT_ARE_FROM 8
#define STRPOS_INSTRCT_ARE_TO 9
/**/
#define STRPOS_ALL_BUT_ARE_FROM 0 /* relevant for instant ints, labels*/
#define STRPOS_ALL_BUT_ARE_TO 7
/**/
#define STRPOS_HDR_CMD_FROM 0
#define STRPOS_HDR_CMD_TO 3
/**/
#define STRPOS_HDR_OP1_ADRSING_FROM 4
#define STRPOS_HDR_OP1_ADRSING_TO 5
/**/
#define STRPOS_HDR_OP2_ADRSING_FROM 6
#define STRPOS_HDR_OP2_ADRSING_TO 7
/**/
#define STRPOS_REGISTS_REG1_FROM 0
#define STRPOS_REGISTS_REG1_TO 3
/**/
#define STRPOS_REGISTS_REG2_FROM 4
#define STRPOS_REGISTS_REG2_TO 7
/**/
#define START_ADDRSS_IC 100 /* dc will be shifted according to the size of the entire instruction-image, after evaluating the size of all words*/

void initMachineCode_per_ParsedLinesList(AsmLinesList *parsed_list);

#endif
