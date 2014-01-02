ftp
---

ftp is a simple implementation of a file transfer program.  It includes both server and client programs, and currently provides functionality for browsing and retreiving files from the server.

#### Compilation:

Server: `make server`

Client: `make client`

Both: `make`

#### Execution:

Server: `ftserve`

Client: `ftclient <server hostname>`

To close either the server or the client: `ctrl-c or ctrl-d (sigint/sigterm)`

#### Usage:

The client interface accepts the following commands for navigating and accessing files on the server:

    pwd             - print working directory
    list            - view files in the current directory
    cd <directory>	- change directory
    get <filename>	- get the specified file
    exit	        - end the ftp session
