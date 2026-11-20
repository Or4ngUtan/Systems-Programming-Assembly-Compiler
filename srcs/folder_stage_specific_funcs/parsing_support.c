#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "../../hdrs/struct_MachineWordsList.h"

#include "../../hdrs/util_funcs.h"
#include "../../hdrs/print_funcs.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/global_entities.h"
#include "../../hdrs/parsing_support.h"

/*
functions here support the "parsing" operation: they are mainly textual analysis functions that are utillized by "expanded_to_parsed.c".
These funcs work in conjunction with "ParsedLine" struct.
These also include functions that handle reading data from (macros-expanded) source files.
*/

/*this is a buffer that will be used to communicate error-messages for the parsing stage*/
char glob_parse_failure_reason[MAX_ERR_MSG_LEN] = {'\0'};
int is_parse_fail_msg_actual = 0;


void resetIsFailReasonActual(void){
    is_parse_fail_msg_actual = 0;
}

void fillFailReasonBuff(const char * msg){
    int msg_len;
    
    msg_len = strlen(msg);
    is_parse_fail_msg_actual = 1;
    strncpy(glob_parse_failure_reason, msg, msg_len);
    glob_parse_failure_reason[msg_len] = '\0';
}

void concatFailReason_ifApplicable(ParsedLine* this_line){
    if(is_parse_fail_msg_actual){
        concatErrWarn(this_line, " - ", 0);
        concatErrWarn(this_line, glob_parse_failure_reason, 0);
    }
}

/*auxilliary functions for parsing START***************************************************/

ParsedLine *createBlankParsedLine()
{
    ParsedLine *this_asmLine;
    this_asmLine = ourMalloc(sizeof(ParsedLine), TEMP_MEM);
    this_asmLine->_1stOprndType = OPRND_NONE;
    this_asmLine->_2ndOprndType = OPRND_NONE;
    this_asmLine->_cmdCode = -1;
    this_asmLine->_src_asm_line_num = 0;
    this_asmLine->_is_line_labeled = 0;
    this_asmLine->_is_warn_msg = 0;
    this_asmLine->_occupy_type = OCCUP_NONE;
    this_asmLine->_mchn_words_lst = NULL;

    /* nullifyCharArr(this_asmLine->_errMsg, MAX_ERR_MSG_LEN - 10); //crucial for putting an end to the string at all //messes these up for some odd reason*/
    /* nullifyCharArr(this_asmLine->_warnMsg, MAX_ERR_MSG_LEN - 10);*/
    return this_asmLine;
}

/*is this char a visible ascii as per the pdf's definition*/
int isVisibleAscii(char c)
{
    return c == ' ' || c == '\t' || /* ASSUMPTION - spaces/tabs are invisible but are visible for our purposes (we assume string const may contain spaces/tabs)*/
           (
               c >= VISIBLE_ASCII_MIN &&
               c <= VISIBLE_ASCII_MAX &&
               c != '\n' &&
               c != EOF);
}

/* returns the smallest and largest possible lengths of a command (instruction or directive)*/
void getCmdNamesLenExtremities(int *min_len_byRef, int *max_len_byRef)
{
    static int min_len = 0, max_len; /*static - this calculation can be done exactly once for the entire prog.*/
    if (min_len == 0)
    {
        int curr_len, i;
        min_len = MAX_ASM_LINE_LEN;
        max_len = 1;
        for (i = 0; i < NUM_INSTRCT_TYPES; i++)
        {
            curr_len = strlen(glob_instrctNamesArr[i]);
            if (curr_len < min_len)
                min_len = curr_len;
            if (curr_len > max_len)
                max_len = curr_len;
        }

        for (i = 0; i < NUM_DRCTV_TYPES; i++)
        {
            curr_len = strlen(glob_drctvNamesArr[i]);
            if (curr_len < min_len)
                min_len = curr_len;
            if (curr_len > max_len)
                max_len = curr_len;
        }
    }
    *min_len_byRef = min_len;
    *max_len_byRef = max_len;
}

/*this gets a raw line from the expanded-file (before refining!), and checks for comments/unclosed str-consts. the null might be nullified if it is empty*/
int trimComment_warnUnclosedStrConst(char *line_str)
{
    int curr_pos_ind, is_inside_str_const;
    char curr_char;

    if (strlen(line_str) == 0)
        return 0;

    /*ASSUMPTION - ';' must appear as the first non-whitespace character (preceeding spaces/tabs are allowed!) of the line to be considered a comment*/
    /*ASSUMPTION - a comment cannot start after a line with actual content*/
    /*if (line_str[0] == ';') //prev. version - only the first char, preceeding spaces/tabs not allowed*/
    if (line_str[skipTabsSpaces(line_str, 0)] == ';') /* if comment. */
    {
        line_str[0] = '\0'; /* nullify this line*/
        return 0;
    }
    /*testing for comment ended*/

    curr_pos_ind = is_inside_str_const = 0;
    do
    {
        curr_char = line_str[curr_pos_ind];
        if (curr_char == '\"')
            is_inside_str_const = !is_inside_str_const;
        curr_pos_ind++;
    } while (curr_char != '\0');

    /* now, if a previous string const is unclosed and weve reached the end of line, then we've got an unclosed-string-const. the line is surely illegal. we return 1;*/
    if (is_inside_str_const && curr_char == '\0')
        return 1;

    /* otherwise the line is legal (so far) and has no comment*/
    return 0;
}

/* we wish to simplify the string-analysis process for each line; it may be done via eliminating unnecessary whitespaces - the meaning of the line remains the same*/
/* thus, here we concatanate redundant whitespaces (consecutive tabs/spaces, whitespaces before/after commas, wrapping whitespaces within square brackets, opening/ending whitespaces)*/
/* we must not touch any whitespace that is within a string constant*/
/* the \n or EOF char at the end is also disposable*/
void refineLine(char *str)
{
    int curr_pos, next_trimmed_pos, len, is_in_str_const;
    int is_prev_comma, is_prev_space, is_prev_brack; /* is_prev_brack: if '[' then 1 if ']' then 2, otherwise 0*/
    int is_curr_comma, is_curr_space, is_curr_brack;
    char curr_c;

    len = strlen(str);
    if (len == 0)
        return;
    
    is_prev_comma = 1;
    is_in_str_const = is_prev_space = is_prev_brack = 0;

    for (curr_pos = next_trimmed_pos = 0; curr_pos < len; curr_pos++)
    {
        curr_c = str[curr_pos];
        /* convert tabs to spaces for simplicity - they have the same function here*/
        if (curr_c == '\t')
            curr_c = ' ';

        /* printf("curr_c is: %c\n", curr_c);*/
        if (!is_in_str_const)
        {
            is_curr_comma = curr_c == ',';
            is_curr_space = curr_c == ' ';

            if (curr_c == '[')
                is_curr_brack = 1;
            else if (curr_c == ']')
                is_curr_brack = 2;
            else
                is_curr_brack = 0;
        }
        else
        {
            is_curr_comma = is_curr_brack = is_curr_space = 0;
        }

        if (curr_c == '\"')
            is_in_str_const = !is_in_str_const;

        if (is_curr_space && (is_prev_comma || is_prev_space || (is_prev_brack == 1)))
        { /*do nothing - disregard current space*/
        }
        else
        {
            if (is_prev_space && (is_curr_comma || (is_curr_brack == 2)))
                str[next_trimmed_pos - 1] = curr_c;
            else
                str[next_trimmed_pos++] = curr_c;

            is_prev_comma = is_curr_comma;
            is_prev_brack = is_curr_brack;
            is_prev_space = is_curr_space;
        }
    }

    if (is_prev_space)
        next_trimmed_pos--;

    str[next_trimmed_pos] = '\0';
}

/* this func extracts the next line from the file onto the given buffer, without the line terminator (can be \n or EOF)*/
/* in case the line is too big, and/or the line terminator is EOF, the indicative byRef args will be updated accordingly*/
/* assuming that the func was called when pos of file was on the first char of the current line*/
void file_extractNextLine_updatePos(FILE *asm_file, char *str_buff, int *is_EOF_byRef, int *is_line_too_big_byRef)
{
    char *fgets_ret_val;
    int is_newline_char_exist;
    char interim_buff[MAX_ASM_LINE_LEN + 2] = {'\0'}; /* +1 for end of line char, +1 for null terminator*/

    *is_EOF_byRef = 0;
    *is_line_too_big_byRef = 0;
    fgets_ret_val = fgets(interim_buff, MAX_ASM_LINE_LEN + 2, asm_file);
    /*printf("after main fgets, line is: S%sE\n", interim_buff);*/

    if (feof(asm_file))
        *is_EOF_byRef = 1;
    else if (fgets_ret_val == NULL)
    {
        printf("unexpected error while reading a line from source file.\n");
        return;
    }

    is_newline_char_exist = strchr(interim_buff, '\n') != NULL || strchr(interim_buff, '\r') != NULL;

    /*printf("feof(asm_file): %d, fgets_ret_val == NULL: %d, is_newline_char_exist: %d; *is_EOF_byRef: %d\n", feof(asm_file), fgets_ret_val == NULL, is_newline_char_exist, *is_EOF_byRef);*/
    if (is_newline_char_exist || *is_EOF_byRef)
    { /*we are ok, line is 80 chars or less, and ends either with newline or with EOF*/
        /*let's nullify every character that is a newline - necessary to simplify parsing*/
        int i;
        for (i = 0; i < MAX_ASM_LINE_LEN + 1; i++)
        {
            if (interim_buff[i] == '\n' || interim_buff[i] == '\r')
                interim_buff[i] = '\0';
        }

        /*finally copy the treated string onto the argument-buffer*/
        nullifyCharArr(str_buff, MAX_ASM_LINE_LEN); /* str_buff was initially put '\0' on the MAX_ASM_LINE_LEN char outside this func*/
        strncpy(str_buff, interim_buff, MAX_ASM_LINE_LEN);
    }
    else
    { /*line has not yet ended. we have to notify line-too-long and also deplete all of the remaining line (that could also end with eof)*/
        *is_line_too_big_byRef = 1;
        while (!is_newline_char_exist)
        {
            fgets_ret_val = fgets(interim_buff, MAX_ASM_LINE_LEN + 2, asm_file);
            if (feof(asm_file))
            {
                *is_EOF_byRef = 1;
                return;
            }
            else if (fgets_ret_val == NULL)
            {
                printf("unexpected error while reading a line from source file.\n");
                return;
            }
            is_newline_char_exist = strchr(interim_buff, '\n') != NULL || strchr(interim_buff, '\r') != NULL;
        }
        /*after this while loop, current position within the file is exactly after the line terminator of the line that was too long and we can proceed onto the next*/
    }
}


/*testing whether a label was declared and referred-to (as operand) on the same line*/
int isLabel_both_declared_used(ParsedLine *this_line)
{
    if (this_line->_is_line_labeled == 0)
        return 0;

    if (
        (this_line->_1stOprndType == OPRND_DIRECT && strncmp(this_line->_line_label, this_line->_1stOprndVal._str, MAX_LABEL_LEN) == 0) ||
        (this_line->_2ndOprndType == OPRND_DIRECT && strncmp(this_line->_line_label, this_line->_2ndOprndVal._str, MAX_LABEL_LEN) == 0))
        return 1;

    return 0;
}

/*auxilliary functions for parsing END****************************************************/

/*legality functions START**************************************************/
/* modifiers for functions: explanation:*/
/*_optExtract - the function checks for legallity of the token, but also enables its return in case it is legal, via a by-reference-argument ("_byRef") */

/* returns 1 if label is legal, 0 otherwise; from and to including; label w.o the colon!*/
int isLegalLabel(char *host_str, int from_pos, int to_pos)
{
    /* 1. starts with an alphabetic (upper/lower case)*/
    /* 2. the label is 30 chars or less*/
    /* 3. contains only alphabetics or numerals*/

    /* on declarations:  - will be checked separately*/
    /* 1. was undeclared before - will be tested when populating the symbol's table*/
    /* 2. no spaces between the label and the colon - will be tested when parsing specifically for a label declaration*/

    int curr_pos;

    /*printf("to_pos: %d, from_pos: %d, to_pos - from_pos + 1: %d\n", to_pos, from_pos, to_pos - from_pos + 1);*/

    if(!(from_pos <= to_pos && to_pos - from_pos + 1 <= MAX_LABEL_LEN)){
        fillFailReasonBuff("illegal label's length");
        return 0;
    }


    if (!isalpha(host_str[from_pos]))
    {
        fillFailReasonBuff("1st character is not alphabetic");
        return 0;
    }

    curr_pos = from_pos;
    do
    {
        curr_pos++;
        if (
            curr_pos <= to_pos &&
            !(isalpha(host_str[curr_pos]) || isdigit(host_str[curr_pos])))
        {
            fillFailReasonBuff("must consist of alpha-numerics only");
            return 0;
        }
    } while (curr_pos <= to_pos);

    if(isReservedword_asSubstr(host_str, from_pos, to_pos)){
        fillFailReasonBuff("is a reserved word");
        return 0;
    }

    return 1;
}

/* returns 1 if the given substring is an integer (without the hash, can have a sign, can be negative)*/
int isLegalInteger_optExtract(char *host_line_str, int from_pos_after_hash, int to_pos, int *opt_int_val_byRef)
{ /* from and to including //from WITHOUT hash (can also be used for testing parts of int sequences, or mat-dims/coords)*/
    int result_num, result_num_sign;
    
    if (!(from_pos_after_hash <= to_pos))
        return 0;

    result_num = 0;
    result_num_sign = 1;
    if (host_line_str[from_pos_after_hash] == '+' || host_line_str[from_pos_after_hash] == '-')
    {
        if (from_pos_after_hash + 1 <= to_pos)
        { /* if we got a starting -/+, than in order for the token to be a valid number, it must be of length 2 at least*/
            if (host_line_str[from_pos_after_hash] == '-')
                result_num_sign = -1;

            from_pos_after_hash++;
        }
        else
            return 0;
    }

    for (; from_pos_after_hash <= to_pos; from_pos_after_hash++)
    {
        if (isdigit(host_line_str[from_pos_after_hash]))
        {
            result_num = result_num * 10 + digitCharToInt(host_line_str[from_pos_after_hash]); /*(int)host_line_str[from_pos] does it work? no, need to subtract '0'*/
            if (!isNumInRange8bits(result_num_sign * result_num)){
                /* number (if next chars are also digits) is out of range*/
                fillFailReasonBuff("an integer is out of range (8-bit signed)");
                return 0;
            }
        }
        else
            return 0;
    }

    if (opt_int_val_byRef != NULL)
        *opt_int_val_byRef = result_num_sign * result_num;
    return 1;
}

/* returns 1 if the given substring is a register (e.g "r0")*/
int isLegalRegister_optExtract(char *host_line_str, int from_pos, int to_pos, int *opt_reg_serial_byRef)
{ /* from and to including*/
    char second_char;
    second_char = host_line_str[to_pos];
    if (from_pos + 1 == to_pos &&
        host_line_str[from_pos] == 'r' &&
        isdigit(second_char) &&
        digitCharToInt(second_char) >= REG_SERIAL_NUM_MIN &&
        digitCharToInt(second_char) <= REG_SERIAL_NUM_MAX)
    {
        if (opt_reg_serial_byRef != NULL)
            *opt_reg_serial_byRef = digitCharToInt(second_char);
        return 1;
    }
    return 0;
}

/* important! if a string const exists then we must ignore any word terminator inside it - they are not actual word terminators*/
/* we might treat string consts same as int sequences - they always terminate the line if they exist - this may help when dealing with them*/
/*^implemented by testing for a string constant with "from_pos" only, while identifying terminating \" with "skipToLastNonBlankInLine"*/
int isLegalStringConst_optExtract(char *host_line_str, int from_pos, int to_pos /*position from skipToLastNonBlankInLine*/, char **opt_str_const_malloced_byRef 	 /*  ** because we want to change the value of a pointer to char*/)
{
    int curr_pos;

    if (!(from_pos != to_pos && host_line_str[from_pos] == '\"' && host_line_str[to_pos] == '\"')) /* must include at least the two double-quotes at the sides*/
        return 0;

    curr_pos = from_pos;

    do
    {
        curr_pos++;
        if (curr_pos < to_pos)
        {
            if (!isVisibleAscii(host_line_str[curr_pos]))
                return 0;
        }
    } while (curr_pos < to_pos);

    if (opt_str_const_malloced_byRef != NULL)
    {
        if (from_pos + 1 == to_pos)
        { /* this is an empty string constant*/
            char *empty_str;
            empty_str = ourMalloc(sizeof(char), TEMP_MEM);
            *empty_str = '\0';
            *opt_str_const_malloced_byRef = empty_str;
        }
        else
            *opt_str_const_malloced_byRef = generateSubstr(host_line_str, from_pos + 1, to_pos - 1);
    }
    return 1;
}

/* this func tests if there is a substring starting at first_open_bracket_pos, that looks like [<expression>][<expression>]*/
/* if so, returns 1 and also assigns the positions of the 4 brackets in order into quad_arr_ptr_byRef*/
int isLegalSquareBracketsExprssn(char *host_line_str, int first_open_bracket_pos, int (*quad_arr_ptr_byRef)[] /*has to be an array (unnecesserily malloced), but not a pointer, so that we can change its values*/)
{
    int curr_pos, is_expecting_opener;
    int i;

    curr_pos = first_open_bracket_pos;
    is_expecting_opener = 1;
    
    for (i = 0; i < 4; i++)
    {
        curr_pos = findNextChar(host_line_str, is_expecting_opener ? '[' : ']', curr_pos);
        if (curr_pos == -1)
            return 0;
        (*quad_arr_ptr_byRef)[i] = curr_pos; /* quad_arr_ptr_byRef is a pointer to an array of ints, so *quad_arr_ptr_byRef is the first int-adress of the by-reference array*/

        is_expecting_opener = !is_expecting_opener;
    }

    if ((*quad_arr_ptr_byRef)[1] + 1 != (*quad_arr_ptr_byRef)[2]){ /* ASSUMPTION: no spacing is allowed between the middle brackets*/
        fillFailReasonBuff("no spacing is allowed between the middle brackets");
        return 0;
    }

    return 1;
}

/* mat dims - like when declaring .mat [2][1] - it receives only the expression within a single pair of brackets (there could without wrapping spaces inside each bracket, but not between them)*/
/* assuming from and to point to the first and last are *digits* (from pos could be a + sign) (not the brackets!) respectively*/
int isLegalMatDimSingle_optExtract(char *host_line_str, int from_pos, int to_pos, int *opt_int_val_byRef)
{
    int dim_size;

    if (to_pos - from_pos < 0) /* empty brackets*/
        return 0;
    if (!isLegalInteger_optExtract(host_line_str, from_pos, to_pos, NULL))
        return 0;

    if (isLegalInteger_optExtract(host_line_str, from_pos, to_pos, &dim_size))
    {
        if (
            dim_size > 0 &&
            isNumInRange8bits(dim_size) /* ASSUMPTION - a dimension of a matrix (cols/rows) must be a positive number that is containable inside a *single signed byte* - thus using isNumInRange8bits*/
        )
        {
            if (opt_int_val_byRef != NULL)
                *opt_int_val_byRef = dim_size;
            return 1;
        }
        else{
            fillFailReasonBuff("a matrix-dimension is negative or out of range (8-bits signed)");
            return 0;
        }
    }
    return 0;
}

/*legality functions END******************************************************/
