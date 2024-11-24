# DECS-Project
## Abstract
Design a file-sharing service that can be accessed by multiple users for read, write, create and delete. Users connect to the service via a client process and can browse, checkout, commit, add, delete, writelock files to the repository.

On checkout, a local copy is created on the client's machine. Changes have to be committed, and each commit has to be synchronized across users using the writelock over the network.

The client can list all the files available on the server with the number of current readers/writers. A commit operation is required to update a file on the server.
Clients can read shared files simultaneously, but only one user can write at a time, etc.

## Build Instructions

- Server: `g++ server_session.cpp server.cpp -o server -lpthread`
- Client: `g++ client.cpp -o client`

## Usage Instructions

### Server

- Start server: `./server <port-number>`

### Client

- Start client: `./client <server-ip> <server-port>`

#### Client-Side commands

##### listall

- List all files present on server with current readers/writers count.
- Usage: `listall`

##### checkout

- Download file from server and store on client side.
- Usage: `checkout <file-name>`

##### commit

- Update a file on server. (syncronized)
- Usage: `commit <file-name>`

##### add

- Add a new file to the server.
- Usage: `add <file-path>`

##### delete

- Delete a file on the server.
- Usage: `delete <file-name>`

##### quit

- Quit the client application. Any logged-in user will be logged-out.
- Usage: `quit`

##### help

- Give information about all the available commands.
- Usage: `help`
