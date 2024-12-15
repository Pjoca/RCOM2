#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "include/connection.h"

int main(int argc, char *argv[]) {
    // incorrect usage of the makefile
    if (argc < 2) {
        fprintf(stderr, "[Error] Correct usage:\n\tmake run URL=[file_path]\n");
        exit(-1);
    }

    Url url;
    
    if (parse_url(argv[1], &url)) {
        fprintf(stderr, "[Error] Invalid URL.\n");
        exit(-1);
    }

    // open control connection
    int socket_fd = open_control_connection(url);
    if (socket_fd < 0) {
        fprintf(stderr, "[Error] Establishing the control connection.\n");
        exit(-1);
    }
    printf("[Success] Connection established at %s\n", url.host);

    // login to ftp server
    if (login(socket_fd, url) != 0) {
        fprintf(stderr, "[Error] Login failed.\n");
        close(socket_fd);
        exit(-1);
    }
    printf("[Success] Login.\n");

    // entering passive mode & establishing data connection
    char buffer[BUFFER_SIZE] = {0};
    int port = enter_passive_mode(socket_fd, buffer);
    if (port < 0) {
        fprintf(stderr, "[Error] Entering the passive mode.\n");
        close(socket_fd);
        exit(-1);
    }

    // establishing data connection
    int data_fd = open_connection(buffer, port);
    if (data_fd < 0) {
        fprintf(stderr, "[Error] Establishing the data connection to %s:%d.\n", buffer, port);
        close(socket_fd);
        exit(-1);
    }

    printf("[Success] Connection established at %s:%d\n", buffer, port);

    get_file(socket_fd, url, data_fd);

    close(socket_fd);
    close(data_fd);

    return 0;
}
