# Test HTTP Server

A toy HTTP server I built while learning programming in C. It only supports HTTP/1.1 since I don't want to deal with implementing HTTP/2 framing, multiplexing, flow control, etc.

This project probably contains many errors and has little to no compliance with [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html). Please do not use it for anything serious.

## Build instructions
The project uses a simple Makefile. To build the server just run:

```
make
```

This will produce a `simple-http` executbale into a `./build` directory. To clean build files:

```
make clean
```

## Dependencies
This project will depend on [OpenSSL](https://github.com/openssl/openssl) for future TLS support. To build the project you will need OpenSSL development libraries installed on your system.

### Debian/Ubuntu
```
sudo apt install libssl-dev
```

### Fedora
```
sudo dnf install openssl-devel
```

### Arch Linux
```
sudo pacman -S openssl
```

## Usage

```
simple-http [options]
```

### Command-line options

The currently supported options are:

* `--host, -h`: IP address the server will bind to
* `--port, -p`: Port used to serve HTTP
* `--verbose, -v`: Enable verbose logging
* `--quiet, -q`: Suppress logging
* `--config, -c`: Location of a `.ini` configuration file
* `--tls-enabled, -s`: Whether to support TLS (not implemented)
* `--tls-port, -t`: Port used to serve HTTPS (not implemented)
* `--tls-key, -k`: Location of the private key for TLS (not implemented)
* `--tls-certificate, -r`: Location of the certificate for TLS (not implemented)


# Configuration File

The following settings can be configured in the configuration file using the format:

```
setting = value
```

By default, the program looks for a file in the current directory called `conf.ini`.

---

### host

IP address the server will bind to.

**Default value:** `0.0.0.0`

---

### http_port

Port used to serve HTTP.

**Default value:** `8080`

---

### https_port

Port used to serve HTTPS.

**Default value:** `8443`

---

### doc_root

Directory containing the static files that will be served.

**Default value:** `./`

---

### index_file

Path to the file (relative to `doc_root`) that will be served when requesting `/`.

**Default value:** `index.html`

---

### keepalive_timeout

Amount of time in seconds to wait before the server closes an idle connection.

**Default value:** `15`

---

### log_level

Logging level.

**Accepted values:** `verbose`, `quiet`, `warn`, `informational`
**Default value:** `informational`

---

### max_connections

Maximum number of concurrent connections allowed by the server.

**Default value:** `1024`

---

### workers_number

Number of worker threads used to handle incoming HTTP requests.

**Default value:** `8`

---

### tls_certificate

Location of the TLS certificate file.

**Default value:** `./server.crt`

---

### tls_key

Location of the TLS private key file.

**Default value:** `./server.key`

---

### tls_enabled

Whether TLS will be enabled.

**Accepted values:** `true`, `false`
**Default value:** `false`

---

## Example Configuration File

```
# conf.ini
host = 0.0.0.0
http_port = 8080
https_port = 8443
doc_root = ./
index_file = index.html
keepalive_timeout = 15
log_level = informational
max_connections = 1024
workers_number = 8
tls_enabled = false
tls_certificate = ./server.crt
tls_key = ./server.key
```


# To-do List

* [x] Multithreading
* [x] Persistent connections
* [ ] Better HTTP/1.1 compliance
* [X] TLS support with OpenSSL
* [x] Read configuration from a file

I don't expect to finish everything on this list, but I will try.