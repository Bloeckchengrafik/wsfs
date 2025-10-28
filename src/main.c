#include "log.h"
#include "types.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

http_method parse_http_method(const char *method) {
    if (!method) {
        return UNKNOWN;
    }

    if (strcmp(method, "GET") == 0) {
        return GET;
    } else if (strcmp(method, "HEAD") == 0) {
        return HEAD;
    } else if (strcmp(method, "OPTIONS") == 0) {
        return OPTIONS;
    } else if (strcmp(method, "POST") == 0) {
        return POST;
    } else if (strcmp(method, "PUT") == 0) {
        return PUT;
    } else if (strcmp(method, "DELETE") == 0) {
        return DELETE;
    } else if (strcmp(method, "TRACE") == 0) {
        return TRACE;
    } else if (strcmp(method, "CONNECT") == 0) {
        return CONNECT;
    } else if (strcmp(method, "PATCH") == 0) {
        return PATCH;
    }

    return UNKNOWN;
}

const char * http_method_to_string(http_method method) {
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case PUT:
            return "PUT";
        case DELETE:
            return "DELETE";
        case TRACE:
            return "TRACE";
        case CONNECT:
            return "CONNECT";
        case PATCH:
            return "PATCH";
        default:
            return "UNKNOWN";
    }
}

int parse_request_line(http_method *method, char *path, char *request_line) {
    if (!request_line) {
        WSFS_LOGE("Request line is NULL");
        return -1;
    }

    char *space = strchr(request_line, ' ');
    if (!space) {
        WSFS_LOGE("Failed to parse request line");
        return -1;
    }

    *space = '\0';
    char method_str[HTTP_MAX_METHOD_LENGTH];
    strcpy(method_str, request_line);
    strcpy(path, space + 1);

    char *next = strchr(path, ' ');
    if (!next) {
        WSFS_LOGE("Failed to parse request line");
        return -1;
    }

    *method = parse_http_method(method_str);
    if (*method == UNKNOWN) {
        WSFS_LOGE("Unknown HTTP method");
        return -1;
    }

    *next = '\0';
    return 0;
}

#define MIME_HTML "text/html"
#define MIME_CSS "text/css"
#define MIME_JS "application/javascript"
#define MIME_JSON "application/json"
#define MIME_PNG "image/png"
#define MIME_JPEG "image/jpeg"
#define MIME_GIF "image/gif"
#define MIME_SVG "image/svg+xml"
#define MIME_XML "application/xml"
#define MIME_TEXT "text/plain"
#define MIME_FONT "font/woff2"

const char* mime_type_by_uri(const char *url) {
    if (strstr(url, ".html")) return MIME_HTML;
    if (strstr(url, ".css")) return MIME_CSS;
    if (strstr(url, ".js")) return MIME_JS;
    if (strstr(url, ".json")) return MIME_JSON;
    if (strstr(url, ".png")) return MIME_PNG;
    if (strstr(url, ".jpg")) return MIME_JPEG;
    if (strstr(url, ".jpeg")) return MIME_JPEG;
    if (strstr(url, ".gif")) return MIME_GIF;
    if (strstr(url, ".svg")) return MIME_SVG;
    if (strstr(url, ".xml")) return MIME_XML;
    if (strstr(url, ".txt")) return MIME_TEXT;
    if (strstr(url, ".woff2")) return MIME_FONT;
    return MIME_TEXT;
}

int wsfs_parse_request(wsfs_connection_t *conn) {
    char buffer[HTTP_BUFFER_SIZE];
    ssize_t bytes_read;

    if ((bytes_read = recv(conn->sock_fd, buffer, HTTP_BUFFER_SIZE - 1, 0)) > 0) {
      buffer[bytes_read] = '\0';
      WSFS_LOGD("Received %zd bytes", bytes_read);
    } else {
        WSFS_LOGE("Failed to receive data");
        return -1;
    }

    char request_line[HTTP_MAX_HEADER_LINE_LENGTH];

    char *next = strtok(buffer, "\r\n");
    if (!next) {
        WSFS_LOGE("Failed to parse request line");
        return -1;
    }

    strcpy(request_line, next);
    WSFS_LOGD("Received request line: %s", request_line);

    int res = parse_request_line(&conn->method, conn->path, request_line);
    if (res != 0) {
        WSFS_LOGE("Failed to parse request line");
        return -1;
    }

    conn->header_count = 0;

    while ((next = strtok(NULL, "\r\n"))) {
        char *name = strstr(next, ": ");
        if (!name) {
            WSFS_LOGE("Failed to parse header");
            return -1;
        }
        header_t *header = &conn->headers[conn->header_count++];
        size_t name_len = name - next;
        if (name_len >= HTTP_MAX_HEADER_NAME_LENGTH) {
            WSFS_LOGE("Header name too long");
            return -1;
        }
        memcpy(header->name, next, name_len);
        header->name[name_len] = '\0';

        size_t value_len = strlen(name) - 2;
        if (value_len >= HTTP_MAX_HEADER_VALUE_LENGTH) {
            WSFS_LOGE("Header value too long");
            return -1;
        }
        memcpy(header->value, name + 2, value_len);
        header->value[value_len] = '\0';

        if (conn->header_count >= HTTP_MAX_HEADERS) {
            WSFS_LOGE("Too many headers, skipping remaining");
            break;
        }
    }

    return 0;
}

void wsfs_inner(wsfs_connection_t *conn) {
    if (wsfs_parse_request(conn) != 0) {
        WSFS_LOGE("Failed to parse request");
        return;
    }

    WSFS_LOGD("Request method: %s", http_method_to_string(conn->method));
    WSFS_LOGD("Request path: %s", conn->path);

    for (int i = 0; i < conn->header_count; i++) {
        WSFS_LOGD("Header %d: %s: %s", i, conn->headers[i].name, conn->headers[i].value);
    }

    // Get current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        WSFS_LOGD("Current working directory: %s", cwd);
    } else {
        WSFS_LOGD("Failed to get current working directory");
    }


    char static_file_path[HTTP_MAX_HEADER_LINE_LENGTH];
    const char *runfiles = getenv("RUNFILES_DIR");
    if (!runfiles) runfiles = ".";
    snprintf(static_file_path, sizeof(static_file_path), "%s%s%s", runfiles, "/src/staticfiles", conn->path);

    // Check for path traversal
    if (strstr(conn->path, "..") != NULL) {
        WSFS_LOGE("Path traversal detected in request path: %s", conn->path);
        const char *response =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 28\r\n"
            "\r\n"
            "Path traversal detected.\r\n\r\n";
        send(conn->sock_fd, response, strlen(response), 0);
        return;
    }

    FILE *file = fopen(static_file_path, "rb");
    WSFS_LOGE("new open file: %s %p", static_file_path, file);

    int is_directory = 0;
    struct stat file_stat;
    if (file && !fstat(fileno(file), &file_stat)) {
        is_directory = S_ISDIR(file_stat.st_mode);
    }

    if (file == NULL || is_directory) {
        WSFS_LOGE("Failed to open file: %s", static_file_path);
        size_t path_len = strlen(static_file_path);
        if (path_len == 0 || static_file_path[path_len - 1] != '/') {
            if (path_len + 1 < sizeof(static_file_path)) {
                static_file_path[path_len] = '/';
                static_file_path[path_len + 1] = '\0';
                path_len++;
            }
        }
        if (path_len + strlen("index.html") < sizeof(static_file_path)) {
            strcat(static_file_path, "index.html");
        }
        file = fopen(static_file_path, "rb");
    }

    if (file == NULL) {
        WSFS_LOGE("Failed to open file: %s", static_file_path);
        const char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 19\r\n"
            "\r\n"
            "File not found.\r\n\r\n";
        send(conn->sock_fd, response, strlen(response), 0);
        return;
    }

    const char* content_type = mime_type_by_uri(static_file_path);

    char buffer[HTTP_MAX_FILE_SIZE];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);// If fread returns 0, it could be because the file is empty, or because of an error.
    if (bytes_read == 0 && ferror(file)) {
        WSFS_LOGE("fread error on file: %s", static_file_path);
        clearerr(file);
    }

    char response[HTTP_BUFFER_SIZE];
    snprintf(response, HTTP_BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", content_type, bytes_read);

    send(conn->sock_fd, response, strlen(response), 0);
    send(conn->sock_fd, buffer, bytes_read, 0);

    fclose(file);
}

void *wsfs_handle_connection(void *arg) {
  wsfs_connection_t *conn = (wsfs_connection_t *)arg;

  WSFS_LOGI("Handling connection from %s:%d", inet_ntoa(conn->client_addr.sin_addr), ntohs(conn->client_addr.sin_port));
  wsfs_inner(conn);
  WSFS_LOGD("Connection handled");

  close(conn->sock_fd);
  free(conn);

  return NULL;
}

int main(int argc, char *argv[]) {
  WSFS_LOGI("Starting WSFS");

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  WSFS_CHECKERR(fd == -1, "Failed to create socket");

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int res = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int));
  WSFS_CHECKERR(res == -1, "Failed to set socket options");

  res = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  WSFS_CHECKERR(res < 0, "Failed to bind socket");

  res = listen(fd, SOMAXCONN);
  WSFS_CHECKERR(res == -1, "Failed to listen on socket");

  struct sockaddr_in client_addr;
  socklen_t client_addrlen = sizeof(client_addr);
  int acceptFd;

  WSFS_LOGI("WSFS bound to :8080");

  while (1) {
    acceptFd = accept(fd, (struct sockaddr *)&client_addr, &client_addrlen);
    WSFS_CHECKERR(acceptFd == -1, "Failed to accept connection");

    wsfs_connection_t *conn = malloc(sizeof(wsfs_connection_t));
    conn->sock_fd = acceptFd;
    conn->client_addr = client_addr;

    pthread_t thread;
    pthread_create(&thread, NULL, wsfs_handle_connection, conn);
    pthread_detach(thread);
  }

  return 0;
}
