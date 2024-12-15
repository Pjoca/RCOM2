#include "../include/utils.h"

#define CHECK_ALLOC(ptr) if ((ptr) == NULL) { perror("[Error] Allocating memory"); exit(EXIT_FAILURE); }

int reg_group_copy(char* field, char* url_str, regmatch_t reg) {
    // no match
    if (reg.rm_so == -1) return 1;

    // calculate match length
    int dist = reg.rm_eo - reg.rm_so;

    // copy the matched substring
    memcpy(field, &url_str[reg.rm_so], dist);
    field[dist] = '\0';
    return 0;
}

int parse_url(char* url_str, Url* url) {
    // defining the regular expression
    const char *regex ="ftp://(([^/:].+):([^/:@].+)@)*([^/]+)/(.+)";

    // allocate memory for every component
    url->user = malloc(BUFFER_SIZE);
    url->password = malloc(BUFFER_SIZE);
    url->host = malloc(BUFFER_SIZE);
    url->path = malloc(BUFFER_SIZE);
  
    regex_t regex_comp;
    regmatch_t groups[MAX_GROUPS];

    // compiling the regular expression
    if (regcomp(&regex_comp, regex, REG_EXTENDED)) {
        fprintf(stderr, "[Error] Compiling the regex expression.\n");
        return 1;
    }

    // executing the regular expression
    if (regexec(&regex_comp, url_str, MAX_GROUPS, groups, 0)) {
        fprintf(stderr, "[Error] Executing the regex expression.\n");
        regfree(&regex_comp);
        return 1;
    }

    // extracts user and password in case it existsor if it's anonymous
    if (groups[2].rm_so != -1) {
        if (reg_group_copy(url->user, url_str, groups[2]) || reg_group_copy(url->password, url_str, groups[3])) {
            regfree(&regex_comp);
            return 1;
        }
    } else {
        strncpy(url->user, "anonymous", BUFFER_SIZE);
        strncpy(url->password, "", BUFFER_SIZE);
    }

    // extracts the host and the path of the url provided
    if (reg_group_copy(url->host, url_str, groups[4]) || reg_group_copy(url->path, url_str, groups[5])) {
        regfree(&regex_comp);
        return 1;
    }

    regfree(&regex_comp);
    return 0;
}

char* get_file_name(char* path) {
    char *name = strrchr(path, '/');
    if (name == NULL) return path;
    return name + 1;
}

int get_socket_line(int sockfd, char* line) {
    FILE* socket = fdopen(sockfd, "r");
    if (!socket) {
        perror("[Error] Opening the socket as a file.\n");
        return -1;
    }

    char* buf = NULL;
    size_t size = 0;
    ssize_t nbytes = getline(&buf, &size, socket);

    if (nbytes < 0) {
        free(buf);
        return -1;
    }

    strncpy(line, buf, nbytes);
    line[nbytes] = '\0';
    free(buf);
    return 0;
}

int read_code(int sockfd, char* expected) {
    // allocate memory for the buffer
    char* buf = (char*)(malloc(BUFFER_SIZE));
    
    // gets socket line
    if (get_socket_line(sockfd, buf) != 0) {
        perror("[Error] Opening the connection.\n");
        exit(EXIT_FAILURE);
    }
    buf[3] = '\0';

    // compare status code with the expected code
    int res = (strcmp(buf, expected) == 0);
    free(buf);
    return res;
}

void clean_socket(int sockfd) {
    // check if data is available in the socket
    int count;
    if (ioctl(sockfd, FIONREAD, &count) < 0 || count <= 0) return;

    // open socket as a file
    FILE* socket = fdopen(sockfd, "r");
    if (!socket) {
        perror("[Error] Opening the socket as a file.\n");
        return;
    }

    char* buf = NULL;
    size_t size = 0;

    // read line from the socket
    while (getline(&buf, &size, socket) > 0) {
        if (buf[3] != '-') break;
    }

    // free resources and close socket
    free(buf);
    fclose(socket);
}