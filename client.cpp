#include<bits/stdc++.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include "user.h"
using namespace std;

#define IP_SIZE 4
#define FILES_METADATA_PATH "./filemetadata.txt"

typedef struct {
    string name;
    vector<string> args;
} Command;

class Client {
    User user;
    sockaddr_in serverAddr;
    int port;
    char ip[IP_SIZE];
    int sockfd;
    unordered_map<string, FileMetaData> filesMap; // key = filename

public:

    Client(int port, char *ip) {
        strcpy(this->ip, ip);
        this->port = port;
        sockfd = -1;
        memset(&serverAddr, 0, sizeof(sockaddr_in));
    }

    int loadFileMetaData() {
        
        // int fd = open(FILES_METADATA_PATH, O_RDONLY);
        // if(fd < 0) {
        //     perror("failed to open fd\n");
        //     return -1;
        // }

        ifstream file(FILES_METADATA_PATH);
        if(!file.is_open()) {
            printf("file open error\n");
            return -1;
        }
        string line;
        FileMetaData metadata;
        while(getline(file, line)) {
            stringstream ss(line);
            string perm;
            ss  >> metadata.filename >> perm >> metadata.path >> metadata.isModified 
                >> metadata.hasWriteLock >> metadata.username >> metadata.lastModified 
                >> metadata.currentReaders;
            if(perm == "WRITE")
                metadata.perms = WRITE;
            else if(perm == "READ")
                metadata.perms = READ;
            else {
                printf("Invalid permission value\n");
                return -1;
            }
            filesMap[metadata.username] = metadata;
        }
        return 0;
    }

    int startServer() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            perror("socket creation failed\n");
            return -1;
        }
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
        //TODO: implement handshake with server
        printf("Connected to server successfully.\n");

    }

    void displayFileMetaData() {
        for(auto file : filesMap) {
            cout<<file.second.filename<<" "<<file.second.currentReaders<<endl;
        }
    }

};

int main(int argc, char **argv) {
    
    if(argc != 3) {
        cout<<"Usage: ./client <server_ip> <port_no>"<<endl;
        return 1;
    }

    int port = atoi(argv[2]);
    char* ip = argv[1];

    Client* client = new Client(port, ip);

    client->loadFileMetaData();
    client->displayFileMetaData();
    

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
