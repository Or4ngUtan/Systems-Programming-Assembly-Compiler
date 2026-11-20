#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>

#include "../../hdrs/mem_management.h"
#include "../../hdrs/global_entities.h"
#include "../../hdrs/util_funcs.h"

#include "../../hdrs/struct_SymbolTable.h"



/*
This file aggregates various functions that have cross-project usage,
and are not meant for a specific assembly-operation.
*/


int isTabOrSpace(char c)
{
    return c == ' ' || c == '\t';
}

void nullifyCharArr(char *char_arr_byRef, int char_arr_len)
{
    int i;
    for (i = 0; i < char_arr_len; i++)
    {
        char_arr_byRef[i] = '\0';
    }
}

int isReservedword(char *w)
{
    int i, j, curr_cmds_arr_len;
    char **curr_cmds_arr;
    curr_cmds_arr = glob_instrctNamesArr;
    curr_cmds_arr_len = NUM_INSTRCT_TYPES;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < curr_cmds_arr_len; j++)
        {
            if (strcmp(w, curr_cmds_arr[j]) == 0)
                return 1;
        }

        if (i == 0)
        {
            curr_cmds_arr = glob_drctvNamesArr;
            curr_cmds_arr_len = NUM_DRCTV_TYPES;
        }
    }

    if (strlen(w) == 2 &&
        w[0] == 'r' &&
        isdigit(w[1]))
    {
        int dig;
        dig = digitCharToInt(w[1]);
        if (dig >= REG_SERIAL_NUM_MIN && dig <= REG_SERIAL_NUM_MAX)
            return 1;
    }

    /*ASSUMPTION - after macros expansion, mcro/mcroend are no longerreserved words*/

    return 0;
}

int isReservedword_asSubstr(char *host_str, int from_pos, int to_pos)
{
    char *substr;
    substr = generateSubstr(host_str, from_pos, to_pos);
    return isReservedword(substr);
}

int findNextChar(char *str, char c, int from_pos)
{
    char curr_c;
    do
    {
        curr_c = str[from_pos];
        from_pos++;
    } while (curr_c != c && curr_c != '\0');

    if (curr_c == '\0')
        return -1;
    return from_pos - 1;
}

int findNextChar_multi(char *host_str, char *terminators, int from_pos)
{
    /* char str_curr_c, term_curr_c; */
    int str_curr_i, term_curr_i;
    int host_str_len, terminators_len;
    int flag_found;


    str_curr_i = from_pos;
    host_str_len = strlen(host_str);
    terminators_len = strlen(terminators);
    if (from_pos >= host_str_len || terminators_len == 0)
        return -1;

    flag_found = 0;
    for (str_curr_i = from_pos; str_curr_i < host_str_len && flag_found == 0; str_curr_i++)
    {
        for (term_curr_i = 0; term_curr_i < terminators_len && flag_found == 0; term_curr_i++)
        {
            if (host_str[str_curr_i] == terminators[term_curr_i])
                flag_found = 1;
        }
    }

    if (flag_found)
        return str_curr_i - 1;
    return -1;
}

int digitCharToInt(char ch)
{
    return ch - '0';
}

int isNumInRange8bits(int num)
{
    static int min_val = 0;
    static int max_val;

    if (min_val == 0)
    {
        min_val = (-1) * pow(2, 8 - 1); /* min value is 1000_0000 <=> (2's cmplmnt.): -(2^(8-1))*/
        max_val = pow(2, 8 - 1) - 1;    /* max value for a signed int is 0111_1111 <=> 2^(8-1) - 1*/
    }

    return num <= max_val && num >= min_val;
}

char *generateSubstr(char *host_str, int from_pos, int to_pos)
{
    char *output_str;
    int num_real_chars;


    if (from_pos > to_pos)
        return NULL;

    num_real_chars = to_pos - from_pos + 1;

    output_str = ourMalloc(num_real_chars + 1, TEMP_MEM); /*+1 for terminating \0*/

    strncpy(output_str, host_str + from_pos, num_real_chars);
    output_str[num_real_chars] = '\0'; /*super important!!!!!!! dunno how it worked before*/

    /*printf("generating substr: %s\n", output_str);*/

    return output_str;
}

/*position manipulators for string analyses*/

int findNextWordTerminator(char *host_str, int from_ind)
{
    static char possib_word_terminators[] = {':', ',', ' ', '\t', '\"'}; /* inclusion of \" is important: ootherwise we might think that a space within a string const is an end of a token*/
    int next_termin;
    next_termin = findNextChar_multi(host_str, possib_word_terminators, from_ind);
    return next_termin == -1 ? strlen(host_str) : next_termin; /* in case no word terminator was found, return the \0 as the next position of a terminator*/
}

int skipTabsSpaces(char *str, int curr_pos)
{
    char curr_c;
    do
    {
        curr_c = str[curr_pos];
        curr_pos++;
    } while (isTabOrSpace(curr_c));
    return curr_pos - 1;
}

int skipTabsSpaces_reverse(char *str, int right_hand_pos)
{ /* returns the position of the 1st non blank char to the left of the given position, -1 if none exists*/
    char curr_c;
    do
    {
        if (right_hand_pos <= -1)
            return -1;
        curr_c = str[right_hand_pos];
        right_hand_pos--;
    } while (isTabOrSpace(curr_c));
    return right_hand_pos + 1;
}

int skipToLastNonBlankInLine(char *str)
{ /* returns the position of the last character that is not space/tab (can be \,,:) 	will be used for parsing sequences of int consts*/
    int str_len;
    str_len = strlen(str);
    if (str_len == 0)
        return -1;

    return skipTabsSpaces_reverse(str, str_len - 1);
}

/*extension without the dot*/
char * appendExtension_to_f_name(const char *f_name_plain, const char *extens_str)
{
    int final_len;
    char *ret_str;

    final_len = strlen(f_name_plain) + strlen(extens_str);
    ret_str = ourMalloc(final_len + 2, TEMP_MEM); /*+1 for the dot, +1 for terminating '\0'*/
    nullifyCharArr(ret_str, final_len + 2);
    strncpy(ret_str, f_name_plain, strlen(f_name_plain));
    strncat(ret_str, ".", 1);
    strncat(ret_str, extens_str, strlen(extens_str));
    return ret_str;
}


/*base conversion functions START*/

int getBase4digit_by_2base2digits_as_int(char *host_str, int left_dig_pos)
{
    char dig_l, dig_r;
    dig_l = host_str[left_dig_pos];
    dig_r = host_str[left_dig_pos + 1];
    if (!(dig_l == '0' || dig_l == '1') || !(dig_r == '0' || dig_r == '1'))
        return -1;

    return 2 * (dig_l == '1') + (dig_r == '1');
}

char *machineWordToBase4str(char *mchn_word)
{
    int new_num_digits, i;
    char *malloced_str;


    new_num_digits = (MACHINE_WORD_ARR_SIZE - 1) / 2; /* 5 in our case, -1 because MACHINE_WORD_ARR_SIZE includes the terminator '\0'*/
    malloced_str = ourMalloc(new_num_digits + 1, TEMP_MEM);  /*+1 for terminator '\0'*/
    nullifyCharArr(malloced_str, new_num_digits + 1); /* the 6'th char is not guaranteed to be '\0'. this is a spaecial case*/

    for (i = 0; i < new_num_digits; i++){
        malloced_str[i] = 'a' + getBase4digit_by_2base2digits_as_int(mchn_word, 2 * i);
    }
    return malloced_str;
}

char *intToBase4str_compact(int num)
{ /* compact version, e.g: num=3 => "d"; num=0 => "a"; num=6 => "bc"*/
    /* will be used only for addresses, and the ic/dc sections-len row*/
    char temp_arr[MACHINE_WORD_ARR_SIZE - 1] = {'\0'};
    char *temp_ptr;
    int i, full_len;


    populateBitsWithNum_byDecimal(temp_arr, 0, MACHINE_WORD_ARR_SIZE - 2, num);
    temp_ptr = machineWordToBase4str(temp_arr);
    /* now, tmp_ptr includes something like "aaacb". we need to make it compact: "cb"*/

    /* the next loop updates temp_ptr so that it points to the first non-'a' char of  temp_ptr a it is now, or to the last 'a', if all chars are 'a'. like this we get a compact form*/
    for (i = 0, full_len = strlen(temp_ptr); i < full_len; i++)
    {
        if (*temp_ptr != 'a')
            break;
        if (i == full_len - 1) /* if there are only 'a's in this string, we should return a string that is "a"*/
            break;
        temp_ptr++; /* we increment temp_ptr <=> trimming 'a's from left by 1 at a time*/
    }
    return temp_ptr; /* it is ok to return temp_ptr even though it was declared locally - it was assigned a malloced pointer from machineWordToBase4str*/
}


/* from, to - including*/
void populateBitsWithNum_byDecimal(char *bits_str, int from_pos, int to_pos, int decimal_val)
{
    int is_negative;
    int curr_remainder, curr_pos_in_str;
    char curr_char;

    
    if (decimal_val == 0)
    {
        do
        {
            bits_str[from_pos++] = '0';
        } while (from_pos <= to_pos);
        return;
    }

    /* we work according to the general algorithm to convert decimal to 2's complmnt signed binary:*/
    /* 1. find the positive version in binary*/
    /* 2. pad with 0's from left to complete the whole length*/
    /* 3. if negative: flip all bits to the left of the rightmost-1-bit (excluding)*/

    /* decimal_val is promised to be accommodable in a 8-bit-string*/

    is_negative = decimal_val < 0;
    decimal_val = abs(decimal_val);
    curr_pos_in_str = to_pos; /* least significant digit will populate "to_pos"*/
    
    do
    {
        if (decimal_val == 0)
        {
            curr_char = '0';
        }
        else
        {
            curr_remainder = decimal_val % 2;
            curr_char = curr_remainder == 0 ? '0' : '1';
            decimal_val /= 2;
        }
        bits_str[curr_pos_in_str--] = curr_char;
    } while (curr_pos_in_str >= from_pos);

    if (is_negative)
    {
        int flip_from_now_on;
        flip_from_now_on = 0;
        curr_pos_in_str = to_pos;

        do
        {
            curr_char = bits_str[curr_pos_in_str];

            if (flip_from_now_on == 1)
            {
                bits_str[curr_pos_in_str] = curr_char == '0' ? '1' : '0'; /* flip bit*/
            }
            else if (flip_from_now_on == 0 && curr_char == '1')
                flip_from_now_on = 1;

            curr_pos_in_str--;
        } while (curr_pos_in_str >= from_pos);
    }
}
/*base conversion functions END*/
