#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef NEED_SOME_UITL_FUNCTIONS
#define NEED_SOME_UTIL_FUNCTIONS

char* dsubstr_replace_all(char* orig, char* search, char* replace);
char* int_to_dynstr(int n);
char* malloc_atoi(int val);
int strlen_int(int val);



#endif