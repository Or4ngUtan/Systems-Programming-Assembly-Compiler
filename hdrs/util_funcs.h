#ifndef UTIL_FUNCS_H
#define UTIL_FUNCS_H

int isTabOrSpace(char c);
void nullifyCharArr(char *char_arr_byRef, int char_arr_len);
int findNextChar(char *str, char c, int from_pos);
int findNextChar_multi(char *host_str, char *terminators, int from_pos);
int digitCharToInt(char ch);
int isNumInRange8bits(int num);
char *generateSubstr(char *host_str, int from_pos, int to_pos);
int findNextWordTerminator(char *host_str, int from_ind);
int skipTabsSpaces(char *str, int curr_pos);
int skipTabsSpaces_reverse(char *str, int right_hand_pos);
int skipToLastNonBlankInLine(char *str);
int isReservedword(char *w);
int isReservedword_asSubstr(char *host_str, int from_pos, int to_pos);

char * appendExtension_to_f_name(const char *f_name_plain, const char *extens_str);

int getBase4digit_by_2base2digits_as_int(char *host_str, int left_dig_pos);
char *machineWordToBase4str(char *mchn_word);
char *intToBase4str_compact(int num);
void populateBitsWithNum_byDecimal(char *bits_str, int from_pos, int to_pos, int decimal_val);

#endif