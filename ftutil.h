#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#ifndef FTUTIL_H
#define FTUTIL_H

//CONSTANTS:

#define PROTOCOL 0
#define CONTROL_PORT 30021
#define CONTROL_PORT_STR "30021"
#define DATA_PORT 30020
#define BACKLOG 5
#define BUF_SIZE 256
#define FILE_BUF_SIZE 4096
#define PROMPT ">>"


//COMMAND TYPE IDENTIFIERS:

#define INVALID -1
#define EXIT 0
#define LIST 1
#define GET 2
#define CD 3
#define PWD 4


//FUNCTION PROTOTYPES:

void send_message(int socket_fd, char *message);
int create_socket(void);
void bind_socket(int socket_fd, unsigned short port);
void listen_socket(int socket_fd);
int accept_connection(int socket_fd);
int parse_command(char * buffer, char * arg);
int input_yn(char * prompt);

#endif
