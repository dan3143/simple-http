#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stddef.h>

typedef struct {
  int capacity;
  int length;
  char *data;
} StringBuilder;

int sb_init(StringBuilder *);
void sb_free(StringBuilder *);
int sb_appendf(StringBuilder *, const char *, ...);
int sb_appendn(StringBuilder *, const char *, size_t);
int sb_append(StringBuilder *, const char *);

#endif