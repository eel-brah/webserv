# Webserv

A lightweight **HTTP server** implemented in **C++98**.
The goal of this project is to build a functional web server from scratch, fully compatible with modern browsers, supporting **GET, POST, DELETE**, CGI execution, file uploads, multiple ports, and custom configuration.

> This project is inspired by the [RFC standards](https://www.rfc-editor.org/rfc/rfc2616) and mimics the behavior of servers like **NGINX** while being coded from scratch with no external libraries.


## Features

* Non-blocking server with **single `epoll`** handling all I/O  
* Configuration file with:
  * Multiple servers and ports
  * server_name, custom error pages, limit client body size 
  * Location blocks (methods allowed, redirection, root, alias, autoindex, index file, uploads, CGI)
* Virtual Hosting
* Accurate HTTP status codes
* Default error pages
* Support for **GET**, **POST**, **DELETE**
* File upload support
* CGI executions (e.g. PHP, Python)
* Multiple client handling with resilience under stress
* Cookies & session management


## Requirements

* **C++98**
* No external libraries (Boost, etc. forbidden)
* Compiles with:

  ```bash
  c++ -Wall -Wextra -Werror -std=c++98
  ```

Subject: [subject.pdf](./en.subject.pdf)

## Installation

```bash
git clone https://github.com/eel-brah/webserv
cd webserv
make
```


## Usage

Run with a configuration file (inspired by NGINX style):

```bash
./webserv [config_file]
```

Then open your browser and navigate to:

```
http://localhost:PORT
```


## Configuration Example

```nginx
server {
    listen 8080;
    server_name localhost;

    root ./www;
    index index.html;

    error_page 404 ./errors/404.html;

    location / {
        allow GET;
        autoindex on;
    }
    location /upload {
        allow POST;
        upload_store ./uploads;
    }

    location /new {
        allow GET POST;
        alias ./www/newest;
        autoindex on;
    }

    location /cgi {
        cgi_ext .py /usr/bin/python3;
        cgi_ext .js /usr/bin/node;
        cgi_ext .pl /usr/bin/perl;
        allow GET POST DELETE;
    }
}
```


## Authors

* [eel-brah](https://github.com/eel-brah)
* [ELPatrinum](https://github.com/ELPatrinum)
* [imenyoo2](https://github.com/imenyoo2)
