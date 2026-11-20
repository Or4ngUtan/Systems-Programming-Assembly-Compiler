#ifndef PARSING_SUPPORT_H
#define PARSING_SUPPORT_H

#include <stdio.h>
#include "global_entities.h"


extern char glob_parse_failure_reason[MAX_ERR_MSG_LEN];


void resetIsFailReasonActual(void);
void concatFailReason_ifApplicable(ParsedLine* this_line);

int isVisibleAscii(char c);

void getCmdNamesLenExtremities(int *min_len_byRef, int *max_len_byRef);

int trimComment_warnUnclosedStrConst(char *line_str);

void refineLine(char *str);

void file_extractNextLine_updatePos(FILE *asm_file, char *str_buff, int *is_EOF_byRef, int *is_line_too_big_byRef);

ParsedLine *createBlankParsedLine(void);

int isLabel_both_declared_used(ParsedLine *this_line);

int isLegalLabel(char *host_str, int from_pos, int to_pos);


int isLegalInteger_optExtract(char *host_line_str, int from_pos_after_hash, int to_pos, int *opt_int_val_byRef);


int isLegalRegister_optExtract(char *host_line_str, int from_pos, int to_pos, int *opt_reg_serial_byRef);


int isLegalStringConst_optExtract(char *host_line_str, int from_pos, int to_pos, char **opt_str_const_malloced_byRef);


int isLegalSquareBracketsExprssn(char *host_line_str, int first_open_bracket_pos, int (*quad_arr_ptr_byRef)[]);


int isLegalMatDimSingle_optExtract(char *host_line_str, int from_pos, int to_pos, int *opt_int_val_byRef);

#endif /* PARSING_FUNCS_H */
