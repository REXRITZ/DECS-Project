#include<bits/stdc++.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include "user.h"
using namespace std;

#define IP_SIZE 4
#define FILES_METADATA_PATH "./filemetadata.txt"
#define BUF_SIZE 1024

typedef struct {
    string username;
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

    Client() {
        sockfd = -1;
        memset(&serverAddr, 0, sizeof(sockaddr_in));
    }

    int loadFileMetaData() {

        ifstream file(FILES_METADATA_PATH);
        if(!file.is_open()) {
            cout<<"file open error"<<endl;
            return -1;
        }
        string line;
        FileMetaData metadata;
        while(getline(file, line)) {
            stringstream ss(line);
            string perm;
            ss  >> metadata.filename >> metadata.path >> metadata.isModified 
                >> metadata.hasWriteLock >> metadata.owner >> metadata.lastModified 
                >> metadata.currentReaders;
            filesMap[metadata.owner] = metadata;
        }
        return 0;
    }

    int startServer(int port, char* ip) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            perror("socket creation failed\n");
            return -1;
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        cout<<ip;
        if(inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0) {
            cout<<"Invalid address/ Address not supported"<<endl;
            return -1;
        }

        if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Connection Failed\n");
            return -1;
        }
        while(1) {
            login();
            char resp[BUF_SIZE] = {0};
            recv(sockfd, resp, sizeof(BUF_SIZE), 0);
            if(strcmp(resp,"OK") == 0)
                break;
            cout<<resp<<endl;
        }
        cout<<"Connected to server successfully."<<endl;
        return 0;
    }

    void login() {
        //check from server side whether user is registered or not
        User tempuser;
        cout<<"----Login----"<<endl;
        cout<<"Enter username: ";
        cin>>tempuser.username;
        cout<<"Enter password: ";
        cin>>tempuser.password;
        char data[BUF_SIZE] = {0};
        snprintf(data, BUF_SIZE, "login %s %s", tempuser.username.c_str(), tempuser.password.c_str());
        send(sockfd, data, strlen(data), 0);
    }

    int checkout() {
        // checkout feature
        return 0;
    }

    int commit() {
        // commit feature
        return 0;
    }

    int deleteFile(string filename) {
        FileMetaData file;
        if(filesMap.find(filename) == filesMap.end()) {
            cout<<"Invalid filename given"<<endl;
            return -1;
        }
        file = filesMap[filename];
        if(user.username == file.owner) {
            cout<<"Permission denied. You are not the file owner"<<endl;
            return -1;
        }
        // delete file from server and locally also
        return 0;
    }

    int addFile(string filepath) {
        // add new file to server
        return 0;
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

    Client* client = new Client();

    // client->loadFileMetaData();
    client->startServer(port, ip);
    

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
