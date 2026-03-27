#include "http/util.h"
#include "net/server.h"

const char *http_code_to_text(HttpCode code) {
  switch (code) {
  case HTTP_OK:
    return "OK";
  case HTTP_BAD_REQUEST:
    return "Bad Request";
  case HTTP_VERSION_NOT_SUPPORTED:
    return "HTTP Version Not Supported";
  case HTTP_CONTENT_TOO_LARGE:
    return "Content Too Large";
  case HTTP_FORBIDDEN:
    return "Forbidden";
  case HTTP_FOUND:
    return "Found";
  case HTTP_INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case HTTP_NOT_FOUND:
    return "Not Found";
  case HTTP_MOVED_PERMANENTLY:
    return "Moved Permanently";
  case HTTP_CONTINUE:
    return "Continue";
  case HTTP_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case HTTP_URI_TOO_LONG:
    return "URI Too Long";
  case HTTP_NOT_IMPLEMENTED:
    return "Not Implemented";
  }
}

const char *http_code_to_description(HttpCode code) {
  switch (code) {
  case HTTP_OK:
    return "Request completed successfully";
  case HTTP_BAD_REQUEST:
    return "Malformed syntax, invalid framing, or deceptive request routing.";
  case HTTP_VERSION_NOT_SUPPORTED:
    return "Server does not support the HTTP version used in the request.";
  case HTTP_CONTENT_TOO_LARGE:
    return "Request body exceeds server or endpoint limits.";
  case HTTP_FORBIDDEN:
    return "Server understood the request but refuses to authorize it.";
  case HTTP_FOUND:
    return "Temporary redirect.";
  case HTTP_INTERNAL_SERVER_ERROR:
    return "Generic catch-all for unhandled server-side exceptions.";
  case HTTP_NOT_FOUND:
    return "Resource not found at this URI.";
  case HTTP_MOVED_PERMANENTLY:
    return "Resource permanently relocated. Clients should update stored URLs.";
  case HTTP_METHOD_NOT_ALLOWED:
    return "The request method is not supported for the requested resource.";
  case HTTP_URI_TOO_LONG:
    return "The URI provided was too long for the server to process.";
  case HTTP_NOT_IMPLEMENTED:
    return "The server either does not recognize the request method, or it "
           "lacks the ability to fulfil the request.";
  case HTTP_CONTINUE:
    return "";
  }
  return "";
}

HttpCode srv_err_to_http_err(ServerError err) {
  switch (err) {
  case SRV_ERR_INTERNAL:
  case SRV_ERR_IO:
    return HTTP_INTERNAL_SERVER_ERROR;

  case SRV_ERR_NOT_FOUND:
    return HTTP_NOT_FOUND;

  case SRV_ERR_PARSE:
  case SRV_ERR_BAD_REQUEST:
    return HTTP_BAD_REQUEST;

  case SRV_OK:
    return HTTP_OK;

  case SRV_AGAIN:
    return HTTP_CONTINUE;
  case SRV_ERR_OVERFLOW:
    return HTTP_CONTENT_TOO_LARGE;
  }
}