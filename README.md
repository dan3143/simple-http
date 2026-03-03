# Test HTTP server

Barebones HTTP server I made while learning programming with C.

It will serve files from a "static" folder in the executable's working directory. In the future this should be configurable.

The routes are hardcoded into a `ROUTES` array under `http_processing.c`. In the future this should also be configurable.

It can be bound to a specific port in a specific IP address.

Usage:

```
simple-http [port [ip address]]
```