#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../hdrs/preprocess.h"
#include "../../hdrs/global_entities.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/print_funcs.h"
#include "../../hdrs/util_funcs.h"

#include "../../hdrs/struct_MacroRow.h"


/*
* This file deals with preprocessing (macros expansion)
* The main idea here is to manage a flag that indicates if we are within a macro definition.
* if so - then the following rows should be considered a macro-body, and saved to an object called "MacroRow" that is wrapped within a "MacroList" along with the other macros
* otherwise - if we identify a referance to an already defined macro, we paste its body instead
* otherwise - just copy the original line
* stop when eof is reached (equal fgets return val is NULL)
*
* the body of a macro is theoretically unlimited in size and can span over multiple rows: we need to enlarge its memory on the fly
* it gets complicated because we do not know what will the macro-body's size be, until we reach mcroend.
* thus, we use a set of special functions and global variables, that realloc and free memory for the macro-body that is being parsed at the moment
*/

/*global vars for communicating current macro's name and body between functions*/
MacroList* this_mcro_list;
char *glob_curr_body_realloced, *glob_curr_name_ourmalloced;
int glob_is_body_realloc_live, glob_body_realloc_len;
int glob_line_num;

void resetBodyVars(void){
    glob_curr_body_realloced = NULL;
    glob_curr_name_ourmalloced = NULL;
    glob_is_body_realloc_live = 0;
    glob_body_realloc_len = 0;
}

/*also initiallizes the body, len, is-live if needed*/
void addToReallocedBody(char* addition_str, int addition_len){
    /* printf("now adding to body of '%s': '%s'\n", glob_curr_name_ourmalloced, addition_str == NULL ? "null" : addition_str); */
    if(glob_is_body_realloc_live == 0){
        glob_body_realloc_len = addition_len + 1;
        glob_curr_body_realloced = calloc(glob_body_realloc_len, 1); /*1 - always for chars*/
    }
    else{
        glob_body_realloc_len += addition_len;
        /* printf("just b4 realloc, glob_body_realloc_len: %d; addition_len: %d\n", glob_body_realloc_len, addition_len);*/
        glob_curr_body_realloced = realloc(glob_curr_body_realloced, (size_t)glob_body_realloc_len);
    }
    
    strncat(glob_curr_body_realloced, addition_str, addition_len);
    glob_curr_body_realloced[glob_body_realloc_len - 1] = '\0';

    glob_is_body_realloc_live = 1;
}

/*passes the realloced body to a macroRow, and resets body-vars*/
void storeLegalMacroRow_reset_vars(MacroList* this_list){
    if(glob_is_body_realloc_live == 0){ /*macro ended legal but empty (or only blank lines)*/
        glob_curr_body_realloced = calloc(1, 1);
    }
    MacroList_addNode(this_list, glob_curr_name_ourmalloced, glob_curr_body_realloced);
    resetBodyVars(); /*now, we "detached" this pointer from the current scope;*/
    /*Now, the only access to the realloced string is through the MacroRow; later, after finishing the preprocessing - we will free all the bodies at once: they are not needed anymore after the preprocessing*/
}

/*frees current body-var, and resets body-vars*/
void abandonBody_reset_vars(void){
    /* printf("b4 abandon; is_live: %d\n", glob_is_body_realloc_live);*/
    if(glob_is_body_realloc_live)
        free(glob_curr_body_realloced);
    /*we don't care about the ourmalloced name; this is small money. it will be freed by flushGarbageCollector later*/
    resetBodyVars();
}

/********************************************** */

/*textual analysis functions for the preprocessor START*/

int isLineUnclosed(char * str){
    int len;
    char last_ch;
    
    if(str == NULL)
        return 0;

    len = strlen(str);
    if(len > 1){
        last_ch = str[len - 1];
        if(last_ch == '\n' || last_ch == '\r' || last_ch == EOF)
            return 0;
        else
            return 1; /*line has no newline/eof at its end*/
    }

    return 0;
}


char *sikpWhiteSpaces(char *ch_ptr)
{
    while (ch_ptr != NULL && isspace(*ch_ptr))
        ch_ptr++;
    return ch_ptr;
}

int isLineEmpty(const char *ch_ptr)
{
    return (*ch_ptr == '\0' || *ch_ptr == '\n');
}

int isValidMacroName(const char *str)
{
    if (str == NULL || !isalpha(*str))
        return 0;
    while (str != NULL && *str != '\n' && !isspace(*str))
    {
        if (!isdigit(*str) && !isalpha(*str) && *str != '_')
            return 0;
        str++;
    }
    return 1;
}

int isMacroCall(const char *line, const char *macro_name)
{
    char next_char;
    const char *ch_ptr;
    int len;

    if (line == NULL || macro_name == NULL)
        return 0;
    
    len = strlen(macro_name);
    if (len == 0)
        return 0;

    ch_ptr = line;
    while (ch_ptr != '\0' && isspace(*ch_ptr))
        ch_ptr++;

    
    if (strncmp(ch_ptr, macro_name, len) == 0)
    {
        next_char = ch_ptr[len];
        return (next_char == '\0' || next_char == '\n' || isspace(next_char));
    }
    return 0;
}

/*textual analysis functions for the preprocessor END*/

int setFiles_forPreprocess(FileNode *current_file_node, FILE **input_f_ptr_ptr, FILE **output_f_ptr_ptr)
{
    char *origin_f_path_w_exten, *new_f_path_w_exten;

    *input_f_ptr_ptr = *output_f_ptr_ptr = NULL;
    origin_f_path_w_exten = new_f_path_w_exten = NULL;


    origin_f_path_w_exten = appendExtension_to_f_name(current_file_node->file_name, "as");
    *input_f_ptr_ptr = fopen(origin_f_path_w_exten, "r");
    if (*input_f_ptr_ptr == NULL)
    {
        printErrWarn_from_str("Failed to open source file", 0, -1);
        return 1;
    }

    new_f_path_w_exten = appendExtension_to_f_name(current_file_node->file_name, "am");
    *output_f_ptr_ptr = fopen(new_f_path_w_exten, "w");
    if (*output_f_ptr_ptr == NULL)
    {
        printErrWarn_from_str("Failed to create an .am file", 0, -1);
        fclose(*input_f_ptr_ptr);
        return 1;
    }
    return 0;
}

int startMacroDef(char *curr_pos_ptr, int *in_macro_def)
{
    char *name_start, *name_end, *after_end;
    int name_len;


    if (*in_macro_def)
    {
        printErrWarn_from_str("Nested macro definition", 0, glob_line_num);
        return 1;
    }

    name_start = curr_pos_ptr + MCRO_STR_LEN;
    name_start = sikpWhiteSpaces(name_start);
    if (*name_start == '\0' || *name_start == '\n')
    {
        printErrWarn_from_str("Missing macro name after 'mcro'", 0, glob_line_num);
        return 1;
    }

    if (!isValidMacroName(name_start))
    {
        printErrWarn_from_str("Illegal macro name - must start with an alphabetic and consist of: alpha-numerics / underscores only", 0, glob_line_num);
        return 1;
    }

    name_end = name_start;
    while (*name_end && !isspace(*name_end) && *name_end != '\n')
        name_end++;

    
    after_end = sikpWhiteSpaces(name_end);


    if (*after_end && *after_end != '\n' && *after_end != '\r')
    {
        printErrWarn_from_str("Extra characters after macro name", 0, glob_line_num);
        return 1;
    }


    name_len = name_end - name_start;
    glob_curr_name_ourmalloced = (char *)ourMalloc(name_len + 1, TEMP_MEM);
    nullifyCharArr(glob_curr_name_ourmalloced, name_len + 1);
    strncpy(glob_curr_name_ourmalloced, name_start, name_len);


    if (isReservedword(glob_curr_name_ourmalloced))
    {
        printErrWarn_from_str("macro name is a Reserved word", 0, glob_line_num);
        return 1;
    }


    if (FindMacro(this_mcro_list, glob_curr_name_ourmalloced) != NULL)
    {
        printErrWarn_from_str("macro name is defined more than once", 0, glob_line_num);
        return 1;
    }

    *in_macro_def = 1;
    return 0;
}

int endMacroDef(char *curr_pos_ptr, int *in_macro_def)
{
    char *after_end;
    
    after_end = curr_pos_ptr + MCROEND_STR_LEN;
    after_end = sikpWhiteSpaces(after_end);


    if (*after_end && *after_end != '\n' && *after_end != '\0' && *after_end != '\r')
    {
        printErrWarn_from_str("Extra characters after 'mcroend'", 0, glob_line_num);
        return 1;
    }
    if (!*in_macro_def)
    {
        printErrWarn_from_str("mcroend without mcro", 0, glob_line_num);
        return 1;
    }
    
    
    storeLegalMacroRow_reset_vars(this_mcro_list);
    *in_macro_def = 0;

    return 0;
}

/*this function handles lines that are not mcro nor mcroend - they can either be a referance to an already defined macro, or simply an original assembly line*/
int handleContentLine(char *line, char *curr_pos_ptr, FILE *output_f)
{
    MacroRow* curr_macro_row;

    curr_macro_row = this_mcro_list->_head;
    while(curr_macro_row != NULL){
        if (isMacroCall(curr_pos_ptr, curr_macro_row->name))
        {
            fputs(curr_macro_row->body, output_f);
            return 1; /*1 means we encountered a referance of the macro*/
        }
        curr_macro_row = curr_macro_row->next;
    }

    /*otherwise: this is a regular line*/
    fputs(line, output_f);
    return 0;
}


/*main function that reads line after line with fgets, identifies lines with 'mcro', 'mcroend', and replaces/copies as is the lines that are outside of macro definitions */
int preprocessFile(FileNode *current_file_node)
{
    FILE *input_f_ptr, *output_f_ptr;
    int in_macro_def, is_err;
    char *curr_pos_ptr;
    char line[MAX_ASM_LINE_LEN + 2] = {'\0'};

    in_macro_def = is_err = 0;
    glob_line_num = 0;

    current_file_node->_macro_list = MacroList_createBlankList();
    this_mcro_list = current_file_node->_macro_list;


    glob_curr_body_realloced = NULL;
    glob_curr_name_ourmalloced = NULL;
    glob_is_body_realloc_live = 0;
    glob_body_realloc_len = 0;

    /* printf("glob_curr_body_realloced==NULL: %d, glob_curr_name_ourmalloced==NULL: %d, glob_is_body_realloc_live: %d, glob_body_realloc_len: %d\n", glob_curr_body_realloced==NULL, glob_curr_name_ourmalloced==NULL, glob_is_body_realloc_live, glob_body_realloc_len); */


    resetBodyVars();
    if (setFiles_forPreprocess(current_file_node, &input_f_ptr, &output_f_ptr) == 1) /*sets file and acts according to the return value (errors are printed inside)*/
        return 1;
    
    while (!is_err && fgets(line, sizeof(line), input_f_ptr) != NULL)
    {
        glob_line_num++;
        curr_pos_ptr = sikpWhiteSpaces(line);

        /*
        printf("inside central while; line_num: %d; is inside: %d\n", glob_line_num, in_macro_def);
        if(in_macro_def){
            printf("curr mcroName: %s\n", glob_curr_name_ourmalloced);
            printf("curr mcroBody: %s\n", glob_curr_body_realloced != NULL ? glob_curr_body_realloced : "null");
        }
        */
        

        if (isLineEmpty(curr_pos_ptr))  /*empty line */
        {
            if (!in_macro_def)
                fputs(line, output_f_ptr);
        }
        else if(isLineUnclosed(line) && !feof(input_f_ptr)){ /*if line is unclosed with a \n or \r and it is not EOF now - it surely exceed 80 chars as we limited fgets with MAX_ASM_LINE_LEN*/
            printErrWarn_from_str("line length exceeds 80 characters", 0, glob_line_num);
            is_err = 1;
        }
        /*ASSUMPTION: we allow spaces before and after 'mcro' as well as 'mcroend'*/
        else if (strncmp(curr_pos_ptr, "mcro", MCRO_STR_LEN) == 0 && isspace(curr_pos_ptr[MCRO_STR_LEN])) /*this is a line that opens with 'mcro'*/
        {
            if (startMacroDef(curr_pos_ptr, &in_macro_def) == 1) /*error on a mcro line*/
                is_err = 1;
        }
        else if (strncmp(curr_pos_ptr, "mcroend", MCROEND_STR_LEN) == 0) /*this is a line that opens with 'mcroend'*/
        {
            if (endMacroDef(curr_pos_ptr, &in_macro_def) == 1){ /*error on a mcroend line*/
                /* printf("failure macroend (line %d)\n", line_num); */
                is_err = 1;
            }
            /* printf("success macroend (line %d)\n", line_num);*/
        }
        else if (in_macro_def)
        {
            addToReallocedBody(line, strlen(line)); /*we dynamically enlarge the global body-var to make room for the current line*/
        }
        else{ /*a regular assembly line, or a line that refers to an already defined macro*/
            handleContentLine(line, curr_pos_ptr, output_f_ptr);
        }
    }
    /* printf("after main while; is_err: %d\n", is_err); */

    /*now, whether it is error or not, close files if needed and abandon realloced body if somewhat still has realloced memory*/
    if (input_f_ptr != NULL)
        fclose(input_f_ptr);
    if (output_f_ptr != NULL)
        fclose(output_f_ptr);
    if(glob_is_body_realloc_live)
        abandonBody_reset_vars();
    
    MacroList_freeMacroBodies(this_mcro_list); /*this frees the all of the "realloced bodies". The rest will be freed when using flushGarbageCollector(0) */

    return is_err;
}
