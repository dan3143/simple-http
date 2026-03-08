#include "http/util.h"
#include "net/server.h"

#ifndef HTTP_STRINGS_H
#define HTTP_STRINGS_H

const char *http_code_to_text(HttpCode);
const char *http_code_to_description(HttpCode);
HttpCode srv_err_to_http_err(ServerError);

#endif