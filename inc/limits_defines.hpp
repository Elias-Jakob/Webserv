#ifndef LIMITS_DEFINES_HPP
# define LIMITS_DEFINES_HPP

// Request parsing limits
#define MAX_BODY_SIZE 1000000000
#define MAX_HEADERS 16
#define MAX_HEADER_LENGTH 256
#define MAX_REQUEST_LINE_LENGTH 512// 8192      // 8KB
#define MAX_URI_LENGTH 1024

// Network & Connction
# define EPOLL_MAX_EVENTS 10
#define MAX_CONNECTIONS 32
# define KEEP_ALIVE_TIMEOUT 60
#define READ_TIMEOUT 10
#define WRTIE_TIMEOUT 10

// Resource & Performance
#define MAX_FILE_UPLOADS 10
#define CGI_TIMEOUT 30

// Query
# define MAX_QUERY_STRING_LENGTH 2048

#endif
