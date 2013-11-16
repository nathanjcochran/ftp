#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "ftutil.h"

void send_message(int socket_fd, char * message) {
    int length = strlen(message);

    if(write(socket_fd, message, length) != length) {
        perror("Error writing message");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }   
}


int create_socket(void) {
    int socket_fd;

    //Create socket:
    if((socket_fd = socket(AF_INET, SOCK_STREAM, PROTOCOL)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

void bind_socket(int socket_fd, unsigned short port) {
    struct sockaddr_in address;

    //Create address:
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //Bind address to socket:
    if(bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error binding socket to address");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}

void listen_socket(int socket_fd) {

    if(listen(socket_fd, BACKLOG) == -1) {
        perror("Error listening for incoming connections");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}


int accept_connection(int socket_fd) {
    struct sockaddr_in address;
    char address_str[BUF_SIZE];
    int connection_fd;
    unsigned int length;

    //Accept a connection:
    length = sizeof(address);
    if((connection_fd = accept(socket_fd, (struct sockaddr *) &address, &length)) == -1) {
        perror("Error accepting incoming connection");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    //Get the peer's address as a string:
    if(inet_ntop(AF_INET, &address.sin_addr, address_str, BUF_SIZE) == NULL) {
        perror("Error converting ip address to string");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    //Print message:
    printf("Connection accepted: %s\n", address_str);

    return connection_fd;
}


int parse_command(char * buffer, char * arg) {
    int command = INVALID;
    
    //Skip over whitespace:
    while(*buffer == ' ' || *buffer == '\t') {
        buffer++;
    }

    //Determine the command given:
    if(strncmp(buffer, "list ", 5) == 0 ||
        strncmp(buffer, "list\t", 5) == 0 ||
        strncmp(buffer, "list\n", 5) == 0) {
        buffer = buffer + 4;
        command = LIST;
    }   

    else if(strncmp(buffer, "get ", 4) == 0 ||
            strncmp(buffer, "get\t", 4) == 0 ||
            strncmp(buffer, "get\n", 4) == 0) {
        buffer = buffer + 3;
        command = GET;
    }   

    else if(strncmp(buffer, "cd ", 3) == 0 ||
            strncmp(buffer, "cd\t", 3) == 0 ||
            strncmp(buffer, "cd\n", 3) == 0) {
        buffer = buffer + 2;
        command = CD;   
    }   

    else if(strncmp(buffer, "exit ", 5) == 0 ||
            strncmp(buffer, "exit\t", 5) == 0 ||
            strncmp(buffer, "exit\n", 5) == 0) {
        buffer = buffer + 4;
        command = EXIT;
    }

    //Get the argument given:
    if(arg != NULL) {

        //Skip over whitespace:
        while(*buffer == ' ' || *buffer == '\t') {
            buffer++;
        }

        //Copy the argument over:
        while((*buffer != ' ') && (*buffer != '\n') && (*buffer != '\r') && (*buffer != '\0')) {
            *arg = *buffer;
            arg++;
            buffer++;
        }

        //Null-terminate:
        *arg = '\0';
    }

    return command;
}

int input_yn(char * prompt) {
    char buf[BUF_SIZE];

    printf("%s", prompt);

    while(1) {
        fgets(buf, BUF_SIZE, stdin);
        
        if(strncmp(buf, "yes\n", 4) == 0 ||
            strncmp(buf, "yes ", 4) == 0 ||
            strncmp(buf, "y\n", 2) == 0 || 
            strncmp(buf, "y ", 2) == 0) {
            return 1;
        }

        if(strncmp(buf, "no\n", 3) == 0 ||
            strncmp(buf, "no ", 3) == 0 ||
             strncmp(buf, "n\n", 2) == 0 ||
             strncmp(buf, "n ", 2) == 0) {
            return 0;
        }

        printf("Invalid input.  Please try again.\n");
        printf("%s", prompt);
    }

    return -1;
}
