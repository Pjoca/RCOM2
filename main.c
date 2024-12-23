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
    printf("[CONSOLE] URL parsed.\n");

    // open control connection
    int sockfd;
    if (open_connection(&sockfd, url) != 0) {
        fprintf(stderr, "[CONSOLE] Establishing the control connection.\n");
        exit(-1);
    }
    printf("[DEBUG] Connected to server.\n");

    if (sockfd < 0) {
        fprintf(stderr, "[CONSOLE] Failed to establish the control connection.\n");
        exit(-1);
    }
    printf("[CONSOLE] Connection established at %s\n", url.host);

    sleep(1);
    char buffer[1024];

    if (read(sockfd, buffer, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    buffer[3] = '\0';
    printf("[DEBUG] Response after connection: %s\n", buffer);

    if (strcmp(buffer, SERVER_READY) != 0) {
        perror("Failed to connect to the service");
        exit(-1);
    }

    // login to ftp server
    if (login(&sockfd, url) != 0) {
        fprintf(stderr, "[CONSOLE] Login failed.\n");
        close(sockfd);
        exit(-1);
    }
    printf("[CONSOLE] Login.\n");

    // entering passive mode & establishing data connection
    char address[BUFFER_SIZE] = {0};
    int port = enter_passive_mode(&sockfd, address);
    if (port < 0) {
        fprintf(stderr, "[CONSOLE] Entering the passive mode.\n");
        close(sockfd);
        exit(-1);
    }

    // establishing data connection
    int datafd;
    struct sockaddr_in server_addr;

    // initialize addr struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

    // create socket
    if ((datafd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[CONSOLE] Opening the socket.\n");
        exit(-1);
    }

    // connect to server
    if (connect(datafd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CONSOLE] Connecting to the server.\n");
        close(datafd);
        exit(-1);
    }

    printf("[CONSOLE] Connection established at %s:%d\n", address, port);

    if (get_file(&sockfd, url, &datafd) != 0) {
        fprintf(stderr, "[CONSOLE] Getting the file.\n");
        close(sockfd);
        close(datafd);
        exit(-1);
    }

    if (server_close(&sockfd) != 0) {
        fprintf(stderr, "[CONSOLE] Closing the connection.\n");
        close(sockfd);
        close(datafd);
        exit(-1);
    }

    close(sockfd);
    return 0;
}
