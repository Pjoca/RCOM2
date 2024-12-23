#include "../include/connection.h"

#define CHECK_ALLOC(ptr) if ((ptr) == NULL) { perror("[CONSOLE] Allocating memory.\n"); exit(-1); }

int parse(char* url_content, URL* url) {
    // defining the regular expression
    const char *regex ="ftp://(([^/:].+):([^/:@].+)@)*([^/]+)/(.+)";

    // allocate memory for every component
    url->user = malloc(BUFFER_SIZE);
    CHECK_ALLOC(url->user);

    url->pass = malloc(BUFFER_SIZE);
    CHECK_ALLOC(url->pass);

    url->host = malloc(BUFFER_SIZE);
    CHECK_ALLOC(url->host);

    url->path = malloc(BUFFER_SIZE);
    CHECK_ALLOC(url->path);
  
    regex_t regex_comp;
    regmatch_t groups[MAX_GROUPS];

    // compilling the regular expression
    if (regcomp(&regex_comp, regex, REG_EXTENDED)) {
        fprintf(stderr, "[CONSOLE] Compiling the regex expression.\n");
        return 1;
    }

    // executing the regular expression
    if (regexec(&regex_comp, url_content, MAX_GROUPS, groups, 0)) {
        fprintf(stderr, "[CONSOLE] Executing the regex expression.\n");
        regfree(&regex_comp);
        return 1;
    }

    // extracts user and password in case it exists or if it's anonymous
    if (groups[2].rm_so != -1 && groups[3].rm_so != -1) {
        size_t user_len = groups[2].rm_eo - groups[2].rm_so;
        size_t pass_len = groups[3].rm_eo - groups[3].rm_so;
        strncpy(url->user, &url_content[groups[2].rm_so], user_len);
        strncpy(url->pass, &url_content[groups[3].rm_so], pass_len);
        url->user[user_len] = '\0';
        url->pass[pass_len] = '\0';
    } else {
        strncpy(url->user, "anonymous\0", 11);
        strncpy(url->pass, "anonymous\0", 11);
    }

    printf("[DEBUG] User: %s\n", url->user);
    printf("[DEBUG] Pass: %s\n", url->pass);

    if (groups[4].rm_so != -1 && groups[5].rm_so != -1) {
        size_t host_len = groups[4].rm_eo - groups[4].rm_so;
        size_t path_len = groups[5].rm_eo - groups[5].rm_so;
        strncpy(url->host, &url_content[groups[4].rm_so], host_len);
        strncpy(url->path, &url_content[groups[5].rm_so], path_len);
        url->host[host_len] = '\0';
        url->path[path_len] = '\0';
    }

    regfree(&regex_comp);
    return 0;
}

int open_connection(int *sockfd, URL url) {

    struct hostent* host = gethostbyname(url.host);
    if (host == NULL) {
        perror("[CONSOLE] Invalid host.\n");
        return 1;
    }
    
    // convert the resolved IP address
    const char* address = inet_ntoa(*(struct in_addr*)host->h_addr);
    struct sockaddr_in server_addr;

    // initialize addr struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(TCP_PORT);

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[CONSOLE] Opening the socket.\n");
        return 1;
    }
    printf("[DEBUG] Socket opened.\n");

    // connect to server
    if (connect(*sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CONSOLE] Connecting to the server.\n");
        close(*sockfd);
        return 1;
    }
    printf("[DEBUG] Connected to server.\n");

    return 0;
}

int login(int *sockfd, URL url) {
    char user[BUFFER_SIZE], pass[BUFFER_SIZE];
    char buffer[1024] = {0};

    // format the FTP commands for username and password
    snprintf(user, BUFFER_SIZE, "USER %s\r\n", url.user);
    snprintf(pass, BUFFER_SIZE, "PASS %s\r\n", url.pass);

    printf("[DEBUG] Sending USER command: %s", user);
    printf("[DEBUG] Sending PASS command: %s", pass);

    // send the username and read the response
    if (write(*sockfd, user, strlen(user)) < 0) {
        perror("[CONSOLE] Failed to send username.\n");
        return 1;
    }
    printf("[DEBUG] USER command sent.\n");

    if (read(*sockfd, buffer, 1024) < 0) {
        perror("[CONSOLE] Failed to read from socket after sending username.\n");
        return 1;
    }

    printf("[DEBUG] Response after USER command: %s\n", buffer);

    // validate the username response
    buffer[3] = '\0';
    if (strcmp(buffer, READY_PASS) != 0) {
        fprintf(stderr, "[CONSOLE] Invalid username: %s\n", url.user);
        return 1;
    }

    // send the password and read the response
    if (write(*sockfd, pass, strlen(pass)) < 0) {
        perror("[CONSOLE] Failed to send password:\n");
        close(*sockfd);
        return 1;
    }
    printf("[DEBUG] PASS command sent.\n");

    if (read(*sockfd, buffer, 1024) < 0) {
        perror("[CONSOLE] Failed to read from socket after sending password.\n");
        close(*sockfd);
        return 1;
    }

    printf("[DEBUG] Response after PASS command: %s\n", buffer);

    // validate the password response
    buffer[3] = '\0';
    if (strcmp(buffer, LOGIN_SUCCESS) != 0) {
        fprintf(stderr, "[CONSOLE] Invalid password: %s\n", url.user);
        return 1;
    }

    return 0;
}

int enter_passive_mode(int *sockfd, char* address) {
    char buffer[1024] = {0};

    // send the PASV command to the server  
    printf("[DEBUG] Sending PASV command\n");
    if (write(*sockfd, "PASV\r\n", strlen("PASV\r\n")) < 0) {
        perror("[CONSOLE] Failed to send PASV command.\n");
        exit(-1);
    }

    // read the server's response
    if (read(*sockfd, buffer, BUFFER_SIZE) < 0) {
        perror("[CONSOLE] Failed to read from socket after sending PASV command.\n");
        exit(-1);
    }
    printf("[DEBUG] Server response for PASV: %s\n", buffer);

    char code[4];
    strncpy(code, buffer, 3);
    code[3] = '\0';

    if (strcmp(code, PASSIVE_MODE_READY) != 0) {
        fprintf(stderr, "[CONSOLE] Unexpected PASV response: %s\n", buffer);
        exit(-1);
    }

    // parse the server's response for IP address and port
    char* start = strchr(buffer, '(');
    char* end = strchr(buffer, ')');
    if (!start || !end || start >= end) {
        fprintf(stderr, "[CONSOLE] Malformed PASV response: %s\n", buffer);
        exit(-1);
    }
    printf("[DEBUG] PASV response data: %s\n", start + 1);

    // extract the data between parentheses
    *end = '\0';  // Null-terminate at the closing parenthesis
    char* data = start + 1;

    // parse the IP address
    int i = 0;
    char* token = strtok(data, ",");
    while (token && i < 4) {
        strcat(address, token);
        if (i < 3) strcat(address, ".");
        token = strtok(NULL, ",");
        i++;
    }
    printf("[DEBUG] Parsed IP address: %s\n", address);

    // parse the port
    if (!token) {
        fprintf(stderr, "[CONSOLE] Incomplete PASV response: %s\n", buffer);
        exit(-1);
    }
    int portMSB = atoi(token);
    printf("[DEBUG] Parsed port MSB: %d\n", portMSB);

    token = strtok(NULL, ",");
    if (!token) {
        fprintf(stderr, "[CONSOLE] Missing port information in PASV response: %s\n", buffer);
        exit(-1);
    }
    int portLSB = atoi(token);
    printf("[DEBUG] Parsed port LSB: %d\n", portLSB);

    // calculate and return the port number
    int port = (portMSB << 8) | portLSB;
    printf("[DEBUG] Calculated port: %d\n", port);
    return port;
}

int get_file(int *sockfd, URL url, int *datafd) {
    char buffer[1024];

    // send RETR command
    printf("[DEBUG] Sending RETR command RETR: %s\r\n", url.path);

    snprintf(buffer, BUFFER_SIZE, "RETR %s\r\n", url.path);

    if (write(*sockfd, buffer, strlen(buffer)) < 0) {
        perror("[CONSOLE] Failed to send RETR command.\n");
        return 1;
    }

    // read the server's response
    if (read(*sockfd, buffer, BUFFER_SIZE) < 0) {
        perror("[CONSOLE] Failed to read from socket after sending command.\n");
        return 1;
    }
    printf("[DEBUG] Server response for RETR: %s\n", buffer);

    // extract the response code
    buffer[3] = '\0';
    if (strcmp(buffer, BIN_MODE) != 0 && strcmp(buffer, ALR_BIN_MODE) != 0) {
        fprintf(stderr, "[CONSOLE] Unexpected response: %s\n", buffer);
        return 1;
    }

    char *name = strrchr(url.path, '/');
    name = (name == NULL) ? url.path : name + 1;
    printf("[DEBUG] Output file name: %s\n", name);

    // open output file
    FILE *file = fopen(name, "w");
    if (file == NULL) {
        perror("[CONSOLE] Failed to open file for writing.\n");
        return 1;
    }
    printf("[DEBUG] File opened for writing: %s\n", name);

    // read from data socket and write to file
    ssize_t number_bytes;
    ssize_t total_bytes = 0;
    while ((number_bytes = read(*datafd, buffer, 1024)) > 0) {
        if (fwrite(buffer, number_bytes, 1, file) < 0) {
            perror("[CONSOLE] Writing file.\n");
            fclose(file);
            return 1;
        }
        total_bytes += number_bytes;
        printf("[DEBUG] %ld bytes written\n", number_bytes);
    }

    if (number_bytes < 0) {
        perror("[CONSOLE] Failed to read data.\n");
        fclose(file);
        return 1;
    }

    printf("[DEBUG] File written: %ld bytes\n", total_bytes);
    fclose(file);
    printf("[DEBUG] File closed.\n");

    close(*datafd);
    printf("[DEBUG] Data socket closed.\n");
    
    // read the server's response
    if (read(*sockfd, buffer, 1024) < 0) {
        perror("[CONSOLE] Failed to read from socket.\n");
        close(*sockfd);
        return 1;
    }

    printf("[DEBUG] Response after command: %s\n", buffer);

    // validate the password response
    buffer[3] = '\0';
    if (strcmp(buffer, TRANSFER_COMPLETE) != 0) {
        fprintf(stderr, "[CONSOLE] Failed to receive transfer complete: %s\n", url.user);
        return 1;
    }

    return 0;
}

int server_close(int *sockfd) {
    char buffer[1024] = {0};

    // send the PASV command to the server
    printf("[DEBUG] Sending QUIT command\n");
    if (write(*sockfd, "QUIT\r\n", strlen("QUIT\r\n")) < 0) {
        perror("[CONSOLE] Failed to send QUIT command.\n");
        return 1;
    }
    printf("[DEBUG] QUIT command sent.\n");

    // read the server's response
    if (read(*sockfd, buffer, BUFFER_SIZE) < 0) {
        perror("[CONSOLE] Failed to read from socket after sending QUIT command.\n");
        return 1;
    }
    printf("[DEBUG] Server response for QUIT: %s\n", buffer);

    // extract the response code
    buffer[3] = '\0';
    if (strcmp(buffer, GOODBYE) != 0) {
        fprintf(stderr, "[CONSOLE] Unexpected QUIT response: %s\n", buffer);
        return 1;
    }

    return 0;
}