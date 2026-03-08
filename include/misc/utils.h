#include <linux/limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#ifndef UTILS_H
#define UTILS_H

bool file_exists(char *);
char *get_file_extension(const char *);
void get_addr_str(struct sockaddr *, char *);
int get_port(struct sockaddr *);
int send_all(int, char *, size_t *);

typedef struct {
  char host[INET6_ADDRSTRLEN];
  char port[5];
  char root_dir[PATH_MAX];
  int log_level;
} ServerConfig;

#endif