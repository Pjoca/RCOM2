/**
 * @file utils.h
 * @brief Describes utility functions such as I/O handling.
 * 
 */

#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 256
#define MAX_GROUPS 6

/**
 * @brief Describes a Url struct. 
 * This is used to store a user given url after it's parsed.
 * 
 */
typedef struct Url {
    char* user; 
    char* password; 
    char* host;
    char* path;
} Url;

/**
 * @brief Fill a Url field from a regex result.
 * 
 * @param field Destination.
 * @param url_str Original url string.
 * @param reg Regex result.
 * @return int Returns 0 upon success, 1 otherwise.
 */
int regGroupCopy(char* field, char* url_str, regmatch_t reg);

/**
 * @brief Parse a url and store the individual fields.
 * 
 * @param url_str Original url string.
 * @param url Url struct to store different fields.
 * @return int Returns 0 upon success, 1 otherwise.
 */
int parseUrl(char* url_str, Url* url);

/**
 * @brief Get a file name from a file path.
 * 
 * @param path File path.
 * @return char* File name.
 */
char* getFileName(char* path);

/**
 * @brief Read a line from a socket.
 * 
 * @param socket Socket file descriptor.
 * @param buf Pointer to store the line read.
 * @return Returns 0 upon success, -1 otherwise.
 */
int getSocketLine(int sockfd, char* line);

/**
 * @brief Verify is the connection return the right code.
 * 
 * @param socket Socket file descriptor.
 * @param expected Expected status code, e.g. "220".
 * @return int 1 if the code is correct, -1 for error, and 0 otherwise
 */
int readCode(int sockfd, char* expected);

/**
 * @brief Read a socket until the end.
 * 
 * @param socket Socket file descriptor.
 */
void cleanSocket(int sockfd);