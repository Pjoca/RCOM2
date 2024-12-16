#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "include/connection.h"

int main(int argc, char *argv[]) {
    // incorrect usage of the makefile
    if (argc < 2) {
        fprintf(stderr, "[CONSOLE] Correct usage:\n\tmake run URL=[file_path]\n");
        exit(-1);
    }

    URL url;
    
    if (parse(argv[1], &url)) {
        fprintf(stderr, "[CONSOLE] Invalid URL.\n");
        exit(-1);
    }

    // open control connection
    int socket_fd = open_connection(url);
    if (socket_fd < 0) {
        fprintf(stderr, "[CONSOLE] Establishing the control connection.\n");
        exit(-1);
    }
    printf("[CONSOLE] Connection established at %s\n", url.host);

    // login to ftp server
    if (login(socket_fd, url) != 0) {
        fprintf(stderr, "[CONSOLE] Login failed.\n");
        close(socket_fd);
        exit(-1);
    }
    printf("[CONSOLE] Login.\n");

    // entering passive mode & establishing data connection
    char address[BUFFER_SIZE] = {0};
    int port = enter_passive_mode(socket_fd, address);
    if (port < 0) {
        fprintf(stderr, "[CONSOLE] Entering the passive mode.\n");
        close(socket_fd);
        exit(-1);
    }

    // establishing data connection
    int socket_data;
    struct sockaddr_in server_addr;

    // initialize addr struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

    // create socket
    if ((socket_data = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[CONSOLE] Opening the socket.\n");
        exit(-1);
    }

    // connect to server
    if (connect(socket_data, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CONSOLE] Connecting to the server.\n");
        close(socket_data);
        exit(-1);
    }

    printf("[CONSOLE] Connection established at %s:%d\n", address, port);

    get_file(socket_fd, url, socket_data);

    close(socket_fd);
    close(socket_data);

    return 0;
}
