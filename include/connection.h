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

#include "utils.h"

#define TCP_PORT 21
#define BUFFER_SIZE 256


/**
 * @brief Creates a socket and connects it to the server that is specified in URL.
 * 
 * @return int Returns sockets file descriptor.
 */
int open_connection(const char* address, int port);

/**
 * @brief Creates connection used for control.
 * 
 * @param url Url struct that contains the url of the connection the needs to be opened.
 * @return int Returns sockets file descriptor.
 */
int open_control_connection(Url url);

/**
 * @brief Login in TCP connection.
 * 
 * @param sockfd 
 * @param url URL already parsed which may contain the credentials.
 * @return int Returns 0 when successfull.
 */
int login(int sockfd, Url url);

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
void get_file(int sockfd, Url url, int datafd);