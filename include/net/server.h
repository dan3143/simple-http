#ifndef SERVER_H
#define SERVER_H

typedef enum {
  SRV_OK = 0,
  SRV_AGAIN,
  SRV_ERR_PARSE,
  SRV_ERR_NOT_FOUND,
  SRV_ERR_BAD_REQUEST,
  SRV_ERR_INTERNAL,
  SRV_ERR_IO,
  SRV_ERR_OVERFLOW,
} ServerError;

void listen_on(char *, char *);

#endif