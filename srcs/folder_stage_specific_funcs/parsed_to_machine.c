#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "../../hdrs/parsed_to_machine.h"
#include "../../hdrs/util_funcs.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/global_entities.h"

#include "../../hdrs/struct_IntsList.h"
#include "../../hdrs/struct_MachineWordsList.h"

#include "../../hdrs/expanded_to_parsed.h"


/*
The functions here are used to convert a "ParsedLine" object onto its appropriate machine code.
i.e, after we have each line structured within a "ParsedLine", we can now translate each of its elements to machine-code,
according to the rules given in the maman.
*/


void initMachineWordsList_for_parsed_line(ParsedLine *this_parsed_line)
{
    MachineWordsList *this_mchn_list;
    this_mchn_list = ourMalloc(sizeof(MachineWordsList), TEMP_MEM);
    this_mchn_list->_head = NULL;
    this_mchn_list->_tail = NULL;
    this_parsed_line->_mchn_words_lst = this_mchn_list;
}

/* from, to - including*/
void populateBitsQuestionMarks(char *bits_str, int from_pos, int to_pos)
{
    for (; from_pos <= to_pos; from_pos++)
        bits_str[from_pos] = '?';
}

void populateAREbits(char *bits_str, enum AREbitsType ARE_val)
{
    bits_str[STRPOS_INSTRCT_ARE_FROM] = '0' + (ARE_val == ARE_EXT); /* msb (of the 2 bits reserved for ARE)*/
    bits_str[STRPOS_INSTRCT_ARE_TO] = '0' + (ARE_val == ARE_RE);   /* lsb*/
}

/*for instructions:*/

/* Helper function to determine operand addressing values */
void get_operand_addressing_values(ParsedLine *this_parsed_line, int *src_adrsing_val, int *dest_adrsing_val)
{
    if (this_parsed_line->_1stOprndType == OPRND_NONE) /* addressing method for both operands is 00 (as non existant)*/
    {
        *src_adrsing_val = 0;
        *dest_adrsing_val = 0;
    }
    else if (this_parsed_line->_2ndOprndType == OPRND_NONE)
    {
        *src_adrsing_val = 0;                                       /* addressing method for src operand is 00 (as non existant)*/
        *dest_adrsing_val = (int)(this_parsed_line->_1stOprndType); /* addressing method for dest operand is this_parsed_line->_1stOprndType (00 (as instant)/01/10/11) //important! the first operand actually contains the dest operand in this case!*/
    }
    else
    {
        /* addressing method for src, dest are this_parsed_line->_1stOprndType, this_parsed_line->_2ndOprndType respectively (00 (as instant)/01/10/11)*/
        *src_adrsing_val = (int)(this_parsed_line->_1stOprndType);
        *dest_adrsing_val = (int)(this_parsed_line->_2ndOprndType);
    }
}

/* this func adds to the tail of the mchn-words-lst of this parsed line, the header-machine-word that is found on every instruction*/
void addMchnWord_instrct_hdr(ParsedLine *this_parsed_line)
{
    MachineWord *this_word;
    int src_adrsing_val, dest_adrsing_val;


    this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
    populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_HDR_CMD_FROM, STRPOS_HDR_CMD_TO, this_parsed_line->_cmdCode);

    get_operand_addressing_values(this_parsed_line, &src_adrsing_val, &dest_adrsing_val);

    populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_HDR_OP1_ADRSING_FROM, STRPOS_HDR_OP1_ADRSING_TO, src_adrsing_val);
    populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_HDR_OP2_ADRSING_FROM, STRPOS_HDR_OP2_ADRSING_TO, dest_adrsing_val);
    populateAREbits(this_word->_bits_str, ARE_ABS); /* A.R.E - A*/
}

/* this func adds to the tail of the mchn-words-lst of this parsed line, the a machine-word that represents an instant int*/
void addMchnWord_instant(ParsedLine *this_parsed_line, int val)
{
    MachineWord *this_word;
    this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
    populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_ALL_BUT_ARE_FROM, STRPOS_ALL_BUT_ARE_TO, val);
    populateAREbits(this_word->_bits_str, ARE_ABS); /* A.R.E - A*/
}

/* this func adds to the tail of the mchn-words-lst of this parsed line, the a machine-word that represents an instant int*/
void addMchnWord_label(ParsedLine *this_parsed_line, char *lbl_str /*expecting 's' for src and 'd' for dest*/)
{
    MachineWord *this_word;
    this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
    this_word->_is_label_ref = 1;
    this_word->_lbl_ref_name = lbl_str;
    populateBitsQuestionMarks(this_word->_bits_str, STRPOS_GENERAL_FROM, STRPOS_GENERAL_TO); /* address and ARE are currently unknown - can be either R (01) or E (10)*/
}

/* this func adds to the tail of the mchn-words-lst of this parsed line, the a machine-word that represents an instant int*/
void addMchnWord_mat_cell_ref(ParsedLine *this_parsed_line, char *mat_name_str, int row_reg_serial, int col_reg_serial /*expecting 's' for src and 'd' for dest*/)
{
    MachineWord *addtl_regists_word;


    addMchnWord_label(this_parsed_line, mat_name_str);

    addtl_regists_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
    populateBitsWithNum_byDecimal(addtl_regists_word->_bits_str, STRPOS_REGISTS_REG1_FROM, STRPOS_REGISTS_REG1_TO, row_reg_serial);
    populateBitsWithNum_byDecimal(addtl_regists_word->_bits_str, STRPOS_REGISTS_REG2_FROM, STRPOS_REGISTS_REG2_TO, col_reg_serial);
    populateAREbits(addtl_regists_word->_bits_str, ARE_ABS); /* A.R.E - A*/
}

/* if this func is called for the first time - [identified an src operand as regist] or [src operand was not regist and dest operand is a regist] -> a new word will be created*/
/* if this func is called for the second time for this parsed line - that means that the src operand is regist and now we are trying to update the dest operand as regist - so we only mod the previously created regist-machine-word that already contains date about the src regist, with the data about the dest regist*/
/* Helper function to check if the current operand is the second register operand */
int is_second_register_operand(ParsedLine *this_parsed_line, char operand_direction, int total_operands)
{
    return (total_operands == 2 && operand_direction == 'd' && this_parsed_line->_1stOprndType == OPRND_REG);
}

void add_or_mod_MchnWord_regists(ParsedLine *this_parsed_line, char curr_operand_drctn /*'s' - src, 'd' - dest*/, int num_operands, int reg_val)
{
    MachineWord *this_word;
    if (is_second_register_operand(this_parsed_line, curr_operand_drctn, num_operands))
    { /* that can only mean that we now populate the dest operand that is of type register, and an registers-mchn-word (not one for a matrix-cell-ref!) already exists*/
        /* printf("2nd call identified; reg serial is: %d; tail word is: %s\n", reg_val, this_parsed_line->_mchn_words_lst->_tail->_mchn_word._bits_str);*/
        this_word = &(((this_parsed_line->_mchn_words_lst)->_tail)->_mchn_word);
    }
    else
    { /* this can only be the first call for adding a registers-word; first, we create it and initialize to 0000-0000-xx.*/
        this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
        populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_GENERAL_FROM, STRPOS_GENERAL_TO, ARE_ABS); /* incl. A.R.E - A*/
    }

    populateBitsWithNum_byDecimal(this_word->_bits_str,
                                  curr_operand_drctn == 's' ? STRPOS_REGISTS_REG1_FROM : STRPOS_REGISTS_REG2_FROM,
                                  curr_operand_drctn == 's' ? STRPOS_REGISTS_REG1_TO : STRPOS_REGISTS_REG2_TO,
                                  reg_val);
}

/*for directives:*/

/* this func creates machine words for a .data (ints-sequence) directive*/
void addMchnWords_data(ParsedLine *this_parsed_line, IntsList *int_seq_ptr)
{
    IntNode *curr_node;
    /* int curr_val; */
    MachineWord *this_word;


    if (int_seq_ptr == NULL)
        return;

    curr_node = int_seq_ptr->_head;
    while (curr_node != NULL)
    {
        this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
        populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_GENERAL_FROM, STRPOS_GENERAL_TO, curr_node->_num);
        curr_node = curr_node->_next_node;
    }
}

/* this func creates machine words for a .mat (label + ints-sequence) directive //the ints sequence might be null! and then wwe simply pad the size of the matrix with 0's*/
void addMchnWords_mat(ParsedLine *this_parsed_line)
{
    MatCell *mat_dims_struct;
    IntsList *given_seq_struct;
    int mat_size, given_seq_size;
    MachineWord *this_word;
    int i;


    mat_dims_struct = (this_parsed_line->_1stOprndVal)._matCell;
    given_seq_struct = (this_parsed_line->_2ndOprndVal)._ints_list;
    mat_size = mat_dims_struct->_internal_row * mat_dims_struct->_internal_col;
    given_seq_size = given_seq_struct == NULL ? 0 : IntsList_getLen(given_seq_struct); /*can be 0!!!*/

    /* populating the explicitly specified ints from the ints-sequence*/
    if (given_seq_size > 0)
    {
        addMchnWords_data(this_parsed_line, given_seq_struct);
    }

    /* populating the remaining space of the matrix with 0's*/
    for (i = given_seq_size; i < mat_size; i++)
    {
        this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
        populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_GENERAL_FROM, STRPOS_GENERAL_TO, MATRIX_PADDING_VALUE);
    }
}

/* this func creates machine words for a .string directive (a word for each char + terminating 00..0)*/
void addMchnWords_str_const(ParsedLine *this_parsed_line)
{
    char *this_str_const;
    int str_const_len;
    MachineWord *this_word;
    int curr_char, i;


    this_str_const = (this_parsed_line->_1stOprndVal)._str;
    str_const_len = strlen(this_str_const);

    for (i = 0; i <= str_const_len; i++)
    {
        if (i == str_const_len)
            curr_char = STRING_TERMINATOR_VALUE; /* for the terminating '0000' word for this string-const*/
        else
            curr_char = (int)(this_str_const[i]);

        this_word = MachineWordsList_addNode(this_parsed_line->_mchn_words_lst);
        populateBitsWithNum_byDecimal(this_word->_bits_str, STRPOS_GENERAL_FROM, STRPOS_GENERAL_TO, curr_char);
    }
}

/*for some operand of an instruction*/
void addMchnWords_instrct_oprnd(ParsedLine *this_parsed_line, char curr_operand_drctn /*'s' - src, 'd' - dest*/, int num_operands)
{
    enum OprndType curr_oper_typ;
    OprndVal *op_val_union_ptr;

    if (num_operands == 1 || (num_operands == 2 && curr_operand_drctn == 's'))
    { /* we should extract the type+val from the 1st-operand-fields (could be either dest or src)*/
        curr_oper_typ = this_parsed_line->_1stOprndType;
        op_val_union_ptr = &(this_parsed_line->_1stOprndVal);
    }
    else
    { /* we should extract the type+val from the 2nd-operand-fields (could only be dest)*/
        curr_oper_typ = this_parsed_line->_2ndOprndType;
        op_val_union_ptr = &(this_parsed_line->_2ndOprndVal);
    }

    if (curr_oper_typ == OPRND_REG)
        add_or_mod_MchnWord_regists(this_parsed_line, curr_operand_drctn, num_operands, op_val_union_ptr->_int);
    else
    {
        MatCell *mat_cell_ptr;

        switch (curr_oper_typ)
        {
        case OPRND_INSTANT:
            addMchnWord_instant(this_parsed_line, op_val_union_ptr->_int);
            break;
        case OPRND_DIRECT:
            addMchnWord_label(this_parsed_line, op_val_union_ptr->_str);
            break;
        case OPRND_MAT_CELL:
            mat_cell_ptr = op_val_union_ptr->_matCell;
            addMchnWord_mat_cell_ref(this_parsed_line,
                                     mat_cell_ptr->_internal_str,
                                     mat_cell_ptr->_internal_row,
                                     mat_cell_ptr->_internal_col);
            break;
        default:
            break;
        } /* switch end*/
    }
}
/*for an entire parsed-line*/

void createMachineWords_perParsedLine_1st_pass(ParsedLine *this_parsed_line)
{
    /* order of actions:*/
    /* 1. if ln is errored - create nothing, go to next parsedLine*/
    /* 2. if line is ./entry/.extern - no machine words should be added*/
    /* 3. if .data/.mat/.string - add with a special method for each type*/
    /* 4. if an instruction - create the "header machine word"*/
    /*  4.1. if it has 2 operands - add 2 operands of type src, dest*/
    /*  4.2. if it has 1 operand - add 1 operand of type dest only*/

    if (this_parsed_line->_lineType == LN_ERR)
    {
        this_parsed_line->_mchn_words_lst = NULL;
        this_parsed_line->_occupy_type = OCCUP_NONE;
        return;
    }

    initMachineWordsList_for_parsed_line(this_parsed_line);

    if (this_parsed_line->_lineType == LN_DRCTV)
    {
        if (this_parsed_line->_cmdCode == DRCTV_EXTERN /*.extern*/ || this_parsed_line->_cmdCode == DRCTV_ENTRY /*.entry*/)
        {
            this_parsed_line->_occupy_type = OCCUP_NONE;
            this_parsed_line->_mchn_words_lst = NULL;
            return;
        }
        else
        { /* it is .data (0) / .mat (1) / .string (3)*/
            this_parsed_line->_occupy_type = OCCUP_DC;
            if (this_parsed_line->_cmdCode == DRCTV_DATA) /*.data*/
                addMchnWords_data(this_parsed_line, (this_parsed_line->_1stOprndVal)._ints_list);
            else if (this_parsed_line->_cmdCode == DRCTV_MAT) /*.mat*/
                addMchnWords_mat(this_parsed_line);
            else if (this_parsed_line->_cmdCode == DRCTV_STRING) /*.string*/
                addMchnWords_str_const(this_parsed_line);
        }
    }
    else /* if (this_parsed_line->_lineType == LN_INSTRCT)*/
    {
        int num_operands;

        this_parsed_line->_occupy_type = OCCUP_IC;
        addMchnWord_instrct_hdr(this_parsed_line); /* creates and appends the first machine word - the "instruction-header word" to the machine-words-list*/

        num_operands = 2;
        num_operands -= this_parsed_line->_1stOprndType == OPRND_NONE;
        num_operands -= this_parsed_line->_2ndOprndType == OPRND_NONE;

        if (num_operands == 0)
        {
        } /* do nothing*/
        else if (num_operands == 1)
        { /* this line has only 1 operand - dest operand*/
            addMchnWords_instrct_oprnd(this_parsed_line, 'd', 1);
        }
        else
        { /*(num_operands == 2)*/
            addMchnWords_instrct_oprnd(this_parsed_line, 's', 2);
            addMchnWords_instrct_oprnd(this_parsed_line, 'd', 2);
        }
    }

    this_parsed_line->_count_mchn_words = MachineWordsList_getLen(this_parsed_line->_mchn_words_lst);
}

/*offsetting DC to start exactly after IC image*/
void offsetDCaddresses(AsmLinesList *parsed_list)
{
    int IC_last;
    AsmLineNode *curr_parsed_node_ptr;
    ParsedLine *curr_parsed_line_ptr;


    if (parsed_list->DC_section_len == 0)
        return;

    IC_last = START_ADDRSS_IC + parsed_list->IC_section_len;    
    curr_parsed_node_ptr = parsed_list->_head;
    while (curr_parsed_node_ptr != NULL)
    {
        curr_parsed_line_ptr = curr_parsed_node_ptr->_parsed_line;
        if (curr_parsed_line_ptr->_occupy_type == OCCUP_DC)
            curr_parsed_line_ptr->_relative_addrss += IC_last;
        curr_parsed_node_ptr = curr_parsed_node_ptr->_next_node;
    }
}

/*populating addresses for all individual machine words - will be used for later infrom from which address was an invokation of a label (ext file and other warnings)*/
void populateIndividualWordAdresses(AsmLinesList *parsed_list)
{
    AsmLineNode *curr_parsed_node_ptr;
    ParsedLine *curr_parsed_line_ptr;
    MachineWordNode *curr_mchn_node;
    int curr_mchn_word_addrss;
    curr_parsed_node_ptr = parsed_list->_head;
    while (curr_parsed_node_ptr != NULL)
    {
        curr_parsed_line_ptr = curr_parsed_node_ptr->_parsed_line;

        if (curr_parsed_line_ptr->_occupy_type != OCCUP_NONE)
        {

            curr_mchn_node = curr_parsed_line_ptr->_mchn_words_lst->_head;
            curr_mchn_word_addrss = curr_parsed_line_ptr->_relative_addrss;
            while (curr_mchn_node != NULL)
            {
                (curr_mchn_node->_mchn_word)._relative_addrss = curr_mchn_word_addrss++;
                curr_mchn_node = curr_mchn_node->_next_node;
            }
        }

        curr_parsed_node_ptr = curr_parsed_node_ptr->_next_node;
    }
}

/*this func  adds machine code to an entire parsedLinesList, and also calculates each line's/word's final address*/
void initMachineCode_per_ParsedLinesList(AsmLinesList *parsed_list)
{
    AsmLineNode *curr_parsed_node_ptr;
    ParsedLine *curr_parsed_line_ptr;
    int counter_IC, counter_DC_pre_offset;


    curr_parsed_node_ptr = parsed_list->_head;
    counter_IC = START_ADDRSS_IC;
    counter_DC_pre_offset = 0;
    while (curr_parsed_node_ptr != NULL)
    {
        curr_parsed_line_ptr = curr_parsed_node_ptr->_parsed_line;
        createMachineWords_perParsedLine_1st_pass(curr_parsed_line_ptr);

        if (curr_parsed_line_ptr->_occupy_type == OCCUP_IC)
        {
            curr_parsed_line_ptr->_relative_addrss = counter_IC;
            counter_IC += curr_parsed_line_ptr->_count_mchn_words;
        }
        else if (curr_parsed_line_ptr->_occupy_type == OCCUP_DC)
        {
            curr_parsed_line_ptr->_relative_addrss = counter_DC_pre_offset;
            counter_DC_pre_offset += curr_parsed_line_ptr->_count_mchn_words;
        }
        /*some parsedLines might not be specified occupation of IC or DC - for example .extern/,entry - they do not appear in the instruction/data images*/

        curr_parsed_node_ptr = curr_parsed_node_ptr->_next_node;
    }

    parsed_list->IC_section_len = counter_IC - START_ADDRSS_IC;
    parsed_list->DC_section_len = counter_DC_pre_offset;

    offsetDCaddresses(parsed_list);
    populateIndividualWordAdresses(parsed_list);
}

