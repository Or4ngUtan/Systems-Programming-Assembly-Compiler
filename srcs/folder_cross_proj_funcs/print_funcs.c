#include <stdio.h>
#include <string.h>

#include "../../hdrs/global_entities.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/parsing_support.h"
#include "../../hdrs/print_funcs.h"


#include "../../hdrs/struct_MachineWordsList.h"
#include "../../hdrs/struct_IntsList.h"
#include "../../hdrs/struct_AsmLinesList.h"

/*
This file aggregates most of the methods needed for communicating with the standard output,
e.g printing messages to the user, errors, warnings etc.
*/


void printErrWarn_intro_only(int is_warn, int opt_src_line_num){
    if(opt_src_line_num > 0){
        printf("  line %d: ", opt_src_line_num);
    }

    printf(ANSI_BOLD); /*bold text*/
    printf(is_warn ? ANSI_COLOR_YELLOW : ANSI_COLOR_RED); /*yellow or red text*/
    printf("%s: ", is_warn ? "Warning" : "Error"); /*message intro*/
    printf(ANSI_COLOR_RESET); /*reset coloring*/
    printf(ANSI_RESET_BOLD); /*reset bold text*/
}



/*primitive print of err or warning by string and optional src-line-number (put -1 to indicate empty)*/
void printErrWarn_from_str(const char * str, int is_warn, int opt_src_line_num /*put -1 to indicate empty*/){
    /* printf("%s: line %d: ", file_name_str, this_line->_src_asm_line_num);*/
    printErrWarn_intro_only(is_warn, opt_src_line_num);
    printf("%s", str);
    printf("\n");
}

/*complex print for functions that require a symbol's name within the error message*/
void printErrWarn_from_3way_str(const char * str1, const char * str2, const char * str3, int is_warn, int opt_src_line_num){
    printErrWarn_intro_only(is_warn, opt_src_line_num);
    printf("%s", str1);
    printf("'%s'", str2);
    printf("%s", str3);
    printf("\n");
}

/*primitive concatanation - add whatever is sent to the end of the string*/
void concatErrWarn(ParsedLine *curr_parsed_line, const char *str_to_append, int is_warn){
    char * dest_str;

    if(is_warn){
        if(curr_parsed_line->_is_warn_msg == 0)
            dest_str = NULL;
        else
            dest_str = curr_parsed_line->_warnMsg;
    }
    else{
        if(curr_parsed_line->_lineType != LN_ERR)
            dest_str = NULL;
        else
            dest_str = curr_parsed_line->_errMsg;
    }
    
    if(dest_str != NULL){
        strncat(dest_str, str_to_append, strlen(str_to_append));
    }
}

/*advanced addition - add comma if not the first warn*/
void addErrWarn(ParsedLine *curr_parsed_line, const char *str_to_append, int is_warn)
{
    int is_initial;
    char * dest_str;

    if(is_warn){
        is_initial = curr_parsed_line->_is_warn_msg == 0;
        dest_str = curr_parsed_line->_warnMsg;
        curr_parsed_line->_is_warn_msg = 1;
    }
    else{
        is_initial = curr_parsed_line->_lineType != LN_ERR;
        dest_str = curr_parsed_line->_errMsg;
        curr_parsed_line->_lineType = LN_ERR;
    }

    if(!is_initial)
        concatErrWarn(curr_parsed_line, ", ", is_warn); /*add a comma for a second or third warning for same line*/
    else
        nullifyCharArr(dest_str, MAX_ERR_MSG_LEN);
    
    concatErrWarn(curr_parsed_line, str_to_append, is_warn);
}

/*deletes all warnings for a parsed-line-list (deletes the indicator)*/
void nullifyAllWarns(AsmLinesList *this_lines_list)
{
    AsmLineNode * curr_line_node;

    if(this_lines_list == NULL)
        return;
    
    curr_line_node = this_lines_list->_head;
    while(curr_line_node != NULL){
        curr_line_node->_parsed_line->_is_warn_msg = 0;
        curr_line_node = curr_line_node->_next_node;
    }
}


/*adds num of operand to the err/warn msg, without any comma considaration*/
void concatOperandNumToErr(ParsedLine *this_asmLine, int operand_serial, int is_warn)
{
    char addtn_str[MAX_LABEL_LEN] = {'\0'};
    strncat(addtn_str, " (operand ", MAX_LABEL_LEN);
    addtn_str[strlen(addtn_str)] = '1' + operand_serial; /*operand_serial comes as 0/1, human readable format is 1/2*/
    strncat(addtn_str, ")", MAX_LABEL_LEN);
    
    concatErrWarn(this_asmLine, addtn_str, is_warn);
}

/* creates and appends to list a ParsedLine object, in a case that the node is used only for indicating a message (could be line is too long / unclosed string constant)*/
void AsmLinesList_addLine_errOnly(AsmLinesList *this_list, const char *err_msg, int line_num)
{
    ParsedLine *this_line;
    this_line = createBlankParsedLine();
    this_line->_src_asm_line_num = line_num;
    this_line->_is_warn_msg = 0;
    addErrWarn(this_line, err_msg, 0);
    /* this_line->_lineType = LN_ERR; //no need - addErrWarn takes care of this */
    AsmLinesList_addNode(this_list, this_line);
}

/*prints both errs, warns at once*/
void printErrsWarns_per_parsedLine(ParsedLine *this_line){
    if(this_line == NULL)
        return;

    if (this_line->_is_warn_msg)
        printErrWarn_from_str(this_line->_warnMsg, 1, this_line->_src_asm_line_num);
    if (this_line->_lineType == LN_ERR)
        printErrWarn_from_str(this_line->_errMsg, 0, this_line->_src_asm_line_num);
}

/*prints all errs/warns per AsmLinesList in succession*/
void printErrsWarns_per_AsmLinesList(AsmLinesList *this_list){
    AsmLineNode * curr_node;
    
    if(this_list == NULL)
        return;

    curr_node = this_list->_head;
    while(curr_node != NULL){
        printErrsWarns_per_parsedLine(curr_node->_parsed_line);
        curr_node = curr_node->_next_node;
    }
}



void printIntro(const char *oprtn_desc, const char *f_name_plain)
{
    printf("Started %s for file: %s.as\n", oprtn_desc, f_name_plain);
}

/*prints conclusion per an operation per a single file*/
void printConclusion(const char *oprtn_desc, const char *f_name_plain, int is_failure)
{
    printf("%s for file: %s.as %s\n", oprtn_desc, f_name_plain, is_failure ? "had failed" : "successful");
}