#include<bits/stdc++.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>

using namespace std;


int main(int argc, char **argv) {
    
    if(argc != 3) {
        cout<<"Usage: ./client <server_ip> <port_no>"<<endl;
        return 1;
    }

    int port = atoi(argv[2]);
    char* ip = argv[1];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket creation failed\n");
        return 1;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    // Client side commands:
    // listall - to list all files present on server with current readers/writers count.
    // checkout - download file from server and store on client side.
    // commit - update file on server -> syncronized
    // add - add a new file to the server
    // delete - delete a file on the server
    
    // Questions so far:
    // Datastructure for client and server?
    // How to store metadata of files?
    // How to apply writelock over a file on server?
    // User adding a file can set permissions for access or just give access to all?
    // Pull changes from client or else what ??



    
}
