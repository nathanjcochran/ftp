#include "ftserve.h"

//Static Variables:
int server_fd, control_fd;

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


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Creates a passive socket that listens on the control port
 * Param:   void
 * Return:  int -  File descriptor of the passive socket
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int start_server(void) {
    int fd;

    fd = create_socket();
    bind_socket(fd, CONTROL_PORT);
    listen_socket(fd);

    printf("Server started\n");
    printf("Listening for incoming connections...\n");

    return fd;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Accepts an incoming connection on the control port
 * Param:   int socket_fd -  File descriptor of the passive, listening socket
 * Return:  int -  File descriptor for the newly initiated control connection
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Handles a complete client session
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void handle_request(int ctrl_fd) {
    int command;
    char arg[BUF_SIZE];

    //Display greeting and instructions:
    send_message(ctrl_fd, "Welcome to Nathan's File Transfer Program\nCommands:\n\t");
    send_message(ctrl_fd, "pwd\t- print working directory\n\t");
    send_message(ctrl_fd, "list\t- view files in current directory\n\t");
    send_message(ctrl_fd, "cd <directory>\t- change directory\n\t");
    send_message(ctrl_fd, "get <filename>\t- get the specified file\n");

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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Reads a single user's command from the control socket, and returns the command type
 * Param:   int ctrl_fd -  File descriptor of the control socket
 * Param:   char * arg -  Buffer to store any arguments sent with the command
 * Return:  int -  Command type identifier
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Creates a data connection with a client and sends a list of all files in the current directory
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Initiates a data connection with a listening client
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Return:  int -  File descriptor of the newly initiated data connection
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Creates a data connection with a client and sends the specified file across it
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Param:   char * arg -  Name of the file to send
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void send_file(int ctrl_fd, char * filename) {
    int data_fd, file_fd, num_read;
    char c;

    //Open data connection:
    data_fd = data_connect(ctrl_fd);

    //Open the specified file:
    if((file_fd = open(filename, O_RDONLY)) == -1) {
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Changes the current working directory, and informs client of new location
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Param:   char * directory -  Name of the target directory
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Shows the client the current working directory
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Signal handler for the sigint and sigterm signals.  Cleans up and says goodbye.
 * Param:   int signal -  The signal received
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void signal_handler(int sig) {
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


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Installs the signal handlers for the sigint and sigterm signals
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void install_sigint_handler(void) {
    struct sigaction sig;

    sig.sa_handler = signal_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;

    sigaction(SIGINT, &sig, NULL);
    sigaction(SIGTERM, &sig, NULL);
}
