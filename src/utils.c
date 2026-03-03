#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

bool file_exists(char *filename) {
  FILE *fp = fopen(filename, "r");
  bool exists = true;
  if (fp == NULL) {
    exists = false;
  }
  fclose(fp);
  return exists;
}