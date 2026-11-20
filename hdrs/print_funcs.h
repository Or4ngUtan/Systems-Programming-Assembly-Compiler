#ifndef PRINT_FUNCS_H
#define PRINT_FUNCS_H

#include "global_entities.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"
#define ANSI_RESET_BOLD    "\x1b[m"

void printErrWarn_line_num_only(int opt_src_line_num);
void printErrWarn_intro_only(int is_warn, int opt_src_line_num);
void printErrWarn_from_str(const char * str, int is_warn, int opt_src_line_num /*put -1 to indicate empty*/);
void printErrWarn_from_3way_str(const char * str1, const char * str2, const char * str3, int is_warn, int opt_src_line_num);
void concatErrWarn(ParsedLine *curr_parsed_line, const char *str_to_append, int is_warn);
void addErrWarn(ParsedLine *curr_parsed_line, const char *str_to_append, int is_warn);
void nullifyAllWarns(AsmLinesList *this_lines_list);
void concatOperandNumToErr(ParsedLine *this_asmLine, int operand_serial, int is_warn);
void AsmLinesList_addLine_errOnly(AsmLinesList *this_list, const char *err_msg, int line_num);
void printErrsWarns_per_parsedLine(ParsedLine *this_line);
void printErrsWarns_per_AsmLinesList(AsmLinesList *this_list);
void printIntro(const char *oprtn_desc, const char *f_name_plain);
void printConclusion(const char *oprtn_desc, const char *f_name_plain, int is_failure);


#endif