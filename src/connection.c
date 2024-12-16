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
        url->user = "anonymous";
        url->pass = "anonymous";
    }

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

int get_socket_line(int socket_fd, char* line) {
    FILE* socket = fdopen(socket_fd, "r");
    if (!socket) {
        perror("[CONSOLE] Opening the socket as a file.\n");
        return -1;
    }

    char* buffer = NULL;
    size_t size = 0;
    ssize_t number_bytes = getline(&buffer, &size, socket);

    if (number_bytes < 0) {
        free(buffer);
        return -1;
    }

    strncpy(line, buffer, number_bytes);
    line[number_bytes] = '\0';
    free(buffer);
    return 0;
}

int open_connection(URL url) {

    struct hostent* host = gethostbyname(url.host);
    if (host == NULL) {
        perror("[CONSOLE] Invalid host.\n");
        exit(-1);
    }
    
    // convert the resolved IP address
    const char* address = inet_ntoa(*(struct in_addr*)host->h_addr);
    struct sockaddr_in server_addr;

    // initialize addr struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(TCP_PORT);

    // create socket
    int socket_fd;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[CONSOLE] Opening the socket.\n");
        exit(-1);
    }

    // connect to server
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CONSOLE] Connecting to the server.\n");
        close(socket_fd);
        exit(-1);
    }

    // verify server's initial response code
    char buffer[BUFFER_SIZE] = {0};
    if (get_socket_line(socket_fd, buffer) != 0) {
        perror("[CONSOLE] Failed to read from socket.\n");
        close(socket_fd);
        exit(-1);
    }

    buffer[3] = '\0';

    if (strcmp(buffer, SERVER_READY) != 0) {
        perror("[CONSOLE] Unexpected response from server.\n");
        close(socket_fd);
        exit(-1);
    }

    int count;
    if (ioctl(socket_fd, FIONREAD, &count) == 0 && count > 0)  {
        char buffer[1024]; // Temporary buffer for reading socket data
        ssize_t bytes_read;

        while ((bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the string
            if (bytes_read >= 4 && buffer[3] != '-') break;
        }

        if (bytes_read < 0) {
            perror("[CONSOLE] Error reading from socket.\n");
        };
    }

    return socket_fd;
}

int login(int socket_fd, URL url) {
    char user[BUFFER_SIZE], pass[BUFFER_SIZE], code[BUFFER_SIZE];

    // Format the FTP commands for username and password
    snprintf(user, BUFFER_SIZE, "user %s\n", url.user);
    snprintf(pass, BUFFER_SIZE, "pass %s\n", url.pass);

    int count;
    if (ioctl(socket_fd, FIONREAD, &count) == 0 && count > 0)  {
        char buffer[1024]; // Temporary buffer for reading socket data
        ssize_t bytes_read;

        while ((bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the string
            if (bytes_read >= 4 && buffer[3] != '-') break;
        }

        if (bytes_read < 0) {
            perror("[CONSOLE] Error reading from socket.\n");
        };
    }

    // Send the username and read the response
    if (write(socket_fd, user, strlen(user)) < 0 || get_socket_line(socket_fd, code) != 0) {
        perror("[CONSOLE] Failed to send username or read response.\n");
        exit(-1);
    }

    // Extract the response code
    code[3] = '\0';

    // Validate the username response
    if (strcmp(code, READY_PASS) != 0 && strcmp(code, LOGIN_SUCCESS) != 0) {
        fprintf(stderr, "[CONSOLE] Invalid username: %s\n", url.user);
        exit(-1);
    }

    // If password is required, send it and validate the response
    if (strcmp(code, READY_PASS) == 0) {
        write(socket_fd, pass, strlen(pass));
        char buffer[BUFFER_SIZE] = {0};
        if (get_socket_line(socket_fd, buffer) != 0) {
            perror("Failed to read from socket.\n");
            close(socket_fd);
            exit(-1);
        }

        buffer[3] = '\0';
        
        if (strcmp(buffer, LOGIN_SUCCESS) != 0) {
            fprintf(stderr, "[CONSOLE] Invalid password: %s\n", url.pass);
            close(socket_fd);
            exit(-1);
        }
    }

    return 0;
}

int enter_passive_mode(int socket_fd, char* address) {
    char buffer[BUFFER_SIZE];
    char code[4]; 

    // Send the PASV command to the server
    const char* pasv_command = "pasv\n";
    if (write(socket_fd, pasv_command, strlen(pasv_command)) < 0) {
        perror("[CONSOLE] Failed to send PASV command.\n");
        exit(-1);
    }

    // Read the server's response
    if (get_socket_line(socket_fd, buffer) != 0) {
        perror("[CONSOLE] Failed to read server response for PASV.\n");
        exit(-1);
    }

    // Extract and validate the response code
    strncpy(code, buffer, 3);
    code[3] = '\0';  // Null-terminate the code string
    if (strcmp(code, PASSIVE_MODE_READY) != 0) {
        fprintf(stderr, "[CONSOLE] Unexpected PASV response: %s\n", buffer);
        exit(-1);
    }

    // Parse the server's response for IP address and port
    char* start = strchr(buffer, '(');
    char* end = strchr(buffer, ')');
    if (!start || !end || start >= end) {
        fprintf(stderr, "[CONSOLE] Malformed PASV response: %s\n", buffer);
        exit(-1);
    }

    // Extract the data between parentheses
    *end = '\0';  // Null-terminate at the closing parenthesis
    char* data = start + 1;

    // Parse the IP address
    int i = 0;
    char* token = strtok(data, ",");
    while (token && i < 4) {
        strcat(address, token);
        if (i < 3) strcat(address, ".");
        token = strtok(NULL, ",");
        i++;
    }

    // Parse the port
    if (!token) {
        fprintf(stderr, "[CONSOLE] Incomplete PASV response: %s\n", buffer);
        exit(-1);
    }
    int portMSB = atoi(token);

    token = strtok(NULL, ",");
    if (!token) {
        fprintf(stderr, "[CONSOLE] Missing port information in PASV response: %s\n", buffer);
        exit(-1);
    }
    int portLSB = atoi(token);

    // Calculate and return the port number
    return (portMSB << 8) | portLSB;
}

void get_file(int socket_fd, URL url, int datafd) {
    char buffer[1024];

    // send retr command
    const char retr[] = "retr ";
    write(socket_fd, retr, strlen(retr));
    write(socket_fd, url.path, strlen(url.path));
    write(socket_fd, "\n", 1);

    char *name = strrchr(url.path, '/');
    name = (name == NULL) ? url.path : name+1;
    // open output file
    int file = open(name, O_WRONLY | O_CREAT, 0777);
    if (file == -1) {
        perror("[CONSOLE] Creating file.\n");
        exit(-1);
    }

    // read from data socket and write to file
    ssize_t number_bytes; 
    while ((number_bytes = read(datafd, buffer, sizeof(buffer))) > 0) {
        if (write(file, buffer, number_bytes) != number_bytes) {
            perror("[CONSOLE] Writing file.\n");
            close(file);
            exit(-1);
        }
    }

    if (number_bytes < 0) {
        perror("[CONSOLE] Reading data.\n");
    }

    close(file);
}
