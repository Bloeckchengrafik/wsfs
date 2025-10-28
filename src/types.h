#include <netinet/in.h>

#define HTTP_PATH_SIZE 256
#define HTTP_BUFFER_SIZE 1024
#define HTTP_MAX_HEADERS 50
#define HTTP_MAX_HEADER_LINE_LENGTH 512
#define HTTP_MAX_HEADER_NAME_LENGTH 256
#define HTTP_MAX_HEADER_VALUE_LENGTH 256
#define HTTP_MAX_METHOD_LENGTH 16
#define HTTP_MAX_FILE_SIZE 1024 * 1024

typedef struct {
    char name[HTTP_MAX_HEADER_NAME_LENGTH];
    char value[HTTP_MAX_HEADER_VALUE_LENGTH];
} header_t;

typedef enum {
    GET,
    HEAD,
    OPTIONS,
    POST,
    PUT,
    DELETE,
    TRACE,
    CONNECT,
    PATCH,
    UNKNOWN
} http_method;

typedef struct {
    int sock_fd;
    struct sockaddr_in client_addr;
    http_method method;
    int header_count;
    char path[HTTP_PATH_SIZE];
    header_t headers[HTTP_MAX_HEADERS];
} wsfs_connection_t;
