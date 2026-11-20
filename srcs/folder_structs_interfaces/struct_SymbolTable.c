#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../hdrs/util_funcs.h"
#include "../../hdrs/global_entities.h"
#include "../../hdrs/mem_management.h"
#include "../../hdrs/print_funcs.h"

#include "../../hdrs/struct_MacroRow.h"
#include "../../hdrs/struct_SymbolTable.h"


/*The symbols-table is a struct that is a node that consists of a linked-list, that is meant to handle and store data about symbols*/
/*Its main role is to be hold defitions of symbols filled on the first pass,
and then dispense those to "fetch" requests from the second pass
It also has some legality functions that manage the table-fill on the first pass,
and tests for usage / compatibility between symbols
*/

EntrySymbol *appendEntrySymbol(char *name_malloced, char *base_filename_malloced, EntrySymbol *head)
{
    EntrySymbol *new_node;
    new_node = (EntrySymbol *)ourMalloc(sizeof(EntrySymbol), TEMP_MEM);
    if (new_node == NULL)
        return NULL;
    
    new_node->name = name_malloced;
    new_node->base_filename = base_filename_malloced;
    new_node->next = head;
    return new_node;
}

ExternSymbol *appendExternSymbol(char *name_malloced, char *base_filename_malloced, ExternSymbol *head)
{
    ExternSymbol *new_node;
    new_node = (ExternSymbol *)ourMalloc(sizeof(ExternSymbol), TEMP_MEM);
    if (new_node == NULL)
        return NULL;
    
    new_node->name = name_malloced;
    new_node->base_filename = base_filename_malloced;
    new_node->next = head;
    return new_node;
}

Symbol *FindSymbol(FileNode * this_f_node, const char *name)
{

    Symbol *current = this_f_node->symbolTable;
    while (current != NULL)
    {
        if (strcmp(current->string_name, name) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

void initSymbolTable(FileNode * this_f_node)
{
    Symbol *current = this_f_node->symbolTable;
    Symbol *next;

    while (current != NULL)
    {
        next = current->next;
        if (current->where_used != NULL)
        {
            struct Usage *usage_current = current->where_used;
            struct Usage *usage_next;
            while (usage_current != NULL)
            {
                usage_next = usage_current->next;
                usage_current = usage_next;
            }
        }
        current = next;
    }
    this_f_node->symbolTable = NULL;
}

int addSymbol(FileNode * this_f_node, const char *name, int value, enum OriginType origin, int line_number)
{
    Symbol *new_symbol;

    Symbol *existing_symbol = FindSymbol(this_f_node, name);
    if (existing_symbol != NULL) /*only case where adding an existing symbol is legal is when it was declared as an entry, and now it is actually defined, or vice versa*/
    {
        if (existing_symbol->int_decimal_value == 0 && existing_symbol->origin == ORIGIN_ENTRY && origin != ORIGIN_EXTERN && origin != ORIGIN_ENTRY)
        {
            existing_symbol->int_decimal_value = value;
            this_f_node->_is_ent_seen = 1; /*indicating that a declaration of an entry has been seen - has to create the ent file*/
            return SYMBOL_SUCCESS;
        }
        if (existing_symbol->origin == ORIGIN_INTERNAL && origin == ORIGIN_ENTRY)
        {
            existing_symbol->origin = origin;
            this_f_node->_is_ent_seen = 1; 
            return SYMBOL_SUCCESS;
        }
        printErrWarn_from_3way_str("Symbol ", name, " is defined a second time", 0, line_number);
        return SYMBOL_ERROR;
    }

    if (FindMacro(this_f_node->_macro_list, name) != NULL)
    {
        printErrWarn_from_3way_str("Cannot add symbol ", name, ". There was a macro with the same name", 0, line_number);
        return SYMBOL_ERROR;
    }
    if (isReservedword((char*) name))
    {
        printErrWarn_from_3way_str("Cannot add symbol ", name, ". is a reserved word", 0, line_number);
        return SYMBOL_ERROR;
    }

    new_symbol = (Symbol *)ourMalloc(sizeof(Symbol), LONG_MEM);
    if (new_symbol == NULL)
    {
        printErrWarn_from_str("Memory allocation failed for Symbol node", 0, line_number);
        return SYMBOL_ERROR;
    }

    new_symbol->string_name = (char *)ourMalloc(strlen(name) + 1, LONG_MEM);
    if (new_symbol->string_name == NULL)
    {
        printErrWarn_from_str("Memory allocation failed for Symbol name", 0, line_number);
        return SYMBOL_ERROR;
    }

    strcpy(new_symbol->string_name, name);
    new_symbol->int_decimal_value = value;
    new_symbol->origin = origin;
    new_symbol->declered_at_line = line_number;
    new_symbol->where_used = NULL;
    new_symbol->next = this_f_node->symbolTable;
    this_f_node->symbolTable = new_symbol;
    return SYMBOL_SUCCESS;
}

void addUsage(struct Usage **head, int line_number)
{
    struct Usage *new_usage = (struct Usage *)ourMalloc(sizeof(struct Usage), LONG_MEM);
    if (new_usage == NULL)
    {
        printErrWarn_from_str("Memory allocation failed for Usage node", 0, -1);
        return;
    }
    new_usage->line_number = line_number;
    new_usage->next = NULL;

    if (*head == NULL)
    {
        *head = new_usage;
    }
    else
    {
        struct Usage *current = *head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_usage;
    }
}

Symbol *fetchByName(FileNode * this_f_node, const char *name, int line_number) /*"line_number" is a misleading name - this func gets the MACHINE-CODE-ADRESS of the word that referred to a label*/
{
    Symbol *symbol;

    /* printf("trying to fetch syb: %s\n", name);*/
    symbol = FindSymbol(this_f_node, name);
    /* printf("after find,  symbol==null: %d\n", symbol == NULL);*/

    if(symbol != NULL && symbol->origin == ORIGIN_EXTERN){
        this_f_node->_is_ext_usage_seen = 1; /*indicating that a usage of ext has been seen - has to create the ext file*/
    }

    if (symbol != NULL && line_number >= 0)
    {
        addUsage(&(symbol->where_used), line_number);
    }
    return symbol;
}

int getAddressByLabelName(FileNode * this_f_node, const char *label_str)
{
    return (fetchByName(this_f_node, label_str, -1))->int_decimal_value;
}

void freeSymbolTable(FileNode * this_f_node)
{
    initSymbolTable(this_f_node);
}

void logSymbolTable(FileNode * this_f_node, const char *filename)
{
    Symbol *current;
    FILE *log = fopen(filename, "w");
    if (log == NULL)
    {
        printf("Failed to open log file: %s\n", filename);
        return;
    }

    current = this_f_node->symbolTable;
    while (current != NULL)
    {
        fprintf(log, "Label: %s, Value: %d, Origin: %d", current->string_name, current->int_decimal_value, current->origin);
        if (current->where_used != NULL)
        {
            struct Usage *usage = current->where_used;
            fprintf(log, ", Used at lines: ");
            while (usage != NULL)
            {
                fprintf(log, "%d", usage->line_number);
                if (usage->next != NULL)
                {
                    fprintf(log, ", ");
                }
                usage = usage->next;
            }
        }
        fprintf(log, "\n");
        current = current->next;
    }

    fclose(log);
}

/*validate that we havn't got any unexpected error, or the user declared an entry without declaring its actual label elsewhere*/
enum SymbolStatus validateSymbolTable(FileNode * this_f_node)
{
    Symbol *current = this_f_node->symbolTable;
    enum SymbolStatus res = SYMBOL_SUCCESS;
    while (current != NULL)
    {
        if (current->origin == -1)
        {
            printErrWarn_from_3way_str("Symbol ", current->string_name, " has an invalid origin", 0, current->declered_at_line);
            res = SYMBOL_ERROR;
        }
        if (current->string_name == NULL)
        {
            printErrWarn_from_str("Symbol has a NULL name", 0, current->declered_at_line);
            res = SYMBOL_ERROR;
        }
        if ((current->origin != ORIGIN_EXTERN && current->int_decimal_value == 0) ||
            current->declered_at_line <= 0)
        {
            printErrWarn_from_3way_str("Symbol ", current->string_name, " is never defined", 0, -1);
            res = SYMBOL_ERROR;
        }
        current = current->next;
    }
    return res;
}

/*validates that every declared label has at least one usage within the current file - warning only*/
int validateIsUsed(FileNode * this_f_node)
{
    Symbol *current;
    int is_warn;

    current = this_f_node->symbolTable;
    is_warn = 0;
    while (current != NULL)
    {
        if (current->where_used == NULL && current->origin != ORIGIN_ENTRY)
        {
            printErrWarn_from_3way_str("Label ", current->string_name, " is defined but never used", 1, current->declered_at_line);
            is_warn = 1;
        }
        current = current->next;
    }
    return is_warn;
}


/*ent/ext files generation START*/
int printSymbolToFile(FILE* f_ptr, char* sym_str, int machine_addrss){
    char * base4_str;
    base4_str = intToBase4str_compact(machine_addrss);

    if(ftell(f_ptr) != SEEK_SET)
        fprintf(f_ptr, "\n");
    
    fprintf(f_ptr, "%s\t%s", sym_str, base4_str);
    return base4_str == NULL;
}

FILE * createEntExtFile(int is_ent, const char *base_filename){
    FILE *new_f_ptr;
    char * curr_f_name_w_extention;
    int base_name_len, suffix_len, total_len;

    base_name_len = strlen(base_filename);
    suffix_len = strlen(".ent");
    total_len = base_name_len + suffix_len;

    curr_f_name_w_extention = (char *)ourMalloc(total_len + 1, TEMP_MEM);

    if(curr_f_name_w_extention == NULL){
        printErrWarn_from_str("Memory allocation failed for filenames", 0, -1);
        return NULL;
    }

    nullifyCharArr(curr_f_name_w_extention, total_len + 1);
    strncat(curr_f_name_w_extention, base_filename, base_name_len);
    strncat(curr_f_name_w_extention, is_ent ? ".ent" : ".ext", suffix_len);

    new_f_ptr = fopen(curr_f_name_w_extention, "w");
    if(new_f_ptr == NULL)
        printErrWarn_from_3way_str("Could not open file ", curr_f_name_w_extention, " for writing", 0, -1);
    return new_f_ptr;
}


void createSymbolOutputFiles(FileNode * this_f_node, int* err_flag_byRef)
{
    Symbol *curr_sym;
    FILE *ent_f_ptr, *ext_f_ptr, **curr_f_ptr_ptr;
    struct Usage *current_usage;
    int curr_file_create_required, i;

    if(this_f_node->_is_ent_seen == 0 && this_f_node->_is_ext_usage_seen == 0){ /*if both files aren't required*/
        *err_flag_byRef = 0;
        return;
    }

    curr_sym = this_f_node->symbolTable;
    ent_f_ptr = ext_f_ptr = NULL;

    for(i = 0; i < 2; i++){ /*now creating files, if required (ent and then ext)*/
        curr_f_ptr_ptr = i == 0 ? &ent_f_ptr : &ext_f_ptr;
        curr_file_create_required = i == 0 ? this_f_node->_is_ent_seen : this_f_node->_is_ext_usage_seen;

        if(curr_file_create_required){
            *curr_f_ptr_ptr = createEntExtFile(i == 0, this_f_node->file_name);
            if(*curr_f_ptr_ptr == NULL){ /*something went wrong while creating a file*/
                *err_flag_byRef = 1;
                return;
            }
            /*printf("successfully genned %s\n", i == 0 ? "ent" : "ext");*/
        }
    }


    while (curr_sym != NULL) /*now iterate over all symbols, if we find and .entry declare or an .extern usage: print to relevant file*/
    {
        if (curr_sym->origin == ORIGIN_EXTERN)
        {
            current_usage = curr_sym->where_used;
            while (current_usage != NULL)
            {
                /* printf("iter usage\n"); */
                if(printSymbolToFile(ext_f_ptr, curr_sym->string_name, current_usage->line_number) == 1){ /*this is not the line number - this is the machine-address of the reference to this extern label)*/
                    *err_flag_byRef = 1;
                    return;
                }
                current_usage = current_usage->next;
            }
        }
        else if (curr_sym->origin == ORIGIN_ENTRY)
        {
            if(printSymbolToFile(ent_f_ptr, curr_sym->string_name, curr_sym->int_decimal_value) == 1){ /*printSymbolToFile prints and returns 1 if error occured*/
                *err_flag_byRef = 1;
                return;
            }
            /* printf("successfully put 1 ent\n"); */
        }
        curr_sym = curr_sym->next;
    }

    if(ent_f_ptr != NULL)
        fclose(ent_f_ptr);
    if(ext_f_ptr != NULL)
        fclose(ext_f_ptr);

    /* printf("finished exntext file gen\n"); */
    
    *err_flag_byRef = 0;
}
/*ent/ext files generation END*/




/*symbol table fill for first pass START*/

/*func was originally a part of first-pass-invoker file*/
int fillSymbolTable(FileNode *current_file_node, AsmLinesList *parsed_lines_list)
{
    int err_curr_flag, err_any_flag;
    AsmLineNode *curr_node;
    ParsedLine *curr_parsed_line;
    enum LineType curr_line_type;
    enum OriginType curr_origin_type;
    int is_line_errored, condit_1, condit_2;
    /* int add_result_1, add_result_2; */
    /* char err_or_warn_buff[MAX_ERR_MSG_LEN]; */
    /* char *curr_warn_ptr, *curr_err_ptr; */


    if (current_file_node == NULL || parsed_lines_list == NULL || parsed_lines_list->_head == NULL)
    {
        printf("Failure: requested symbol-table fill for an empty lines-list/file-node\n");
        return 1;
    }

    initSymbolTable(current_file_node);

    err_any_flag = 0;
    curr_node = parsed_lines_list->_head;

    while (curr_node != NULL)
    {
        err_curr_flag = 0;
        curr_parsed_line = curr_node->_parsed_line;
        curr_line_type = curr_parsed_line->_lineType;

        is_line_errored = curr_line_type == LN_ERR;
        condit_1 = condit_2 = 0;
        if(!is_line_errored){
            condit_1 = curr_parsed_line->_is_line_labeled;
            condit_2 = (curr_line_type == LN_DRCTV) && (curr_parsed_line->_cmdCode == (int)DRCTV_EXTERN || curr_parsed_line->_cmdCode == (int)DRCTV_ENTRY);
        }

        /* first, test for line-level-declaration*/
        if (condit_1)
        {
            /* addSymbol should return 0 if ok, 1 if error, 2 if warning*/
            err_curr_flag = addSymbol(current_file_node,
                                        curr_parsed_line->_line_label,
                                        curr_parsed_line->_relative_addrss,
                                        ORIGIN_INTERNAL,
                                        curr_parsed_line->_src_asm_line_num
            ); /* should also receive curr_parsed_line->_errMsg for filling an error and also curr_parsed_line->_warnMsg for filling a warning 	important! add should only strncat err msgs to the buff, not strncopy, to allow multiple errors / warnings*/
        }

        /* second, test for ent/ext directives*/
        if (condit_2)
        {
            curr_origin_type = (curr_parsed_line->_cmdCode == (int)DRCTV_EXTERN) ? ORIGIN_EXTERN : ORIGIN_ENTRY;
            err_curr_flag = addSymbol(current_file_node,
                                        (curr_parsed_line->_1stOprndVal)._str,
                                        0, /* machine-address - unknown (entry / extern here)*/
                                        curr_origin_type,
                                        curr_parsed_line->_src_asm_line_num
                                        ) ||
                            err_curr_flag; /* avoiding short circuiting or - we gotta try add this symbol at any rate*/
        }

        err_any_flag = err_any_flag || err_curr_flag;
        curr_node = curr_node->_next_node;
    } /*end of while*/

    return err_any_flag;
}

/*symbol table fill for first pass END*/