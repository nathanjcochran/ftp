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
#include "ftutil.h"

#define GREETING "Welcome to Nathan's File Transfer Program\nCommands:\n\tlist - view files in current directory\n\tget <filename> - get the specified file\n\tcd <directory> - change directory\n\tpwd - print working directory\n"

//Static Variables:
int server_fd, control_fd;

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

int main(int argc, char * argv[]) {

    //Install signal handlers:
    install_sigint_handler();

    //Start the server:
    server_fd = start_server();

    //Iteratively handle connections:
    while(1) {

        //Accept a connection:
        control_fd = accept_connection(server_fd);
        
        //Handle it:
        handle_request(control_fd);

        //Close it:
        close(control_fd);
        control_fd = -1;
        printf("Connection closed by client\n");
    }

    close(server_fd);    
    return EXIT_SUCCESS;
}


int start_server(void) {
    int fd;

    fd = create_socket();
    bind_socket(fd, CONTROL_PORT);
    listen_socket(fd);

    printf("Server started\n");
    printf("Listening for incoming connections...\n");

    return fd;
}


void handle_request(int ctrl_fd) {
    int command;
    char arg[BUF_SIZE];

    //Display greeting and instructions:
    send_message(ctrl_fd, GREETING);

    //Get user's command choice:
    while((command = get_command(ctrl_fd, arg)) != EXIT) {

        //Perform appropriate response:
        switch(command) {
            case INVALID:
                send_message(ctrl_fd, "Invalid command\n");
                break;

            case LIST:
                list_directories(ctrl_fd);
                break;

            case GET:
                send_file(ctrl_fd, arg);
                break;

            case CD:
                change_directory(ctrl_fd, arg);
                break;

            case PWD:
                show_cwd(ctrl_fd);
                break;

        }
    }
}

int get_command(int ctrl_fd, char * arg) {
    int i;
    char buffer[BUF_SIZE];

    //Read the command from the socket:
    for(i=0; i<BUF_SIZE; i++) {

        if(i==0) {
            send_message(ctrl_fd, PROMPT);
        }

        //Read a character:
        if(read(ctrl_fd, &buffer[i], 1) == -1) {
            perror("Error reading from socket");
            exit(EXIT_FAILURE);
        };
        
        //End of line:
        if((buffer[i] == '\n') || (buffer[i] == '\r')) {
            if(i==0) {
                i--;
                continue;
            }
            else {
                break;
            }
        }
    }

    return parse_command(buffer, arg);
}

void list_directories(int ctrl_fd) {
    DIR * directory;
    struct dirent * entry;
    int data_fd;

    //Open data connection:
    data_fd = data_connect(ctrl_fd);

    if((directory = opendir("./")) == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    
    while((entry = readdir(directory)) != NULL) {
        if((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
            send_message(ctrl_fd, entry->d_name);
            send_message(ctrl_fd, "  ");
        }
    }
    send_message(ctrl_fd, "\n");

    closedir(directory);
    close(data_fd);
}


int data_connect(int ctrl_fd) {
    struct sockaddr_in address;
    int data_fd;
    unsigned int length;

    //Get peer's address:
    length = sizeof(address);
    if(getpeername(ctrl_fd, (struct sockaddr *) &address, &length) == -1) {
        perror("Error getting peer's address");
        close(ctrl_fd);
        exit(EXIT_FAILURE);
    }
    
    //Change to the data port:
    address.sin_port = htons(DATA_PORT);

    //Create a new socket:
    data_fd = create_socket();

    //Connect to peer via that socket:
    if(connect(data_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error opening data connection");
        close(ctrl_fd);
        exit(EXIT_FAILURE);
    }
    
    return data_fd;
}

void send_file(int ctrl_fd, char * arg) {
    int data_fd, file_fd, num_read;
    char c;

    //Open data connection:
    data_fd = data_connect(ctrl_fd);

    //Open the specified file:
    if((file_fd = open(arg, O_RDONLY)) == -1) {
        if(errno == ENOENT) {
            send_message(ctrl_fd, "Invalid filename: file does not exist\n");
            close(data_fd);
            return;
        }
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    //Transfer file:
    while((num_read = read(file_fd, &c, 1)) > 0) {
        if(write(data_fd, &c, 1) != 1) {
            perror("Error writing to data socket");
            exit(EXIT_FAILURE);
        }
    }
    if(num_read == -1) {
        perror("Error reading from file");
        exit(EXIT_FAILURE);
    }

    close(file_fd);
    close(data_fd);
}

void change_directory(int ctrl_fd, char * directory) {

    if(chdir(directory) == -1) {
        if(errno == EACCES) {
            send_message(ctrl_fd, "Error: permission denied\n");
        }
        else if(errno == ENOTDIR || errno == ENOENT) {
            send_message(ctrl_fd, "Error: invalid directory\n");
        }
        else {
            send_message(ctrl_fd, "Error: could not change directories\n");
        }
    }
    else {
        show_cwd(ctrl_fd);
    }
}

void show_cwd(int ctrl_fd) {
    char buf[BUF_SIZE];

    if(getcwd(buf, BUF_SIZE) == NULL) {
        perror("Error getting the current working directory");
        close(ctrl_fd);
        exit(EXIT_FAILURE);
    }
    send_message(ctrl_fd, "Remote working directory: ");
    send_message(ctrl_fd, buf);
    send_message(ctrl_fd, "\n");
}


void signal_handler(int signal) {
    if(control_fd != -1) {
        printf("Closing client connection...\n");
        send_message(control_fd, "Server closed connection.\n");

        close(control_fd);
        control_fd = -1;
        printf("Client connection closed\n");
    }

    close(server_fd);
    printf("Server shut down\n");
    exit(EXIT_SUCCESS);
}


void install_sigint_handler(void) {
    struct sigaction sig;

    sig.sa_handler = signal_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;

    sigaction(SIGINT, &sig, NULL);
    sigaction(SIGTERM, &sig, NULL);
}
