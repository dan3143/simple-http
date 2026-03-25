#include "misc/util.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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

void get_addr_str(struct sockaddr *sa, char *addr_str) {
  if (sa->sa_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
    inet_ntop(AF_INET, &(ipv4->sin_addr), addr_str, INET6_ADDRSTRLEN);
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
    inet_ntop(AF_INET6, &(ipv6->sin6_addr), addr_str, INET6_ADDRSTRLEN);
  }
}

int get_port(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
    return ntohs(ipv4->sin_port);
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
    return ntohs(ipv6->sin6_port);
  }
}

void print_hex_bytes(char *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {

    printf("%02x  ", (unsigned char)buf[i]);

    if ((i + 1) % 4 == 0) {
      printf("\n");
    }
  }

  if (len % 4 != 0) {
    printf("\n");
  }
}

int safe_str_to_int(char *str, int *output) {
  errno = 0;
  char *endptr;
  int converted_value = strtol(str, &endptr, 10);
  if (errno == ERANGE)
    return 0;
  if (endptr == str)
    return 0;
  *output = converted_value;
  return 1;
}
