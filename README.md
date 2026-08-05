_This project has been created as part of the 42 curriculum by ejakob && cgeringe._

# Description

The objective of this project is to build a simple HTTP webserver in C++98.  
What is a webserver? A webserver is a machine reachable over a network
(most of the time the Internet), running software that listens for incoming
requests, processes them and then sends back responses to the respective caller.
HTTP (Hyper Text Transfer Protocol) is, in essence, the language in which these
request and response conversations are taking place.

### Features & Core Concepts

- HTTP-Methods: A method specifies the action of a request (`GET` to fetch content, `POST` to send data, `DELETE` to remove files).
- I/O Multiplexing: A non-blocking event driven architecture (like `epoll`, which is used in this project) allows a single-threaded server event loop to monitor hundreds of file descriptors simultaneously.
- CGI (Common Gateway Interface): Generates dynamic content by executing external scripts in a child process.
- Cookie & Session Management: Tracks user state across stateless HTTP requests using session IDs and Set-Cookie headers.

### Standards

The server is loosely based on `HTTP/1.0`/`HTTP/1.1` and `CGI/1.1`.

# Instructions

1. __Build:__ Run `make` in the root directory of the repository to compile the webserv executable.
2. __Configuration:__ In order to launch the webserver, you need to provide some configurations (e.g. `conf/webserv.conf`). A simple config file could look like this:

```
server {
    listen = 127.0.0.1:8080;
    server_name = example.com;

    error_page 404 = /www/errors/404.html;
    error_page 500 = /www/errors/500.html;

    client_max_body_size = 2K;
    root = /www;

    location / {
        accepted_methods = GET, POST;
        index = index.html;
        autoindex = on;
    }
    location /cgi-bin {
        accepted_methods = GET, POST;
        alias = /cgi-bin;
        cgi_extension = .php, .py;
    }
}

```

3. __Run:__ To start up the webserver, pass the config file path as an argument.

```

./webserv <config_file_path.conf>

```

4. __Access:__ The webserver is now up and running. Open your browser or terminal (`curl`) and navigate to the IP and port combination specified in your config file (e.g. `http://127.0.0.1:8080`).

# Resources

- [Hypertext Transfer Protocol -- HTTP/1.1 RFC](https://datatracker.ietf.org/doc/html/rfc2616)  
- [The Common Gateway Interface (CGI) Version 1.1 RFC](https://datatracker.ietf.org/doc/html/rfc3875)  
- [An Introduction to Network Programming in UNIX](https://www.inf.usi.ch/carzaniga/edu/adv-ntw25s/socket_programming.html)  
- [Medium article about epoll: The Engine Behind High-Performance Linux Networking](https://medium.com/@m-ibrahim.research/mastering-epoll-the-engine-behind-high-performance-linux-networking-85a15e6bde90)  
