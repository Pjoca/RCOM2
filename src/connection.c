#include "../include/connection.h"

#define CHECK_ALLOC(ptr) if ((ptr) == NULL) { perror("Memory allocation failed"); exit(EXIT_FAILURE); }

int openConnection(const char* address, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to open socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int openControlConnection(Url url) {
    struct hostent* host = gethostbyname(url.host);
    if (host == NULL) {
        perror("Failed to resolve host");
        exit(EXIT_FAILURE);
    }

    const char* address = inet_ntoa(*(struct in_addr*)host->h_addr);
    int sockfd = openConnection(address, TCP_PORT);

    if (!readCode(sockfd, "220")) {
        perror("Unexpected status code on connection");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    cleanSocket(sockfd);
    return sockfd;
}

int login(int sockfd, Url url) {
    char user[BUFFER_SIZE], pass[BUFFER_SIZE], code[BUFFER_SIZE];

    snprintf(user, BUFFER_SIZE, "user %s\n", url.user);
    snprintf(pass, BUFFER_SIZE, "pass %s\n", url.password);

    cleanSocket(sockfd);

    write(sockfd, user, strlen(user));
    if (getSocketLine(sockfd, code) != 0) {
        perror("Failed to read login response");
        exit(EXIT_FAILURE);
    }

    code[3] = '\0';
    if (strcmp(code, "331") != 0 && strcmp(code, "230") != 0) {
        fprintf(stderr, "Invalid username: %s\n", url.user);
        exit(EXIT_FAILURE);
    }

    if (strcmp(code, "331") == 0) {
        write(sockfd, pass, strlen(pass));
        if (!readCode(sockfd, "230")) {
            fprintf(stderr, "Invalid password: %s\n", url.password);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int enterPassiveMode(int sockfd, char* address) {
    char buf[BUFFER_SIZE], code[BUFFER_SIZE];

    const char* pasv = "pasv\n";
    write(sockfd, pasv, strlen(pasv));

    if (getSocketLine(sockfd, buf) != 0) {
        perror("Failed to enter passive mode");
        exit(EXIT_FAILURE);
    }

    strncpy(code, buf, 3);
    code[3] = '\0';

    if (strcmp(code, "227") != 0) {
        fprintf(stderr, "Unexpected response for PASV: %s\n", buf);
        exit(EXIT_FAILURE);
    }

    char* token = strtok(buf, "(");
    for (int i = 0; i < 4; i++) {
        token = strtok(NULL, ",");
        strcat(address, token);
        if (i < 3) strcat(address, ".");
    }

    int portMSB = atoi(strtok(NULL, ","));
    int portLSB = atoi(strtok(NULL, ","));

    return portMSB * 256 + portLSB;
}

void getFile(int sockfd, Url url, int datafd) {
    char buf[1024];
    const char retr[] = "retr ";

    write(sockfd, retr, strlen(retr));
    write(sockfd, url.path, strlen(url.path));
    write(sockfd, "\n", 1);

    int file = open(getFileName(url.path), O_WRONLY | O_CREAT, 0777);
    if (file == -1) {
        perror("Failed to create file");
        exit(EXIT_FAILURE);
    }

    ssize_t nbytes;
    while ((nbytes = read(datafd, buf, sizeof(buf))) > 0) {
        if (write(file, buf, nbytes) != nbytes) {
            perror("Failed to write to file");
            close(file);
            exit(EXIT_FAILURE);
        }
    }

    if (nbytes < 0) {
        perror("Error reading data");
    }

    close(file);
}
