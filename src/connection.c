#include "../include/connection.h"

#define CHECK_ALLOC(ptr) if ((ptr) == NULL) { perror("[Error] Allocating memory.\n"); exit(EXIT_FAILURE); }

int open_connection(const char* address, int port) {
    int socket_fd;
    struct sockaddr_in server_addr;

    // initialize addr struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

    // create socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[Error] Opening the socket.\n");
        exit(EXIT_FAILURE);
    }

    // connect to server
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[Error] Connecting to the server.\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

int open_control_connection(Url url) {
    struct hostent* host = gethostbyname(url.host);
    if (host == NULL) {
        perror("[Error] Invalid host.\n");
        exit(EXIT_FAILURE);
    }

    // convert the resolved IP addr
    const char* address = inet_ntoa(*(struct in_addr*)host->h_addr);

    // establish connection
    int socket_fd = open_connection(address, TCP_PORT);

    // verify server's initial response code
    if (!read_code(socket_fd, "220")) {
        perror("[Error] Unexpected error code.\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    clean_socket(socket_fd);
    return socket_fd;
}

int login(int socket_fd, Url url) {
    char user[BUFFER_SIZE], pass[BUFFER_SIZE], code[BUFFER_SIZE];

    // prepare ftp commands
    snprintf(user, BUFFER_SIZE, "user %s\n", url.user);
    snprintf(pass, BUFFER_SIZE, "pass %s\n", url.password);

    clean_socket(socket_fd);

    // send username
    write(socket_fd, user, strlen(user));
    if (get_socket_line(socket_fd, code) != 0) {
        perror("[Error] Reading the login response.\n");
        exit(EXIT_FAILURE);
    }

    // validate the username response
    code[3] = '\0';
    if (strcmp(code, "331") != 0 && strcmp(code, "230") != 0) {
        fprintf(stderr, "[Error] Invalid username: %s\n", url.user);
        exit(EXIT_FAILURE);
    }

    // send password command if needed
    if (strcmp(code, "331") == 0) {
        write(socket_fd, pass, strlen(pass));
        if (!read_code(socket_fd, "230")) {
            fprintf(stderr, "[Error] Invalid password: %s\n", url.password);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int enter_passive_mode(int socket_fd, char* address) {
    char buf[BUFFER_SIZE], code[BUFFER_SIZE];

    // send PASV command
    const char* pasv = "pasv\n";
    write(socket_fd, pasv, strlen(pasv));

    // read server's response
    if (get_socket_line(socket_fd, buf) != 0) {
        perror("[Error] Entering the passive mode.\n");
        exit(EXIT_FAILURE);
    }

    // validate response code
    strncpy(code, buf, 3);
    code[3] = '\0';
    if (strcmp(code, "227") != 0) {
        fprintf(stderr, "[Error] Unexpected PASV response: %s\n", buf);
        exit(EXIT_FAILURE);
    }

    // parse ip addr and port and execute ip addr
    char* token = strtok(buf, "(");
    for (int i = 0; i < 4; i++) {
        token = strtok(NULL, ",");
        strcat(address, token);
        if (i < 3) strcat(address, ".");
    }

    // extract port
    int portMSB = atoi(strtok(NULL, ","));
    int portLSB = atoi(strtok(NULL, ","));

    return portMSB * 256 + portLSB;
}

void get_file(int socket_fd, Url url, int datafd) {
    char buf[1024];

    // send retr command
    const char retr[] = "retr ";
    write(socket_fd, retr, strlen(retr));
    write(socket_fd, url.path, strlen(url.path));
    write(socket_fd, "\n", 1);

    // open output file
    int file = open(get_file_name(url.path), O_WRONLY | O_CREAT, 0777);
    if (file == -1) {
        perror("[Error] Creating file.\n");
        exit(EXIT_FAILURE);
    }

    // read from data socket and write to file
    ssize_t nbytes; 
    while ((nbytes = read(datafd, buf, sizeof(buf))) > 0) {
        if (write(file, buf, nbytes) != nbytes) {
            perror("[Error] Writing file.\n");
            close(file);
            exit(EXIT_FAILURE);
        }
    }

    if (nbytes < 0) {
        perror("[Error] Reading data.\n");
    }

    close(file);
}
