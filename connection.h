/**
 * @file connection.h
 * @brief Describes functions that work with tcp connections.
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
 * @brief Creates a socket and connects to the server specified in url.
 * 
 * @return int Returns the socket's file descriptor.
 */
int openConnection(const char* address, int port);

/**
 * @brief Creates a connection to be used for control.
 * 
 * @param url Url struct containing the url of the connection to be opened.
 * @return int Returns the socket's file descriptor.
 */
int openControlConnection(Url url);

/**
 * @brief Login into the tcp connection.
 * 
 * @param sockfd 
 * @param url Previously parsed url that contains the credentials (or not).
 * @return int Returns 0 on success.
 */
int login(int sockfd, Url url);

/**
 * @brief Enter passive mode.
 * 
 * @param sockfd Socket file descriptor.
 * @param address String to be filled with the address given by the server.
 * @return int Port number where server awaits connection.
 */
int enterPassiveMode(int sockfd, char* address);

/**
 * @brief Get the File from the data connection.
 * 
 * @param sockfd Control socket file descriptor.
 * @param url Parsed url struct.
 * @param datafd Data socket file descriptor.
 */
void getFile(int sockfd, Url url, int datafd);