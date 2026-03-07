#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
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

int send_all(int fd, char *buf, size_t *len) {
  size_t total = 0;
  int bytes_left = *len;
  int n;
  while (total < *len) {
    n = send(fd, buf + total, bytes_left, 0);
    if (n == -1) {
      break;
    }
    total += n;
    bytes_left -= n;
  }
  *len = total;
  return n == -1 ? -1 : 0;
}