/**
 * @file utils.h
 * @brief Utility functions description.
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
 * @brief Fill url field from input using regex.
 * 
 * @param field Destination field.
 * @param url_str Original url in string format.
 * @param reg Final result in regex format.
 * @return int Returns 0 when successfull and 1 otherwise.
 */
int reg_group_copy(char* field, char* url_str, regmatch_t reg);

/**
 * @brief Parse FTP URL in string format and extract its components.
 * 
 * @param url_str Original url in string format.
 * @param url Url structure to store its fields.
 * @return int Returns 0 when successfull and 1 otherwise.
 */
int parse_url(char* url_str, Url* url);

/**
 * @brief Get filename from path.
 * 
 * @param path Path of the specified file.
 * @return char* Name of the file.
 */
char* get_file_name(char* path);

/**
 * @brief Reads line from socket.
 * 
 * @param socket Socket descriptor.
 * @param buf Store the line that was read.
 * @return Returns 0 when successfull and -1 otherwise.
 */
int get_socket_line(int sockfd, char* line);

/**
 * @brief Verifies the connection and returns the correct code.
 * 
 * @param socket Socket descriptor.
 * @param expected The status code that is expected.
 * @return int Retunrs 1 if code correct, -1 if error was found and 0 in the remaining scenarios.
 */
int read_code(int sockfd, char* expected);

/**
 * @brief Reads complete socket.
 * 
 * @param socket Socket descriptor.
 */
void clean_socket(int sockfd);