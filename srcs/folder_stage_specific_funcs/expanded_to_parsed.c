#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "../../hdrs/parsing_support.h"
#include "../../hdrs/print_funcs.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/global_entities.h"

#include "../../hdrs/struct_IntsList.h"
#include "../../hdrs/struct_AsmLinesList.h"

/*
this file deals with "parsing" the expanded asm file, i.e converting the raw text into a structured form where we can access the various properties of each line easily.
our goal here is to receive a .as file, after macros expansion, and analyze each row.
The general idea is that when we recognize an expression, we try to predict what the next expressions may be.
for example, after identifying the type pf the command/directive we narrow the possibilities for the next words.
if all assembly tokens are legal and their combination possible: we update this line with its properties to a struct of the type "ParsedLine"
otherwise - if some expression is misplaced / illegal - we alert about this and mark the line as errored

like this, we create a dynamically-sized list of ParsedLines, that contains all the data that was parsed, including errors.
afterwards we can comfortabley iterate over this list, and start converting it to machine code.
this is the massivest file in the project, as it deals with textual-analysis of all of the possible line-contents.
*/



/***************************************************************************************************************************************** */

/*parsing funcs START*/

/* modifiers for functions: explanation:*/
/*_updatePos - the function also updates the next position to read from the line, if parsing is successful*/
/*_updateErr - the function also puts an error indication in the ParsedLine object, if a definite attempt to parse this type of token was identified (we inferred that the token cannot be of any other token-type)*/

/* this function tests if host-line has an opening-label attempt,*/
/* if theere is no attempt, or the attempt is illegal: we put this_asmLine->_is_line_labeled = 0;*/
/* if there is a definite illegal attempt, we update an error for this_asmLine;*/
/* otherwise (attempted + legal): we put this_asmLine->_is_line_labeled = 1, and also update this_asmLine->_line_label*/
void parseLabelDeclare_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int *curr_char_pos_byRef)
{
    int label_1st_char_pos, colon_pos;

    label_1st_char_pos = skipTabsSpaces(host_line_str, 0);
    colon_pos = findNextChar(host_line_str, ':', label_1st_char_pos);

    /* printf("inside parseLabelDeclare_updatePos_updateErr, colonpos is: %d\n", colon_pos);*/

    if (colon_pos == -1)
        this_asmLine->_is_line_labeled = 0;
    else
    {
        if(colon_pos == label_1st_char_pos ){
            addErrWarn(this_asmLine, "illegal declaration of a label - blank label", 0);
            return;
        }
        else if(isTabOrSpace(host_line_str[colon_pos - 1])){
            addErrWarn(this_asmLine, "illegal declaration of a label - spacing is not allowed before the colon", 0);
            return;
        }

        /* ASSUMPTION - we do not require space between the colon and the expressions to its right*/
        if (label_1st_char_pos < colon_pos &&
            isLegalLabel(host_line_str, label_1st_char_pos, colon_pos - 1))
        {
            this_asmLine->_line_label = generateSubstr(host_line_str, label_1st_char_pos, colon_pos - 1);
            /*printf("lbl is: %s\n", this_asmLine->_line_label);*/
            this_asmLine->_is_line_labeled = 1;
            *curr_char_pos_byRef = colon_pos + 1;
        }
        else
        {
            /* printf("inside illegal declare of label\n");*/
            addErrWarn(this_asmLine, "illegal declaration of a label", 0);
            concatFailReason_ifApplicable(this_asmLine);
        }
    }
}

/* parses an instant integer of the form "#-92"*/
void parseInstantInteger_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int operand_num, int *curr_char_pos_byRef)
{
    /* printf("inside parseInstantInteger_updatePos_updateErr\n");*/
    int from_pos, is_attempt;
    from_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);
    is_attempt = host_line_str[from_pos] == '#';

    if (is_attempt)
    {
        int last_pos, val;
        from_pos++;
        /* printf("from_pos is: %d\n", from_pos);*/
        last_pos = findNextWordTerminator(host_line_str, from_pos) - 1;
        /* printf("inside parseInstant... , last pos is: %d\n", last_pos);*/

        if (isLegalInteger_optExtract(host_line_str, from_pos, last_pos, &val))
        {
            if (operand_num == 0)
            {
                this_asmLine->_1stOprndType = OPRND_INSTANT;
                (this_asmLine->_1stOprndVal)._int = val;
            }
            else
            {
                this_asmLine->_2ndOprndType = OPRND_INSTANT;
                (this_asmLine->_2ndOprndVal)._int = val;
            }
            *curr_char_pos_byRef = last_pos + 1;
        }
        else{
            addErrWarn(this_asmLine, "illegal instant integer", 0);
            concatFailReason_ifApplicable(this_asmLine);
        }
    }
}

/* parses an str constant of the form "bla1 bla2"*/
void parseStringConst_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int operand_num, int *curr_char_pos_byRef)
{
    int from_pos;
    char *extracted_str;

    from_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);

    if (host_line_str[from_pos] == '\"')
    { /* attempted string-const. from here its either success or error - no option for other types of operands*/
        int to_pos;
        to_pos = skipToLastNonBlankInLine(host_line_str); /* a string const will always come as a 2nd operand, and thus the closing-double-quotes will terminate the line*/
        if (host_line_str[to_pos] != '\"')
        { /* it means that after the closer-quotes (they exist, we tested for them while purifying the string) there are more non-blank chars - can't happen on any occasion*/
            addErrWarn(this_asmLine, "excess token(s), after a string-constant", 0);
            return;
        }

        /* potential problem - if there are multiple string-const expressions, it will see the last one and assume it is ok, while it ain't*/
        /* fix: we will test if the next occurance of a string-const-closer-quotes are exactly the last string-const-quotes (there could be potentially double-quotes-escapes within the string-const: \", but we do not address for this)*/
        if (findNextChar(host_line_str, '\"', from_pos + 1) != to_pos)
        {
            addErrWarn(this_asmLine, "identified multiple string-constant expressions", 0);
            return;
        }

        if (isLegalStringConst_optExtract(host_line_str, from_pos, to_pos, &extracted_str))
        {
            /* no! could also come as 1st and only oper. on .string //a string const will always come as a 2nd operand*/
            if (operand_num == 0)
            {
                this_asmLine->_1stOprndType = OPRND_STR_CONST;
                (this_asmLine->_1stOprndVal)._str = extracted_str;
            }
            else
            {
                this_asmLine->_2ndOprndType = OPRND_STR_CONST;
                (this_asmLine->_2ndOprndVal)._str = extracted_str;
            }

            *curr_char_pos_byRef = to_pos + 1; /* for further parsing*/
        }
        else
            addErrWarn(this_asmLine, "illegal string-constant", 0);
    }
}

/* parses the definition of a matrix's-dimensions, like .mat [2][3]*/
void parseMatDims_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int operand_num, int *curr_char_pos_byRef) /**without* an adjucent label*/
{                                                                                                                                  /* ASSUMPTIONS - wrapping spaces within the square-brackets are allowed; spaces between the middle brackets are NOT allowed*/
    /* done > //!!! need to test that the int sequence after the mat dims is included within the mat-dims*/
    /*^to counter this, within "read operand", if cmd is ".mat" then after getting a legal 2nd operand, check if the int sequence is contained within the matrix's dims*/

    /* printf("in parseMatDims_updatePos_updateErr\n");*/
    int first_opener_bracket_pos;
    int quad_arr[4];
    int last_pos; /* pos of last closing bracket*/
    int is_legal;
    int row_coord, col_coord, internal_from, internal_to;
    int i;

    first_opener_bracket_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);
    if (host_line_str[first_opener_bracket_pos] != '[')
        return;

    
    if (!isLegalSquareBracketsExprssn(host_line_str, first_opener_bracket_pos, &quad_arr))
    {
        addErrWarn(this_asmLine, "illegal composition of square-brackets", 0);
        concatFailReason_ifApplicable(this_asmLine);
        return;
    }

    last_pos = quad_arr[3];
    if (findNextWordTerminator(host_line_str, last_pos) != last_pos + 1)
    { /* if the last bracket is not followed by a word terminator then we are in trouble*/
        addErrWarn(this_asmLine, "illegal matrix-dimensions syntax", 0);
        return;
    }

    
    is_legal = 1;
    for (i = 0; i < 2 && is_legal; i++)
    {
        internal_from = skipTabsSpaces(host_line_str, quad_arr[i * 2]) + 1; /* allowing wrapping spaces within the square brackets*/
        internal_to = skipTabsSpaces_reverse(host_line_str, quad_arr[1 + i * 2]) - 1;
        /* printf("internal_from, internal_to: %d, %d\n", internal_from, internal_to);*/
        is_legal = isLegalMatDimSingle_optExtract(host_line_str, internal_from, internal_to, i == 0 ? &row_coord : &col_coord); /* testing for validity and extracting the integer-expressions within the current square bracket*/
    }

    is_legal = is_legal && row_coord > 0 && col_coord > 0; /* dims must be positive*/

    if (is_legal)
    {
        /* this function will be only called with operand_num=0*/
        this_asmLine->_1stOprndType = OPRND_MAT_DIMS;
        (this_asmLine->_1stOprndVal)._matCell = ourMalloc(sizeof(MatCell), TEMP_MEM);
        (this_asmLine->_1stOprndVal)._matCell->_internal_str = NULL;
        (this_asmLine->_1stOprndVal)._matCell->_internal_row = row_coord;
        (this_asmLine->_1stOprndVal)._matCell->_internal_col = col_coord;

        *curr_char_pos_byRef = last_pos + 1;
    }
    else
    {
        addErrWarn(this_asmLine, "illegal definition of matrix-dimensions", 0);
        concatFailReason_ifApplicable(this_asmLine);
    }
}

/* parses a reference to a cell of a matrix, like MAT1[r1 ][ r3]*/
void parseMatCell_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int operand_num, int *curr_char_pos_byRef)
{ /* including both the label and the coordinates e.g mat1[r0][r1]*/
    /* ASSUMPTIONS - wrapping spaces within the square-brackets are allowed; spaces between the label and the first opening bracket are NOT allowed; spaces between the middle brackets are NOT allowed*/
    int from_pos, next_terminator_pos, first_opener_bracket_pos;
    int quad_arr[4];
    int last_pos; /* pos of last closing bracket*/
    int is_legal;
    int row_coord, col_coord, internal_from, internal_to;
    int i;
    
    from_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef); /* must be a label-start-pos, if legal*/
    next_terminator_pos = findNextWordTerminator(host_line_str, from_pos);
    first_opener_bracket_pos = findNextChar(host_line_str, '[', from_pos);

    if (
        first_opener_bracket_pos == -1 ||
        from_pos >= first_opener_bracket_pos || /* label/exprssn-b4-brackets is of length of at least 1*/
        first_opener_bracket_pos >= next_terminator_pos)
        return;

    /* if wev'e reached here then - committed to attempt (can't be another operand type - if not legal then error)*/
    if (!isLegalLabel(host_line_str, from_pos, first_opener_bracket_pos - 1))
    {
        addErrWarn(this_asmLine, "illegal label before a square bracket", 0);
        concatFailReason_ifApplicable(this_asmLine);
        return;
    }
    
    if (!isLegalSquareBracketsExprssn(host_line_str, first_opener_bracket_pos, &quad_arr))
    {
        addErrWarn(this_asmLine, "illegal composition of square-brackets", 0);
        concatFailReason_ifApplicable(this_asmLine);
        return;
    }

    last_pos = quad_arr[3];
    if (findNextWordTerminator(host_line_str, last_pos) != last_pos + 1)
    { /* if the last bracket is not followed by a word terminator then we are in trouble*/
        addErrWarn(this_asmLine, "illegal reference to a matrix-cell", 0);
        return;
    }
    
    is_legal = 1;
    for (i = 0; i < 2 && is_legal; i++)
    {
        internal_from = skipTabsSpaces(host_line_str, quad_arr[i * 2]) + 1; /* allowing wrapping spaces within the square brackets*/
        internal_to = skipTabsSpaces_reverse(host_line_str, quad_arr[1 + i * 2]) - 1;
        is_legal = isLegalRegister_optExtract(host_line_str, internal_from, internal_to, i == 0 ? &row_coord : &col_coord); /* testing for validity and extracting the register-expressions within the current square bracket*/
    }

    is_legal = is_legal && row_coord >= 0 && col_coord >= 0; /* coordinates cannot be negative*/

    if (is_legal)
    {
        OprndVal *curr_oprnd_val_ptr;
        if (operand_num == 0)
        {
            this_asmLine->_1stOprndType = OPRND_MAT_CELL;
            curr_oprnd_val_ptr = &(this_asmLine->_1stOprndVal);
            /* curr_oprnd_val_ptr = (*this_asmLine)._1stOprndVal; <==> <what's written up> <==> curr_oprnd_val_ptr is a pointer that points to the already defined union-pointer that is  _1stOprndVal*/
        }
        else
        {
            this_asmLine->_2ndOprndType = OPRND_MAT_CELL;
            curr_oprnd_val_ptr = &(this_asmLine->_2ndOprndVal);
        }

        /* something here failed after freeing the malloced _matCell/_matCell->_internal_str allocated here. weird*/
        curr_oprnd_val_ptr->_matCell = ourMalloc(sizeof(MatCell), TEMP_MEM);
        curr_oprnd_val_ptr->_matCell->_internal_str = generateSubstr(host_line_str, from_pos, quad_arr[0] - 1 /*1 position before 1st opening bracket*/); /* the label before the coordinates*/
        curr_oprnd_val_ptr->_matCell->_internal_row = row_coord;
        curr_oprnd_val_ptr->_matCell->_internal_col = col_coord;

        *curr_char_pos_byRef = last_pos + 1;
    }
    else
    {
        addErrWarn(this_asmLine, "illegal coordinate-expression(s) within a reference to a matrix-cell", 0);
    }
}

/* parses a sequence of integers, i.e "1, -3,5,  7"*/
void parseIntSeq_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int operand_num, int *curr_char_pos_byRef)
{ /* also allows for a single-int sequence*/
    int from_pos, last_pos;
    int is_legal, is_commited_to_seq;
    IntsList *ints_list;
    int curr_read_val;
    int pos_1, pos_2, pos_comma; /* pos_1 - a start of a number-expression; pos_2 - an end of a number-expression; pos_comma - the pos of the next comma*/

    from_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);
    last_pos = skipToLastNonBlankInLine(host_line_str);

    if (from_pos > last_pos || !isdigit(host_line_str[last_pos])) /* last char of this sequence (that ends in last_pos) must be a digit for this sequence to be legal*/
        return;

    
    is_legal = 1;
    /*is_commited_to_seq = 0; // this tells whether there is a definite attempt of a sequence-expression (i.e we identified a first number + we idebtified a comma) or not (this operand can still be of some other type besides a sequence of integers)*/
    is_commited_to_seq = 1; /*change - a sequence of integers is always a single option as an operand - if the operand is not blank: we assume commited to sequence of integers*/

    ints_list = IntsList_createIntsList(); /* potential problem - memory is allocated even if the sequence will eventually be determined as illegal; mem. waste*/

    
    /* now we gotta analyze the substring starting at from_pos and ending at last_pos, including, and verify it is a valid sequence of numbers (1 or more, divided by commas)*/
    /* ASSUMPTION - we allow free spaces throughout the int-sequence*/
    pos_comma = from_pos - 1;
    do
    {
        pos_1 = pos_comma + 1; /* now we are after the last comma*/
        pos_1 = skipTabsSpaces(host_line_str, pos_1);
        pos_comma = findNextChar(host_line_str, ',', pos_1); /* this is position of the comma*/
        if (pos_comma == -1)                                 /* another comma was not found - it must be the last element of the sequence, and it must end the refined-line*/
            pos_2 = last_pos;
        else
            pos_2 = skipTabsSpaces_reverse(host_line_str, pos_comma - 1); /* this is the last char of an actual to-be-number (after any kind of separator!)*/

        is_legal = is_legal & isLegalInteger_optExtract(host_line_str, pos_1, pos_2, &curr_read_val);

        if (is_legal)
        {
            is_commited_to_seq = 1;
            IntsList_addInt(ints_list, curr_read_val);
        }
    } while (pos_comma != -1);

    if (is_legal && is_commited_to_seq)
    { /* we identified a pattern of a sequence and it is legal*/
        /* no! .data has a single operand that is int seq! // an int sequence will always come as a second operand*/
        if (operand_num == 0)
        {
            this_asmLine->_1stOprndType = OPRND_INT_SEQ;
            (this_asmLine->_1stOprndVal)._ints_list = ints_list;
        }
        else
        {
            this_asmLine->_2ndOprndType = OPRND_INT_SEQ;
            (this_asmLine->_2ndOprndVal)._ints_list = ints_list;
        }
        *curr_char_pos_byRef = last_pos + 1;
    }
    else if (!is_legal && is_commited_to_seq){ /* we identified a pattern of a sequence and it is NOT legal*/
        addErrWarn(this_asmLine, "illegal sequence of integers", 0);
        concatFailReason_ifApplicable(this_asmLine);
    }
}

/* reads the command (instructions + directives), e.g: ... mov ... => type: instruction, cmd_code: 0*/
/* e.g ... .string ... => type: directive, cmd_code: 3*/
void parseLineTypeAndCmdCode_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int *curr_char_pos_byRef) /* assuming the line is not blank nor comment*/
{
    int from_pos;
    int next_word_end_pos, next_word_len;
    int flag_matched;
    int min_possib_len, max_possib_len;

    from_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);
    next_word_end_pos = findNextWordTerminator(host_line_str, from_pos) - 1;
    /*printf("in parseCmd; from_pos, next_word_end_pos: %d, %d \n", from_pos, next_word_end_pos);*/
    next_word_len = next_word_end_pos - from_pos + 1;

    flag_matched = 0;
    getCmdNamesLenExtremities(&min_possib_len, &max_possib_len);
    /* printf("next_word_len, min_possib_len, max_possib_len: %d, %d, %d \n", next_word_len, min_possib_len, max_possib_len);*/
    if (next_word_len >= min_possib_len && next_word_len <= max_possib_len) /* implemented > // might wanna make these constants dynamic, determined on startup, based on the lengths of every command, and put into global vars*/
    {
        char **cmd_names_arr; /* can be a pointer to: either glob_instrctNamesArr, or glob_drctvNamesArr*/
        int cmd_names_arr_len;
        char *substr; /* will contain the to-be command name*/
        int i;

        if (host_line_str[from_pos] == '.')
        { /* it can only be a directive*/
            this_asmLine->_lineType = LN_DRCTV;
            cmd_names_arr = glob_drctvNamesArr;
            cmd_names_arr_len = NUM_DRCTV_TYPES;
        }
        else
        { /* it can only be an instruction*/
            this_asmLine->_lineType = LN_INSTRCT;
            cmd_names_arr = glob_instrctNamesArr;
            cmd_names_arr_len = NUM_INSTRCT_TYPES;
        }

        substr = generateSubstr(host_line_str, from_pos, next_word_end_pos);
        /*printf("to-be-cmd is: %s\n", substr);*/
        
        for (i = 0; i < cmd_names_arr_len && flag_matched == 0; i++)
        {
            /*printf("now trying to compare cmd to: %s\n", cmd_names_arr[i]);*/
            if (strcmp(substr, cmd_names_arr[i]) == 0)
            {
                this_asmLine->_cmdCode = i; /* now cmd_code_byRef will contain the index of the matching instruction/directive in the respective global array*/
                flag_matched = 1;
            }
        }
    }


    if (flag_matched == 0) /* no match was found - unidentified instruction/directive*/
        addErrWarn(this_asmLine, "unidentified/missing instruction/directive", 0);
    else{ /* match found*/
            /*test here: if command is .entry/.extern, and the line is labled. add warning!*/
            if(this_asmLine->_is_line_labeled && (this_asmLine->_cmdCode == (int)DRCTV_ENTRY || this_asmLine->_cmdCode == (int)DRCTV_EXTERN)){
                addErrWarn(this_asmLine, "line with .entry/.extern should not be labled", 1);
            }
            /*test here: if command is .data/.string/.mat, and the line is *not* labled => add warning! (you should define a label for a data-segmant variable)*/
            else if(!this_asmLine->_is_line_labeled && this_asmLine->_lineType == LN_DRCTV &&
                (this_asmLine->_cmdCode == (int)DRCTV_DATA || this_asmLine->_cmdCode == (int)DRCTV_MAT || this_asmLine->_cmdCode == (int)DRCTV_STRING)
            ){
                addErrWarn(this_asmLine, "line with .data/.string/.mat has no label", 1);
            }
        *curr_char_pos_byRef = next_word_end_pos + 1; /* for further parsing*/
    }

    /* printf("after parsing cmd; this_asmLine->_cmdCode: %d\n", this_asmLine->_cmdCode);*/
        
}

/* tries reading a single possible operand of a known type*/
void parseSingleOperandByType_updatePos_updateErr(ParsedLine *this_asmLine, char *host_line_str, int operand_serial, int operand_type, int *curr_char_pos_byRef)
{
    int reg_serial; /* for manual updating within switch (no parsing function for registers - too simple)*/
    int token_start_char_pos, token_end_char_pos;
    enum OprndType *this_type_field_ptr;
    OprndVal *this_opval_field_ptr;

    resetIsFailReasonActual();
    token_start_char_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);
    token_end_char_pos = findNextWordTerminator(host_line_str, token_start_char_pos + 1) - 1; /* might not be relevant, for example with an operand that is a string constant*/    

    /* let's point to the operand's type and value fields (there are 2 pairs for each ParsedLine)*/
    if (operand_serial == 0)
    {
        this_type_field_ptr = &(this_asmLine->_1stOprndType);
        this_opval_field_ptr = &(this_asmLine->_1stOprndVal);
    }
    else
    {
        this_type_field_ptr = &(this_asmLine->_2ndOprndType);
        this_opval_field_ptr = &(this_asmLine->_2ndOprndVal);
    }

    *this_type_field_ptr = OPRND_NONE; /* initiallization of operand-type*/

    /* enum OprndType*/
    /*     {*/
    /*         OPRND_NONE = -1,*/

    /*         OPRND_INSTANT = 0,  // e.g #-1*/
    /*         OPRND_DIRECT = 1,   // e.g x (x is a label)*/
    /*         OPRND_MAT_CELL = 2, // e.g myMat[r0][r1] //ASSUMPTION - inner wrapping spaces/tabs are allowed within the square brackets, but not between the middle brackets/label and first bracket*/
    /*         OPRND_REG = 3,      // e.g r0*/

    /*         OPRND_INT_SEQ = 4,   // i.e integer-sequence, e.g 1,2,-3,   4,+5*/
    /*         OPRND_STR_CONST = 5, // i.e string-constant, e.g "blablaXYZ ..bla" //ASSUMPTION - inner spaces/tabs are allowd inside a string const*/
    /*         OPRND_MAT_DIMS = 6   // i.e matrix-dimensions, e.g [1][5] //ASSUMPTION - inner wrapping spaces/tabs are allowed within the square brackets, but not between the middle brackets*/
    /*     };*/

    /* printf("b4 singl-switch; token_start_char_pos, end: %d, %d\n", token_start_char_pos, token_end_char_pos);*/

    /*switch START*/
    switch ((enum OprndType)operand_type)
    {
    case OPRND_INSTANT:
        parseInstantInteger_updatePos_updateErr(this_asmLine, host_line_str, operand_serial, curr_char_pos_byRef);
        break;

    case OPRND_DIRECT:
        /* printf("trying is_legal_label for the second oper;\n");*/
        if(isalpha(host_line_str[token_start_char_pos])){
            if (isLegalLabel(host_line_str, token_start_char_pos, token_end_char_pos))
            {
                /* printf("determined is_legal_label for the second oper;\n");*/
                *this_type_field_ptr = OPRND_DIRECT;
                this_opval_field_ptr->_str = generateSubstr(host_line_str, token_start_char_pos, token_end_char_pos);
                *curr_char_pos_byRef = token_end_char_pos + 1;
            }
            else{ /*addition: if we're trying to parse a label, then it is the last option for this operand (preceedence is lowest). thus we assume the user meant for a label*/
                addErrWarn(this_asmLine, "illegal operand. In case label was meant", 0);
                concatFailReason_ifApplicable(this_asmLine);
            }
        }
        break;

    case OPRND_MAT_CELL:
        parseMatCell_updatePos_updateErr(this_asmLine, host_line_str, operand_serial, curr_char_pos_byRef);
        break;

    case OPRND_REG:
        if (isLegalRegister_optExtract(host_line_str, token_start_char_pos, token_end_char_pos, &reg_serial))
        {
            /* printf("inside switch, test for reg successful\n");*/
            *this_type_field_ptr = OPRND_REG;
            this_opval_field_ptr->_int = reg_serial;
            *curr_char_pos_byRef = token_end_char_pos + 1;
        }
        break;

    case OPRND_INT_SEQ:
        parseIntSeq_updatePos_updateErr(this_asmLine, host_line_str, operand_serial, curr_char_pos_byRef);
        if (this_asmLine->_lineType != LN_ERR && this_asmLine->_1stOprndType == OPRND_MAT_DIMS && this_asmLine->_2ndOprndType == OPRND_INT_SEQ)
        {
            int mat_size, int_seq_size;
            mat_size = (this_asmLine->_1stOprndVal)._matCell->_internal_row * (this_asmLine->_1stOprndVal)._matCell->_internal_col;
            int_seq_size = IntsList_getLen((this_asmLine->_2ndOprndVal)._ints_list);
            if (int_seq_size > mat_size) /* important! checking here for overflow of elements during .mat definition*/
                addErrWarn(this_asmLine, "number of elements is greater than the size of the defined matrix", 0);
        }
        break;

    case OPRND_STR_CONST:
        parseStringConst_updatePos_updateErr(this_asmLine, host_line_str, operand_serial, curr_char_pos_byRef);
        break;

    case OPRND_MAT_DIMS:
        parseMatDims_updatePos_updateErr(this_asmLine, host_line_str, operand_serial, curr_char_pos_byRef);
        break;

    default:
        break;
    }
    /*switch END*/

    if (this_asmLine->_lineType == LN_ERR)
        concatOperandNumToErr(this_asmLine, operand_serial, 0);
}

/* reads (both! (if 2)) operands after already knowing the line-type and command-code for this line*/
/* the operand type is put inside the field "_1stOprndType", and the value is put, according to its nature, inside the union: _1stOprndVal (its type is OprndVal (union) in order to allow for all value-types that an operand can get)*/
void parseBothOperands_updatePos_updateErr(ParsedLine *this_asmLine,                  /*will be used to then update the operands within the asmLine object, or alternatively an error message*/
                                           char *host_line_str, int *curr_char_pos_byRef /*also updates curr_char_pos from the calling function for further reads*/
)
{
    int cmd_code; /*will be used to determine possible operand types*/
    /* types_preceedance_str will indicate which operand-types should be tried for first and which ones later*/
    /* types_possible_str_both is an array of 2 strings that represent the possible operand types for the current command type, for the first and second operands respectively*/
    char types_preceedance_str[NUM_TYPES_OPERANDS] = {'\0'}, **types_possible_str_both;
    char *types_possible_str_curr, curr_preceed_char, curr_possib_char;
    int operand_num, preceed_str_len, possib_str_len, curr_preceed_pos, curr_possib_pos;
    int comma_pos;
    enum OprndType *curr_typ_ptr;

    this_asmLine->_1stOprndType = this_asmLine->_2ndOprndType = OPRND_NONE; /* initiallization of operand-types*/

    cmd_code = this_asmLine->_cmdCode;
    /* printf("cmd_code is: %d\n", cmd_code); */
    if (this_asmLine->_lineType == LN_INSTRCT)
    {
        strcpy(types_preceedance_str, "0231"); /* 0 - instant-int, 2 - mat-cell, 3 - register, 1 - direct-label*/
        /*^ for example, "direct" (label) should be checked for only if we did not match for any other operand type (a string of "r0" could be considered a label, but we can identify it as a register beforehand)*/
        types_possible_str_both = glob_INSTRCTS_legalOprndTypes[cmd_code];
    }
    else /* it is a directive*/
    {
        strcpy(types_preceedance_str, "6541"); /* 6 - mat dims, 5 - str const, 4 - int seq, 1 - direct-label*/
        /*^ same principle, it is logical to first check for a mat-dims, then string constant, then int-sequence, then label*/
        types_possible_str_both = glob_DRCTVS_legalOprndTypes[cmd_code];
    }

    preceed_str_len = strlen(types_preceedance_str);

    /* now we iterate with nested loops - first, we iterate for each of the 2 operands*/
    /* second, we iterate for each of the operand types by their priority (preceedance)*/
    /* then, we check for each operand-type from the preceedance-string, whether it is possible for this particular cmd code.*/
    /* if so - we try to parse it. sometimes we can identify a form of syntax that obliges the currentnly tested operand type. if so - we do not continue if parsing is unseccessgul*/
    /* printf("parsing operands\n"); */
    for (operand_num = 0; operand_num < 2; operand_num++) /* 2 iterations for a maximum of 2 operands per command*/
    {
        /* printf("oper serial %d\n", operand_num);*/
        curr_typ_ptr = operand_num == 0 ? &(this_asmLine->_1stOprndType) : &(this_asmLine->_2ndOprndType);
        types_possible_str_curr = types_possible_str_both[operand_num]; /* now types_possible_str_curr is a pointer to a single string*/
        if (types_possible_str_curr == NULL)                            /* if operand number <operand_num> should be empty for this cmd*/
            break;
        possib_str_len = strlen(types_possible_str_curr);
        /* now, in case this is the 2nd operand of an instruction (only) then we must read a comma now*/
        if (operand_num == 1 && this_asmLine->_lineType == LN_INSTRCT)
        {
            comma_pos = skipTabsSpaces(host_line_str, *curr_char_pos_byRef);
            if (host_line_str[comma_pos] == '\0' || host_line_str[comma_pos] != ',')
            {
                addErrWarn(this_asmLine, "expecting comma after first operand", 0);
                return;
            }

            if (comma_pos + 1 >= strlen(host_line_str))
            {
                addErrWarn(this_asmLine, "expecting second operand after comma", 0);
                return;
            }

            *curr_char_pos_byRef = comma_pos + 1;

            /* if(operand_num == 1){*/
            /*     printf("now trying to parse operand 2; remaining line is: %s\n", (host_line_str + *curr_char_pos_byRef));*/
            /* }*/
        }

        /*in addition, in case that this is a .mat command, it could or could not specify int-sequence after defining the dimensions.*/
        /*this is the only case when the number of operands for a command can vary*/
        /*thus, if the line ends here - ew infer that the .mat directive is without int-sequence, and it is legal*/
        if(operand_num == 1 && this_asmLine->_lineType == LN_DRCTV && cmd_code == (int)DRCTV_MAT){
            if(*curr_char_pos_byRef == strlen(host_line_str)){ /*this is legal only if we are at the '\0' char of this line right now*/
                (this_asmLine->_2ndOprndVal)._ints_list = NULL;
                return;
            }
            /*otherwise - this is not the end of this line, it has additional contents that can only be int-sequence*/
        }

        for (curr_preceed_pos = 0; *curr_typ_ptr == OPRND_NONE && curr_preceed_pos < preceed_str_len; curr_preceed_pos++)
        { /* iterating over all preceedance-string chars*/
            curr_preceed_char = types_preceedance_str[curr_preceed_pos];

            for (curr_possib_pos = 0; *curr_typ_ptr == OPRND_NONE && curr_possib_pos < possib_str_len; curr_possib_pos++)
            { /* iterating over all possib-string chars*/
                curr_possib_char = types_possible_str_curr[curr_possib_pos];

                if (curr_possib_char == curr_preceed_char)
                {
                    parseSingleOperandByType_updatePos_updateErr(this_asmLine, host_line_str, operand_num, digitCharToInt(curr_possib_char), curr_char_pos_byRef);
                    if (this_asmLine->_lineType == LN_ERR)
                        return; /* error was already updated from parseSingleOperandByType_updatePos_updateErr*/
                }
            } /* curr_possib_pos for END*/
        } /* curr_preceed_pos for END*/

        /* if NONE of the possible operand-types was read successfuly for this operand_num*/
        if (*curr_typ_ptr == OPRND_NONE)
        {
            addErrWarn(this_asmLine, "illegal/missing operand", 0);
            concatOperandNumToErr(this_asmLine, operand_num, 0);
            return;
        }
        else if(*curr_typ_ptr == OPRND_DIRECT && isLabel_both_declared_used(this_asmLine)){ /*check for declaration and usage of the same label on this row*/
            addErrWarn(this_asmLine, "declaration and reference of a label on the same line", 0);
            concatOperandNumToErr(this_asmLine, operand_num, 0);
            return;
        }
    } /* operand_num for END*/
}

/* core function that will be analyzing each line*/
/* this function receives a string that was refined beforehand - of max 80 characters, without unnecessary spaces, without comments, the line is not blank, without line-end/EOF char at the end.*/
/* the function performs textual analysis, and returns an asmLine-object that contains all the needed data for the next steps*/
ParsedLine *parseRawAsmLine(char *host_line_str)
{
    ParsedLine *this_asmLine; /* the line will have its place within the asmLines-nodes-list*/
    int curr_char_pos;
    
    this_asmLine = createBlankParsedLine();
    this_asmLine->_lineType = LN_INIT;
    curr_char_pos = 0;

    /* 1st, we check for an opening label*/
    resetIsFailReasonActual();
    parseLabelDeclare_updatePos_updateErr(this_asmLine, host_line_str, &curr_char_pos);

    /*
    if(strncmp(host_line_str, "K: .data 22",strlen("K: .data 22"))==0){
        printf("here 33-2, err: %d\n", this_asmLine->_lineType == LN_ERR);
    }
    */
    
    /* 2nd, we identify the command*/
    if (this_asmLine->_lineType != LN_ERR){
        resetIsFailReasonActual();
        parseLineTypeAndCmdCode_updatePos_updateErr(this_asmLine, host_line_str, &curr_char_pos);
    }
    
    /* 3rd, we parse for operands according to the command identified*/
    if (this_asmLine->_lineType != LN_ERR)
        parseBothOperands_updatePos_updateErr(this_asmLine, host_line_str, &curr_char_pos);


    /* 4th, we make sure the line ends properly*/
    if (this_asmLine->_lineType != LN_ERR && curr_char_pos != strlen(host_line_str)) /* here, if operands have been read succesfuly but the line is not yet depleted - excess tokens*/
        addErrWarn(this_asmLine, "excess tokens/expressions for this command", 0);


    /* printf("finished parse asm ln\n");*/
    return this_asmLine;
}

/* this func is to be called from main, after macro-expansion*/
/* a version of the parser that cleans each line before sending it to analysis*/
AsmLinesList* parseRawAsmFile(FILE* asm_file, int* err_flag_byRef)
{
    /* rewind(asm_file); //get back to the beginning of the file*/
    ParsedLine* parsed_line_ptr;
    AsmLinesList* ret_list;
    int is_line_err_flag, is_EOF_flag, is_file_err_flag;
    int curr_raw_line_serial_num;
    char curr_line_buff[MAX_ASM_LINE_LEN + 1] = {'\0'}; /*+ 1 for the '\n'/EOF, +1 for the '\0' //fix - +1 only for the '\0'*/
    /*printf("curr_line_buff: '%s'\n", curr_line_buff);*/

    ret_list = AsmLinesList_createAsmLinesList();
    curr_raw_line_serial_num = 1; /* human-readable format*/


    is_file_err_flag = 0;
    do
    {
        parsed_line_ptr = NULL;
        file_extractNextLine_updatePos(asm_file, curr_line_buff, &is_EOF_flag, &is_line_err_flag);
        /*printf("pre-refined line (%d) is: '**%s**'\n", curr_raw_line_serial_num, curr_line_buff);*/

        if (is_line_err_flag)
            AsmLinesList_addLine_errOnly(ret_list, "line exceeds maximum of 80 characters", curr_raw_line_serial_num);
        else
        {
            is_line_err_flag = trimComment_warnUnclosedStrConst(curr_line_buff);
            if (is_line_err_flag)
                AsmLinesList_addLine_errOnly(ret_list, "line has an unclosed string-constant", curr_raw_line_serial_num);
            else
            {
                refineLine(curr_line_buff);
                /*printf("refined line (%d) is: '**%s**'\n", curr_raw_line_serial_num, curr_line_buff);*/
                if (strlen(curr_line_buff) > 0)
                {
                    parsed_line_ptr = parseRawAsmLine(curr_line_buff);
                    parsed_line_ptr->_src_asm_line_num = curr_raw_line_serial_num;
                    is_line_err_flag = parsed_line_ptr->_lineType == LN_ERR;
                    AsmLinesList_addNode(ret_list, parsed_line_ptr);
                }
            }
        }

        is_file_err_flag = is_file_err_flag || is_line_err_flag; /*suffice that one row is errored and the entire file is considered errored*/
        curr_raw_line_serial_num++;
    } while (!is_EOF_flag);

    printErrsWarns_per_AsmLinesList(ret_list);
    *err_flag_byRef = is_file_err_flag;
    return ret_list;
}
