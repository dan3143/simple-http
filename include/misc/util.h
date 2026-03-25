#include <linux/limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#ifndef MISC_UTIL_H
#define MISC_UTIL_H

int get_port(struct sockaddr *);
bool file_exists(char *);
char *get_file_extension(const char *);
void get_addr_str(struct sockaddr *, char *);
void print_hex_bytes(char *, size_t);
int safe_str_to_int(char *, int *);

#endif