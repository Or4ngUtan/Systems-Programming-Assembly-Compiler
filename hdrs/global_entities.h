#ifndef GLOBAL_ENTITIES_H
#define GLOBAL_ENTITIES_H

/*
This header contains most of the definitions for entities that are used throughout the entire project:
struct typedefs, enums, common constants.

Each struct has its own functions-interface specified in its own file.
*/



/***********************************************************************************/





/*constants START*/

/*note - not all constants are included here - those that we could contain locally were left private to the specific .c/.h files*/

#define NUM_INSTRCT_TYPES 16
#define NUM_DRCTV_TYPES 5
#define NUM_TYPES_OPERANDS 7 /* 0-instant, 1-direct, 2-matCell, 3-reg, 4-intSeq, 5-strConst, 6-matDims*/
#define REG_SERIAL_NUM_MIN 0 /* registers range from r0 to r7*/
#define REG_SERIAL_NUM_MAX 7
#define MACHINE_WORD_ARR_SIZE 11

#define VISIBLE_ASCII_MIN 32 /* "string constants may contain only visible ascii chars"*/
#define VISIBLE_ASCII_MAX 126

#define MAX_FILENAME_LEN 256
#define MAX_ASM_LINE_LEN 80 /* ASSUMPTION - refers to asm lines after BEFORE expansion - we do not allow lines of the .as file to exceed 80 chars! */
#define MAX_LABEL_LEN 30
#define MAX_ERR_MSG_LEN 400 /* used to restrict strings that contain error messages */

#define MATRIX_PADDING_VALUE 0 /* Default value for padding matrix cells*/
#define STRING_TERMINATOR_VALUE 0 /* Null terminator for strings*/

/*constants END*/






/***********************************************************************************/





/*enums START*/

enum LineType
{
    LN_INIT = -2,   /* line is uninitiallized*/
    LN_ERR = -1,    /* line is errored*/
    LN_INSTRCT = 0, /* instruction*/
    LN_DRCTV = 1    /* directive*/
};

enum OccupyType
{
    OCCUP_NONE = -1, /* a line that does not occupy any machine-words in the memory-image*/
    OCCUP_IC = 0,    /* a line that occupies IC words*/
    OCCUP_DC = 1     /* a line that occupies DC words*/
};

enum OprndType
{
    /* 0-instant, 1-direct, 2-matCell, 3-reg, 4-intSeq, 5-strConst, 6-matDims*/
    OPRND_NONE = -1, /* useful? yes, while iterating over all possible types and still unmatched type*/

    OPRND_INSTANT = 0,  /* e.g #-1*/
    OPRND_DIRECT = 1,   /* e.g x (x is a label)*/
    OPRND_MAT_CELL = 2, /* i.e reference to a cell, e.g myMat[r0][r1]*/
    OPRND_REG = 3,      /* e.g r0*/

    OPRND_INT_SEQ = 4,   /* i.e integer-sequence, e.g 1,2,-3,   4,+5*/
    OPRND_STR_CONST = 5, /* i.e string-constant, e.g "blablaXYZ ..bla" //note - inner spaces/tabs are allowd inside a string const*/
    OPRND_MAT_DIMS = 6   /* i.e matrix-dimensions, e.g [1][5]*/
};

enum DrctvName
{
    DRCTV_DATA = 0,
    DRCTV_MAT = 1,
    DRCTV_EXTERN = 2,
    DRCTV_STRING = 3,
    DRCTV_ENTRY = 4
};

enum SymbolStatus /*used for various indications regarding symbols-legallity*/
{
    SYMBOL_SUCCESS,
    SYMBOL_ERROR,
    SYMBOL_WARNING
};

enum OriginType /*indicates the origin of a label definition (internal means regular label)*/
{
    ORIGIN_INTERNAL,
    ORIGIN_EXTERN,
    ORIGIN_ENTRY
};

enum AREbitsType{
    ARE_ABS = 0,
    ARE_RE = 2, /*reallocatable ARE bits should look like 10 !!!*/
    ARE_EXT = 1 /*extern ARE bits should look like 01 !!!*/
};

/*enums END*/





/***********************************************************************************/





/*global vars START*/

extern char *glob_instrctNamesArr[NUM_INSTRCT_TYPES];
extern char *glob_drctvNamesArr[NUM_DRCTV_TYPES];
extern char *glob_INSTRCTS_legalOprndTypes[NUM_INSTRCT_TYPES][2];
extern char *glob_DRCTVS_legalOprndTypes[NUM_INSTRCT_TYPES][2];

/*global vars END*/





/***********************************************************************************/





/*IntsList struct START*/

typedef struct IntNode
{
    int _num;
    struct IntNode *_next_node;
} IntNode;

typedef struct
{
    IntNode *_head;
    IntNode *_tail;
} IntsList;

/*IntsList struct END*/





/***********************************************************************************/





/*MachineWordsList struct START*/

typedef struct
{
    int _is_label_ref;
    char *_lbl_ref_name;
    int _relative_addrss;
    /* char _ARE; //relevant only for words that are a ref to a label, and can be R or E (external); otherwise it is immediate - A*/

    char _bits_str[MACHINE_WORD_ARR_SIZE];
} MachineWord;

typedef struct MachineWordNode
{
    MachineWord _mchn_word; /* an actual struct! not a pointer - no need to allocate separately*/
    struct MachineWordNode *_next_node;
} MachineWordNode;

typedef struct
{
    MachineWordNode *_head;
    MachineWordNode *_tail;
} MachineWordsList;

/*MachineWordsList struct END*/





/***********************************************************************************/





/*AsmLinesList struct START*/

typedef struct
{ /* if mat-cell, e.g mat[r0][r1], then all 3 fields will be occupied; if mat-dim (declaration of matrix with .mat), then only the 2 integers will be occupied*/
    int _internal_row;
    int _internal_col;
    char *_internal_str;
} MatCell;

typedef union
{
    int _int;             /* for instant, reg*/
    char *_str;           /* for label, string-const*/
    MatCell *_matCell;    /* for mat-cell, mat-dims*/
    IntsList *_ints_list; /* for int-sequences*/
} OprndVal;

typedef struct
{
    int _src_asm_line_num; /* after macro expansion; will be used to refer to errors*/

    enum LineType _lineType;
    char _errMsg[MAX_ERR_MSG_LEN];

    int _is_warn_msg;
    char _warnMsg[MAX_ERR_MSG_LEN];

    int _is_line_labeled;
    char *_line_label;

    int _cmdCode;
    enum OprndType _1stOprndType, _2ndOprndType;
    OprndVal _1stOprndVal, _2ndOprndVal; /* important! a value, not a pointer (otherwise it is not allocated automatically with the creation of a new ParsedLine)*/

    /*not filled during initial parsing*/
    MachineWordsList *_mchn_words_lst;
    enum OccupyType _occupy_type;
    int _relative_addrss; /* address (by terms of IC/DC) of the first machine-word of this line*/
    int _count_mchn_words;
} ParsedLine;

/* this is a structure that will consist of nodes of asmLines*/
typedef struct AsmLineNode
{
    ParsedLine *_parsed_line;
    struct AsmLineNode *_next_node;
} AsmLineNode;

/* this is a structure that will aggregate all the "asmLineNode"s*/
typedef struct
{
    AsmLineNode *_head;
    AsmLineNode *_tail;
    int IC_section_len;
    int DC_section_len;
} AsmLinesList;

/*AsmLinesList struct END*/





/***********************************************************************************/





/*SymbolList struct START*/

typedef struct Usage
{
    int line_number;
    struct Usage *next;
} Usage;

typedef struct Symbol
{
    char *string_name;
    int int_decimal_value;
    enum OriginType origin;
    int declered_at_line;
    Usage *where_used;
    struct Symbol *next;
} Symbol;

/*SymbolList struct END*/





/***********************************************************************************/




/*MacroRow struct START*/

typedef struct MacroRow
{
    char *name;
    char *body;
    struct MacroRow *next;
} MacroRow;

typedef struct{
    MacroRow* _head;
    MacroRow* _tail;
} MacroList;

/*MacroRow struct END*/




/***********************************************************************************/






/*FilesList struct START*/

typedef struct FileNode
{
    char *file_name;
    Symbol *symbolTable;

    int _is_ent_seen, _is_ext_usage_seen; /*indicates the need for creation of ent/ext files*/
    struct FileNode *next_node;
    MacroList* _macro_list;
} FileNode;

typedef struct FileList
{
    FileNode *head;
    FileNode *tail;
} FileList;

typedef struct EntrySymbol
{
    char *name;
    int address;
    char *base_filename;
    struct EntrySymbol *next;
} EntrySymbol;

typedef struct ExternSymbol
{
    char *name;
    int address;
    char *base_filename;
    struct ExternSymbol *next;
} ExternSymbol;

/*FilesList struct END*/





/***********************************************************************************/




#endif