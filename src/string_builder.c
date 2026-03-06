#include "string_builder.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_INITIAL_CAPACITY 64

int sb_init(StringBuilder *sb) {
  sb->capacity = SB_INITIAL_CAPACITY;
  sb->data = malloc(sb->capacity);
  if (sb->data == NULL) {
    return -1;
  }
  sb->length = 0;
  sb->data[0] = '\0';
  return 0;
}

void sb_free(StringBuilder *sb) {
  free(sb->data);
  sb->data = NULL;
  sb->capacity = 0;
  sb->length = 0;
}

int sb_grow(StringBuilder *sb, size_t min_capacity) {
  int new_capacity = sb->capacity;
  if (new_capacity < min_capacity) {
    new_capacity *= 2;
  }
  char *new_data = realloc(sb->data, new_capacity);
  if (new_data == NULL) {
    return -1;
  }
  sb->data = new_data;
  sb->capacity = new_capacity;
  return 0;
}

int sb_appendn(StringBuilder *sb, const char *str, size_t n) {
  size_t required_size = sb->length + n + 1;
  if (required_size > sb->capacity) {
    if (sb_grow(sb, required_size) < 0)
      return -1;
  }
  memcpy(sb->data + sb->length, str, n);
  sb->length += n;
  sb->data[sb->length] = '\0';
  return 0;
}

int sb_append(StringBuilder *sb, const char *str) {
  return sb_appendn(sb, str, strlen(str));
}

int sb_appendf(StringBuilder *sb, const char *fmt, ...) {

  va_list args;
  va_start(args, fmt);

  va_list copy;
  va_copy(copy, args); // orignal args will be modified after vsnprintf

  int needed_bytes = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  if (needed_bytes < 0) {
    va_end(copy);
    return -1;
  }

  size_t required_capacity = sb->length + needed_bytes + 1;
  if (required_capacity > sb->capacity) {
    if (sb_grow(sb, required_capacity) < 0) {
      va_end(copy);
      return -1;
    }
  }

  vsnprintf(sb->data + sb->length, needed_bytes + 1, fmt, copy);
  sb->length += needed_bytes;
  va_end(copy);
  return 0;
}
