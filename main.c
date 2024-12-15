#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "include/connection.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n\t./download [file_path]\n");
        exit(EXIT_FAILURE);
    }

    Url url;
    if (parseUrl(argv[1], &url)) {
        fprintf(stderr, "The provided URL is not valid.\n");
        exit(EXIT_FAILURE);
    }

    // Open a TCP connection at the given URL
    int sockfd = openControlConnection(url);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to establish control connection.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connection established: %s\n", url.host);

    if (login(sockfd, url) != 0) {
        fprintf(stderr, "Login failed.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Logged in\n");

    // Enter passive mode and establish a data connection
    char address[BUFFER_SIZE] = {0};
    int port = enterPassiveMode(sockfd, address);
    if (port < 0) {
        fprintf(stderr, "Failed to enter passive mode.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int datafd = openConnection(address, port);
    if (datafd < 0) {
        fprintf(stderr, "Failed to establish data connection to %s:%d.\n", address, port);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connection established: %s:%d\n", address, port);

    /*if (getFile(sockfd, url, datafd) != 0) {
        fprintf(stderr, "Failed to retrieve file.\n");
    }*/
    getFile(sockfd, url, datafd);

    close(sockfd);
    close(datafd);

    return 0;
}
