#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utilities.h"

// replaces a one dyanmically allocated string with another and frees the original
char* dsubstr_replace_all(char* orig, char* search, char* replace) {

  int size_delta = strlen(replace) - strlen(search);

  char* new_str = orig;
  char* ss_ptr = strstr(new_str, search);
  
    while (ss_ptr != NULL) {
      new_str = calloc(strlen(orig) + size_delta + 1, sizeof(char));
      strncpy(new_str, orig, ss_ptr - orig);
      strcat(new_str, replace);
      ss_ptr = ss_ptr + strlen(search);
      strcat(new_str, ss_ptr);
      free(orig);
      orig = new_str;
      ss_ptr = strstr(orig, search);
    }
  return new_str;
}

// Takes an interger and returns a pointer to dyanamically allocated string of characters
// that represent that integer.
// REFERENCE:
// Used code linked at this link as a starting point (but modified significantly)
// https://stackoverflow.com/questions/9994742/want-to-convert-integer-to-string-without-itoa-function
char* malloc_atoi(int val) {
  int i = 0;
  bool positive = true;

  int len = strlen_int(val);
  char* str = calloc(len, sizeof(char*));

  if (val < 0) {
    val = -val;
    positive = false;
    str[0] = '-';
    i++;
  }

  while (i < len) {
    str[len - i - (int) positive] = val % 10 + '0';
    val /= 10;
    i++;
  }
  
  str[i] = '\0';

  return str;
}

// determines the number of characters that will be needed to print an integer as a string (in base 10)
int strlen_int(int val) {
  int len = 0;
  if (val < 0) {
    len++;
  }
 do {
    val /= 10;
    len++;
  }  while (val != 0);
  return len;
}

