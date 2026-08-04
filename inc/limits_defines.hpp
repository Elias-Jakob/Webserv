#ifndef LIMITS_DEFINES_HPP
# define LIMITS_DEFINES_HPP

// Request parsing limits
#define MAX_BODY_SIZE 1000000000
#define MAX_HEADERS 32
#define MAX_HEADER_LENGTH 256
#define MAX_HEADER_NAME_LENGTH 32
#define MAX_HEADER_VALUE_LENGTH 256
#define MAX_REQUEST_LINE_LENGTH 8192      // 8KB
#define MAX_URI_LENGTH 4096

// Network & Connction
# define EPOLL_MAX_EVENTS 10
#define MAX_CONNECTIONS 32
# define KEEP_ALIVE_TIMEOUT 20
#define READ_TIMEOUT 10
#define WRTIE_TIMEOUT 10

// Resource & Performance
#define MAX_FILE_UPLOADS 10
#define CGI_TIMEOUT 30

// Query
# define MAX_QUERY_STRING_LENGTH 2048

// SessionManager
# define SWEEP_INTERVAL 60

// COLOR DEFINES
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define BOLD    "\033[1m"

#endif
