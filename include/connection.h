/**
 * @file connection.h
 * @brief Describes the functions interacting with the TCP connections.
 * 
 */

#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 256
#define MAX_GROUPS 6
#define TCP_PORT 21
#define BUFFER_SIZE 256

#define SERVER_READY "220"
#define READY_PASS "331"
#define LOGIN_SUCCESS "230"
#define PASSIVE_MODE_READY "227"

/**
 * @brief Describes an URL struct. 
 * This is used to store an user given url after it's parsed.
 * 
 */
typedef struct URL {
    char* user; 
    char* pass; 
    char* host;
    char* path;
} URL;

/**
 * @brief Fill url field from input using regex.
 * 
 * @param dest Destination field.
 * @param url_content Original url in string format.
 * @param reg Final result in regex format.
 * @return int Returns 0 when successful and 1 otherwise.
 */
int parse(char* url_content, URL* url);

/**
 * @brief Get the socket line object
 * 
 * @param socket_fd Socket descriptor.
 * @param line Line to be read.
 * @return int Returns 0 when successful and -1 otherwise.
 */
int get_socket_line(int socket_fd, char* line);
/**
 * @brief Creates connection used for control.
 * 
 * @param url Url struct that contains the url of the connection the needs to be opened.
 * @return int Returns sockets file descriptor.
 */
int open_connection(URL url);

/**
 * @brief Login in TCP connection.
 * 
 * @param sockfd 
 * @param url URL already parsed which may contain the credentials.
 * @return int Returns 0 when successfull.
 */
int login(int sockfd, URL url);

/**
 * @brief Entering the passive mode.
 * 
 * @param sockfd Socket descriptor.
 * @param address Will be filled with the address provided by the server.
 * @return int Port number where the server is waiting for a connection.
 */
int enter_passive_mode(int sockfd, char* address);

/**
 * @brief Get file from data connection.
 * 
 * @param sockfd Socket file descriptor.
 * @param url URL already parsed.
 * @param datafd Data socket descriptor.
 */
void get_file(int sockfd, URL url, int datafd);