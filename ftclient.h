/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Program: ftclient.h
 * Author: Nathan Cochran
 * Date: 11/17/2013
 * Description: Header file for ftclient.c
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include "ftutil.h"

//Function Prototypes:
void control_connect(int ctrl_fd, char *host);
void receive_message(int ctrl_fd);
void make_request(int ctrl_fd, char *request);
void get_request(int ctrl_fd, char *response);
int open_data_connection(int ctrl_fd);
void receive_listing(int data_fd);
void receive_file(int data_fd, char *filename);
void signal_handler(int sig);
void install_signal_handlers(void);

