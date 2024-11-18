#include<iostream>
#include<sys/socket.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include "server_session.h"
using namespace std;

// #define BACKLOG 4096
// #define BUF_SIZE 1024
// #define USER_DATA_PATH "./usermetadata.txt"
// #define FILES_METADATA_PATH "./filemetadata.txt"
// #define FILE_DIR_PATH "./files/"

queue<pair<int,string>> connQueue;
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueWait = PTHREAD_COND_INITIALIZER;

void* processClient(void*);
int initThreadPool(vector<pthread_t>&, int, ServerSession*);

int main(int argc, char** argv) {

    if(argc != 2) {
        cout<<"Usage: ./server <port_no>"<<endl;
        return 1;
    }

    int port = atoi(argv[1]);
    int threadPoolSize = 4;

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    ServerSession* session = new ServerSession();
    if (session->loadUserMetaData() < 0) {
        cout << "ERROR: server->loadUserMetaData()" << endl;
    }
    // session->printUsers();
    
    if (session->loadFileMetaData() < 0) {
        return 1;
    }

    vector<pthread_t>workerThreads(threadPoolSize);
    for(int i = 0; i < threadPoolSize; ++i) {
        if(pthread_create(&workerThreads[i], NULL, &processClient, session) != 0) {
            cerr << "Failed to create thread";
            return 1;
        }
    }

    session->startServer(port, serverAddr);
    // session->startListening();

    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    while(1) {
        int connFd;
        connFd = accept(session->getsockfd(), (struct sockaddr*)&clientAddr, &clientLen);
        if(connFd < 0) {
            perror("accept connection error\n");
            return 1;
        }
        int port = ntohs(clientAddr.sin_port);
        string clientid = inet_ntoa(clientAddr.sin_addr);
        clientid += ":" + to_string(port);
        
        pthread_mutex_lock(&queueLock);
        connQueue.push({connFd, clientid});
        pthread_cond_signal(&queueWait);
        pthread_mutex_unlock(&queueLock);
        
    }

    return 0;
}

void* processClient(void* arg) {
    ServerSession* session = (ServerSession*)arg;
    while(1) {
        
        pthread_mutex_lock(&queueLock);
        while(connQueue.empty()) {
            pthread_cond_wait(&queueWait, &queueLock); // wait till queue is empty
        }
        int connFd = connQueue.front().first;
        string clientid = connQueue.front().second;
        connQueue.pop();
        pthread_mutex_unlock(&queueLock);
        // read input commands here
        char buff[BUF_SIZE] = {0};
        if(read(connFd, buff, sizeof(buff)) < 0) {
            cerr << "commmand read: read failed";
            break;
        }
        if(strlen(buff) == 0) {
            break;
        }
        cout<<"Request: "<< buff <<endl;
        vector<string>command;
        char* token = strtok(buff, " ");
        while(token) {
            command.push_back(token);
            token = strtok(NULL, " ");
        }
        if(command[0] == "login") {
            User user(command[1], command[2]);
            const char* resp = session->authenticateUser(user, clientid);
            write(connFd, resp, strlen(resp));
        } else if(command[0] == "add") {
            if(command.size() != 3) {
                write(connFd, "Usage: add <filename.txt>", 25);
                continue;
            }
            session->addFile(command[1], connFd, command[2]);
        } else if(command[0] == "listall") {
            string resp = session->listall();
            write(connFd, resp.c_str(), resp.length());
        } else if(command[0] == "quit") {
            session->quit(connFd, command[1]);
            continue;
        } else if(command[0] == "checkout") {
            if(command.size() != 3) {
                write(connFd, "Usage: checkout <filename.txt>", 30);
                continue;
            }
            session->checkout(command[1], connFd, command[2]);
        } else if(command[0] == "commit") {
            if(command.size() != 3) {
                write(connFd, "Usage: commit <filename.txt>", 28);
                continue;
            }
            session->commit(command[1], connFd, command[2]);
        } else if(command[0] == "delete") {
            if(command.size() != 3) {
                write(connFd, "Usage: delete <filename.txt>", 28);
                continue;
            }
            session->deleteFile(command[1], connFd, command[2]);
        } else {
            cout << "Enter a valid command!" << endl;
        }
        close(connFd);
    }
    cout<<"Connection with client terminated successfully"<<endl;
    return NULL;
}
