#include "ftutil.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Sends a message over the specified socket
 * Param:   int socket_fd -  File descriptor of connection to send message over
 * Param:   char * message -  Message to send
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void send_message(int socket_fd, char * message) {
    int length = strlen(message);

    if(write(socket_fd, message, length) != length) {
        perror("Error writing message");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }   
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Creates a new IPv4 TCP socket
 * Param:   void
 * Return:  int -  File descriptor of the newly created socket
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int create_socket(void) {
    int socket_fd;

    //Create socket:
    if((socket_fd = socket(AF_INET, SOCK_STREAM, PROTOCOL)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Binds the socket to the specified port.  Used in creating a passive socket
 * Param:   int socket_fd -  File descriptor of the socket to bind
 * Param:   unsigned short port -  The port number to bind the socket to
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Makes the specified socket listen for incoming connections
 * Param:   int socket_fd -  Socket to listen for connections
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void listen_socket(int socket_fd) {

    if(listen(socket_fd, BACKLOG) == -1) {
        perror("Error listening for incoming connections");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parses the client's command from the buffer, and returns the appropriate command type identifier
 * Param:   char * buffer -  Buffer containing the user's raw command
 * Param:   char * arg -  Buffer to return any arguments passed in with the command
 * Return:  int -  The command type identifier
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

    else if(strncmp(buffer, "pwd ", 4) == 0 ||
            strncmp(buffer, "pwd\t", 4) == 0 ||
            strncmp(buffer, "pwd\n", 4) == 0) {
        buffer = buffer + 3;
        command = PWD;   
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Prompts the user for a yes/no answer.  Returns 1 for yes, 0 for no.
 * Param:   char * prompt -  The prompt to display
 * Return:  int -  1 for yes, 0 for no
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
