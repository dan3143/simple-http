#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool file_exists(char *filename) {
  FILE *fp = fopen(filename, "r");

  if (fp == NULL) {
    return false;
  }

  fclose(fp);

  return true;
}

char *get_file_extension(const char *filename) {
  char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return NULL;
  return dot + 1;
}