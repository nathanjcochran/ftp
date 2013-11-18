/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Program: ftclient.c
 * Author: Nathan Cochran
 * Date: 11/17/2013
 * Description: File transfer client.  Works with ftserve.c.
 *      Build with "make client" or simply "make".
 *      One command line argument is required: the hostname
 *      of the computer on which the server is running
 *      (ports are defined in ftutil.h).
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "ftclient.h"

//Static Variables:
int control_fd;

int main(int argc, char * argv[]) {
    char request[BUF_SIZE];

    //Ensure a hostname was specified:
    if(argc != 2) {
        printf("Usage:\n\t%s <server hostname>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    //Install signal handlers:
    install_signal_handlers();
    
    //Create a socket:
    control_fd = create_socket();

    //Open a control connection with host:
    control_connect(control_fd, argv[1]);

    do {
        //Receive a message (often just the prompt):
        receive_message(control_fd);

        //Get user request/input:
        get_request(control_fd, request);

        //Send the request to the server:
        make_request(control_fd, request);

    } while(parse_command(request, NULL) != EXIT);

    close(control_fd);
    printf("Connection closed\n");

    return EXIT_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Initiates a control connection with the specified server
 * Param:   int ctrl_fd -  File descriptor of the socket to open the connection on
 * Param:   char * host -  Name of the server to connect to
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void control_connect(int ctrl_fd, char * host) {
    struct addrinfo hints, *results, *p;

    //Specifications for the address to connect to:
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //Get address info for the specified host:
    if(getaddrinfo(host, CONTROL_PORT_STR, &hints, &results) != 0) {
        perror("Error getting server address info");
        printf("Please check that the server hostname is correct\n");
        exit(EXIT_FAILURE);
    }

    //Iterate through linked list of results, attempting to connect:
    for(p = results; p != NULL; p = p->ai_next) {
        if(connect(ctrl_fd, p->ai_addr, p->ai_addrlen) != -1) {
            freeaddrinfo(results);
            return;
        }
    }

    perror("Unable to open control connection with specified host");
    close(ctrl_fd);
    exit(EXIT_FAILURE);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Reads messages sent from the server until the server sends a prompt or closes the connection
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void receive_message(int ctrl_fd) {
    int i, num_read;
    char buffer[BUF_SIZE];

    for(i=0; i<BUF_SIZE-1; i++) {

        //Read in a character:
        if((num_read = read(ctrl_fd, &buffer[i], 1)) == -1) {
            perror("Error reading from control socket");
            close(ctrl_fd);
            exit(EXIT_FAILURE);
        }

        //No characters read in: socket has been closed by server
        if(num_read == 0)
            break;

        //End of message:
        if(strncmp(&buffer[i-(strlen(PROMPT)-1)], PROMPT, strlen(PROMPT)) == 0) {
            buffer[i+1] = '\0';
            break;
        }
    }

    //Display the message:
    printf("%s", buffer);

    //If the connection was closed by
    //the server, exit the program:
    if(num_read == 0) {
        close(ctrl_fd);
        exit(EXIT_SUCCESS);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Sends a request to the server, and handles any client-side preparations
 *      (e.g. listening on a port for incoming data connections)
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Param:   char * request -  The user's raw request
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void make_request(int ctrl_fd, char * request) {
    int data_fd, command;
    char arg[BUF_SIZE];

    //Send the raw request to the server:
    send_message(control_fd, request);

    //Parse the command:
    command = parse_command(request, arg);

    //If it was a GET request, receive file:
    if(command == GET) {
        data_fd = open_data_connection(ctrl_fd);
        receive_file(data_fd, arg);
        close(data_fd);
    }

    //If it was a LIST request, receive directory listing:
    else if(command == LIST) {
        data_fd = open_data_connection(ctrl_fd);
        receive_listing(data_fd);
        close(data_fd);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Reads user input into the response buffer
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Param:   char * response -  The buffer to store the user input
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void get_request(int ctrl_fd, char * response) {
    
    if(fgets(response, BUF_SIZE, stdin) == NULL) {
        perror("Error reading user input");
        close(ctrl_fd);
        exit(EXIT_FAILURE);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Creates a passive socket and waits for the server to connect,
 *      thereby initiating the data connection
 * Param:   int ctrl_fd -  File descriptor of the control connection
 * Return:  int -  File descriptor of the data connection
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int open_data_connection(int ctrl_fd) {
    struct sockaddr_in ctrl_address, data_address;
    int passive_fd, data_fd;
    unsigned int length;

    //Get peer's address from control socket:
    length = sizeof(ctrl_address);
    if(getpeername(ctrl_fd, (struct sockaddr *) &ctrl_address, &length) == -1) {
        perror("Error getting peer's address");
        close(ctrl_fd);
        exit(EXIT_FAILURE);
    }

    //Open a passive socket:
    passive_fd = create_socket();
    bind_socket(passive_fd, DATA_PORT);
    listen_socket(passive_fd);

    while(1) {

        //Accept an incoming connection:
        length = sizeof(data_address);
        if((data_fd = accept(passive_fd, (struct sockaddr *) &data_address, &length)) == -1) {
            perror("Error accepting incoming data connection");
            close(ctrl_fd);
            close(passive_fd);
            exit(EXIT_FAILURE);
        }

        //Check that it's originating from the expected address:
        if(ctrl_address.sin_addr.s_addr == data_address.sin_addr.s_addr) {
            break;
        }
        
        //If not, close the unknown connection and try again:
        close(data_fd);
    }

    //Close the passive socket and
    //return  the data connection socket:
    close(passive_fd);
    return data_fd;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Reads and displays a directory listing from a data connection
 * Param:   int data_fd -  File descriptor of the data connection to read from
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void receive_listing(int data_fd) {
    char buffer[BUF_SIZE];
    int num_read;

    //Read and display data until the connection is closed:
    while((num_read = read(data_fd, buffer, BUF_SIZE)) > 0) {
        printf("%s", buffer);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Receives a file over a data connection, saving it in the client's current directory
 * Param:   int data_fd -  File descriptor of the data connection
 * Param:   char * filename -  Name of the file that is being received
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void receive_file(int data_fd, char * filename) {
    int file_fd, num_read;
    char buffer[FILE_BUF_SIZE];

    //If data comes across the connection:
    if((num_read=read(data_fd, buffer, FILE_BUF_SIZE)) > 0) {

        //Create a file:
        if((file_fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0660)) == -1) {

            //If file already exists, prompt for overwrite:
            if(errno == EEXIST) {

                //Overwrite: create new file
                if(input_yn("File already exists. Overwrite? ")) {
                    if((file_fd = open(filename, O_WRONLY, 0666)) == -1) {
                        perror("Error creating file");
                        close(data_fd);
                        exit(EXIT_FAILURE);
                    }
                }

                //Don't overwrite: close data connection and return
                else {
                    printf("File not received: %s\n", filename);
                    close(data_fd);
                    return;
                }
            }
            
            //Other error: exit program
            else {
                perror("Error creating file");
                close(data_fd);
                exit(EXIT_FAILURE);
            }
        }

        //Write to the file:
        do {
            if(write(file_fd, buffer, num_read) == -1) {
                perror("Error writing to file");
                close(data_fd);
                exit(EXIT_FAILURE);
            }
        } while((num_read = read(data_fd, buffer, FILE_BUF_SIZE)) > 0);

        //Error reading from connection:
        if(num_read == -1) {
            perror("Error reading file from data connection");
            close(data_fd);
            exit(EXIT_FAILURE);
        }
        printf("File received: %s\n", filename);
    }

    //Error reading from connection:
    if(num_read == -1) {
        perror("Error reading file from data connection");
        close(data_fd);
        exit(EXIT_FAILURE);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * A signal handler for sigint and sigterm signals.  Cleans up and says goodbye.
 * Param:   int sig -  The signal received
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void signal_handler(int sig) {
    printf("\nClosing connection to server...\n");
    send_message(control_fd, "exit\n");

    close(control_fd);
    printf("Connection closed\n");
    exit(EXIT_SUCCESS);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Installs the signal handlers for the sigint and sigterm signals
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void install_signal_handlers(void) {
    struct sigaction sig;

    sig.sa_handler = signal_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;

    sigaction(SIGINT, &sig, NULL);
    sigaction(SIGTERM, &sig, NULL);
}
