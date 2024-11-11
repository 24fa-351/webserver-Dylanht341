#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 80
#define LISTEN_BACKLOG 5

int requests_count = 0;
long total_bytes_received = 0;
long total_bytes_sent = 0;

void send_response(int socket_fd, const char *status, const char *content_type, const char *body) {
    char response[1024];
    int response_size = snprintf(response, sizeof(response),
                                 "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s",
                                 status, content_type, strlen(body), body);
    write(socket_fd, response, response_size);
    total_bytes_sent += response_size;
}

void static_file(){ }

void stats(int socket_fd) {
    char body[1024];
    snprintf(body, sizeof(body),
             "<html><body><h1>Server Stats</h1><p>Requests: %d</p><p>Bytes Received: %ld</p><p>Bytes Sent: %ld</p></body></html>",
             requests_count, total_bytes_received, total_bytes_sent);
    send_response(socket_fd, "200 OK", "text/html", body);
}

void calculate(int socket_fd, const char *query) {
    int a = 0;
    int b = 0;
    char body[1024];

    sscanf(query, "a=%d&b=%d", &a, &b);
    snprintf(body, sizeof(body),
             "<html><body><h1>Server Stats</h1><p>%d + %d = %d</p></body></html>",
             a, b, a + b);
    send_response(socket_fd, "200 OK", "text/html", body);
}

void handleConnection(int* socket_fd_ptr) {
    int socket_fd = *socket_fd_ptr;
    free(socket_fd_ptr);

    printf("Handling connection on %d\n", socket_fd);
    char buffer[1024];
    int bytes_read = read(socket_fd, buffer, sizeof(buffer));
    requests_count++;
    total_bytes_received += bytes_read;
    printf("Received: %s\n", buffer);

    char method[8];
    char path[256];
    
    sscanf(buffer, "%s %s", method, path);
    if (strcmp(method, "GET") == 0) {
        if (strncmp(path, "/static/", 8) == 0) {

        } else if (strcmp(path, "/stats") == 0) {
            stats(socket_fd);
        } else if (strncmp(path, "/calc", 5) == 0) {
            char *query = strchr(path, "?");
            calculate(socket_fd, query);
        }
    }
    printf("Ending connection on %d\n", socket_fd);
}

int main(int argc, char* argv[]) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in socket_address;
    memset(&socket_address, '\0', sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_address.sin_port = htons(PORT);

    int returnval;

    returnval = bind(socket_fd, (struct sockaddr*) &socket_address, sizeof(socket_address));
    returnval = listen(socket_fd, LISTEN_BACKLOG);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while(1) {
        pthread_t thread;
        int *client_fd_buf = malloc(sizeof(int));

        *client_fd_buf = accept(socket_fd, (struct sockaddr*)&client_address, &client_address_len);
        printf("Accepted connection on %d\n", *client_fd_buf);
        pthread_create(&thread, NULL, (void* (*) (void*))handleConnection, (void*)&*client_fd_buf);
        
    }
    return 0;
}