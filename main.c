#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "include/connection.h"

int main(int argc, char *argv[]) {
    // incorrect usage of the makefile
    if (argc < 2) {
        fprintf(stderr, "[Error] Correct usage:\n\t./download [file_path]\n");
        exit(EXIT_FAILURE);
    }

    Url url;
    if (parse_url(argv[1], &url)) {
        fprintf(stderr, "[Error] Invalid URL.\n");
        exit(EXIT_FAILURE);
    }

    // open control connection
    int sockfd = open_control_connection(url);
    if (sockfd < 0) {
        fprintf(stderr, "[Error] Establishing the control connection.\n");
        exit(EXIT_FAILURE);
    }
    printf("[Success] Connection established at %s\n", url.host);

    // login to ftp server
    if (login(sockfd, url) != 0) {
        fprintf(stderr, "[Error] Login failed.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[Success] Logged in\n");

    // entering passive mode & establishing data connection
    char address[BUFFER_SIZE] = {0};
    int port = enter_passive_mode(sockfd, address);
    if (port < 0) {
        fprintf(stderr, "[Error] Entering the passive mode.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // establishing data connection
    int datafd = open_connection(address, port);
    if (datafd < 0) {
        fprintf(stderr, "[Error] Establishing the data connection to %s:%d.\n", address, port);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[Success] Connection established at %s:%d\n", address, port);

    get_file(sockfd, url, datafd);

    close(sockfd);
    close(datafd);

    return 0;
}
