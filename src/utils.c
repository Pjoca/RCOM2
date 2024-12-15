#include "../include/utils.h"

#define CHECK_ALLOC(ptr) if ((ptr) == NULL) { perror("Memory allocation failed"); exit(EXIT_FAILURE); }

int regGroupCopy(char* field, char* url_str, regmatch_t reg) {
    if (reg.rm_so == -1) return 1;
    int dist = reg.rm_eo - reg.rm_so;
    memcpy(field, &url_str[reg.rm_so], dist);
    field[dist] = '\0';
    return 0;
}

int parseUrl(char* url_str, Url* url) {
    const char *regex ="ftp://(([^/:].+):([^/:@].+)@)*([^/]+)/(.+)";
    url->user = malloc(BUFFER_SIZE);
    url->password = malloc(BUFFER_SIZE);
    url->host = malloc(BUFFER_SIZE);
    url->path = malloc(BUFFER_SIZE);
  
    regex_t regex_comp;
    regmatch_t groups[MAX_GROUPS];

    if (regcomp(&regex_comp, regex, REG_EXTENDED)) {
        fprintf(stderr, "Failed to compile regex\n");
        return 1;
    }

    if (regexec(&regex_comp, url_str, MAX_GROUPS, groups, 0)) {
        fprintf(stderr, "Regex execution failed\n");
        regfree(&regex_comp);
        return 1;
    }

    if (groups[2].rm_so != -1) {
        if (regGroupCopy(url->user, url_str, groups[2]) || regGroupCopy(url->password, url_str, groups[3])) {
            regfree(&regex_comp);
            return 1;
        }
    } else {
        strncpy(url->user, "anonymous", BUFFER_SIZE);
        strncpy(url->password, "", BUFFER_SIZE);
    }


    if (regGroupCopy(url->host, url_str, groups[4]) || regGroupCopy(url->path, url_str, groups[5])) {
        regfree(&regex_comp);
        return 1;
    }

    regfree(&regex_comp);
    return 0;
}

char* getFileName(char* path) {
    char *name = strrchr(path, '/');
    if (name == NULL) return path;
    return name + 1;
}

int getSocketLine(int sockfd, char* line) {
    FILE* socket = fdopen(sockfd, "r");
    if (!socket) {
        perror("Failed to open socket as FILE");
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

int readCode(int sockfd, char* expected) {
    char* buf = (char*)(malloc(BUFFER_SIZE));
    
    if (getSocketLine(sockfd, buf) != 0) {
        perror("Opening connection failed.\n");
        exit(EXIT_FAILURE);
    }
    buf[3] = '\0';
    int res = (strcmp(buf, expected) == 0);
    free(buf);
    return res;
}

void cleanSocket(int sockfd) {
    int count;
    if (ioctl(sockfd, FIONREAD, &count) < 0 || count <= 0) return;

    FILE* socket = fdopen(sockfd, "r");
    if (!socket) {
        perror("Failed to open socket as FILE");
        return;
    }

    char* buf = NULL;
    size_t size = 0;

    while (getline(&buf, &size, socket) > 0) {
        if (buf[3] != '-') break;
    }

    free(buf);
    fclose(socket);
}