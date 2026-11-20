Hello,

This is the final project of Yosef Vaksin and Matan Mayo.

Our program is called “assembler” and called like: “ ./assembler file1 <file2> ...”, while the files inserted as arguments are without the .as extension.

The project is organized as a folders-system, while the makefile is directly within the mother-folder, which will also contain the executable file. The .o files for the executable are put within the “build” folder.

Any test files should be placed within the mother-folder before run, and any output files are put into this same folder.

The project’s background, purpose and definitions are described in maman 14.

At the top of each c file, we put an explanation about the file’s purpose and its main contribution to the assembly operation, but here are the general ideas of our project:

The work is separated into several stages, some take place once per an entire run, some are repeated per file-argument.

In the beginning we interpret the command line arguments and insert them into FileList that consists of FileNodes (struct_FileList.c)

1. For each FileNode, we first invoke the “preprocessor” that manages macros expansion. At this stage, If any error is encountered, the run is terminated immediately (preprocess.c, struct_MacroRow.c)

2. For each FileNode, we send the macro-expanded .am file to “Parsing” i.e we iterate over each line and extract its contents into a dedicated struct: “ParsedLine” that is able to hold all the relevant info from each line (e.g. command code, is the line labelled, type of the operands, value of the operands etc.) in a structured form. Like this, after parsing and dealing with the bulk of the textual-analysis stage, we can comfortably iterate over these parsedLines and start their translation into machine code. This is the most massive part of the operation. (expanded_to_parsed.c, parsing_support.c, struct_AsmLinesList.c)

3. For each FileNode we perform the initial translation into machine code. This includes logical operations on the ParsedLines, in order to figure out the value for each bit. Meanwhile we also fill the symbol’s table, which manages and alerts if something is logically wrong with the symbols. (parsed_to_machine.c, struct_MachineWordsList.c, struct_SymbolTable.c)

4. For each FileNode we perform the second pass: iterating over machine words that refer to labels, and asking for them from the symbol-table. After this stage we also warn about unused labels. (invoker_funcs.c, struct_SymbolTable.c)

5. For each FileNode we generate output files, if relevant (invoker_funcs.c)

After successful(!) completion of these operation per each FileNode, we activate our “Prelinker” – its role is to test compatibility between .ent/.ext definitions across all the inputted files. This stage only generates warnings and will not cause the assembly operation to be considered errored.

In a general perspective,

We manage all of our structs-typedefs, enumerated types, and most of the constants in the header “global_entities.h”, simplifying the dependencies for the entire project.

Most .c files have a matching header file that expose only the functions that are actually needed by external files / the main function. They are the ones included by external files (.c files will not be referred to, generally).

Additionally, we have implemented a special mechanism to manage dynamic memory more automatically: we do not track each allocation manually, but track them in a dynamic “garbage-collector” list that is being “flushed” once in a while. (mem_management.c)

In addition, we have “util_funcs.c” that provide small, miscellaneous functions that are used throughout the project, “print_funcs.c” that aggregate most of the interactions with the standard output, and “invoker_funcs.c” that consist of methods that are called from main, and utilize the entire project.

special note:
-------------

We have received a permission to hand over the project a few weeks later. We’ve attached the relevant email in a separate file.

Untrivial Assumptions:
----------------------

We have taken certain assumptions for several lacking/ambiguous definitions in the maman.
Most of them are indicated in comments as “ASSUMPTION”s, and are summarized here:

1. In label declarations, we do not require space between the colon and the expressions to its right
2. Declaration-of and referance-to the same label on the same line is illegal
3. We do not allow lines of the original .as file to exceed 80 chars
4. We allow strings like "r8" (register-like but serial is not an actual reg), "string", "data" (name of a directive without the dot) to be legal names for labels.
5. We consider the tokens after directives as operands when describing errors, for example: ".mat [2][2] 1, 2, 3" -> "[2][2]" is operand 1, "1, 2, 3" is operand 2.
6. We create the .ext file only if there are machine words that refer to the relevant symbol - this is different from creating the file for every .extern directive.
7. If the same .as filename was passed several times as an argument, we simply ignore duplicates
8. After macros expansion, mcro/mcroend are no longer reserved words
9. On label declarations, we do not require space between the colon and the expressions to its right
10. In regards to square-brackets expressions: wrapping spaces within the square-brackets are allowed; spaces between the label and the first opening bracket are NOT allowed; spaces between the middle brackets are NOT allowed
11. Int-sequences - we allow free spaces throughout the int-sequence
12. String consts - inner spaces/tabs are ALLOWED inside a string const
13. Comments - ';' must appear as the first non-whitespace character (preceeding spaces/tabs are allowed!) of the line to be considered a comment; a comment cannot start after a line with actual content
14. All numerical values in the input, including dimensions of a matrix (cols/rows) must be containable inside a *single signed byte* i.e -128 up to 127
15. We allow spaces before and after 'mcro' as well as 'mcroend'
