# Test HTTP server

A toy HTTP server I made while learning programming with C. It only supports HTTP/1.1 since I don't wanna deal with implementing HTTP/2 framing, multiplexing, flow control, etc.

It will serve static files from the current folder (modifiable by the `-d` option).

It can be bound to a specific port in a specific IP address using the `-p` and `-h` options respectively.

Usage:

```
simple-http [options]
```

This is just a toy server with a lot of errors and little to no compliance with [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html). Please do not use it for anything serious.

## To-do list:

- [ ] Multithreading
- [ ] Persistent connections
- [ ] Better HTTP/1.1 compliance
- [ ] TLS support with [openssl](https://github.com/openssl/openssl)
- [ ] Read configuration from a file

I don't expect to finish everything laid out in that list, but I will try.
