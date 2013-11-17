#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ftutil.h"

//Function Prototypes:
int start_server(void);
void handle_request(int ctrl_fd);
int get_command(int ctrl_fd, char *arg);
void list_directories(int ctrl_fd);
int data_connect(int ctrl_fd);
void send_file(int ctrl_fd, char *arg);
void change_directory(int ctrl_fd, char * directory);
void show_cwd(int ctrl_fd);
void signal_handler(int signal);
void install_sigint_handler(void);

