#include<bits/stdc++.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include "user.h"
using namespace std;

#define BACKLOG 4096
#define BUF_SIZE 1024
#define USER_DATA_PATH "./usermetadata.txt"
#define FILES_METADATA_PATH "./filemetadata.txt"
#define FILE_DIR_PATH "./files/"

class Server {
    int sockfd;
    unordered_map<string, FileMetaData> filesMap;
    unordered_map<string, User> users;
    unordered_map<string, User> activeUsers; 
    unordered_map<string, User> clientMap; //mapping of ip address to user
    queue<int>connQueue; // queue of connection fds
public:

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
            filesMap[metadata.filename] = metadata;
        }
        return 0;
    }

    int loadUserMetaData() {
        ifstream file(USER_DATA_PATH);
        if(!file.is_open()) {
            cout << "file open error" << endl;
            return -1;
        }

        string line;
        
        while(getline(file, line)) {
            stringstream ss(line);
            User user;
            ss >> user.username >> user.password;
            users[user.username] = user;
        }

        return 0;
    }

    int startServer(int port, sockaddr_in serverAddr) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            perror("socket creation failed\n");
            return -1;
        }

        if(bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Bind failed\n");
            return -1;
        }
        if((listen(sockfd, BACKLOG)) < 0) {
            perror("listen failed\n");
            return -1;
        }
        cout<<"Server started successfully"<<endl;
        return 0;
    }

    void startListening() {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        while(1) {
            int connFd;
            connFd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientLen);
            if(connFd < 0) {
                perror("accept connection error\n");
                return;
            }
            int port = ntohs(clientAddr.sin_port);
            string clientid = inet_ntoa(clientAddr.sin_addr);
            clientid += ":" + to_string(port);
            
            while(1) {
                // read input commands here
                char buff[BUF_SIZE] = {0};
                if(read(connFd, buff, sizeof(buff)) < 0) {
                    perror("commmand read: read failed");
                    break;
                }
                if(strlen(buff) == 0)
                    break;
                cout<<"Request: "<< buff <<endl;
                vector<string>command;
                char* token = strtok(buff, " ");
                while(token) {
                    command.push_back(token);
                    token = strtok(NULL, " ");
                }
                if(command[0] == "login") {
                    User user(command[1], command[2]);
                    const char* resp = authenticateUser(user, clientid);
                    write(connFd, resp, strlen(resp));
                } else if(command[0] == "add") {
                    if(command.size() != 2) {
                        write(connFd, "Usage: login <filename.txt>", 27);
                        continue;
                    }
                    const char* resp = addFile(command[1], clientid);
                    write(connFd, resp, strlen(resp));
                } else if(command[0] == "listall") {
                    string resp = listall();
                    write(connFd, resp.c_str(), resp.length());
                } else if(command[0] == "quit") {
                    printUsers();
                    quit(connFd, clientid);
                } else if(command[0] == "checkout") {
                    checkout(command[1], connFd);
                } else if(command[0] == "commit") {
                    commit(command[1], connFd);
                } else {
                    cout << "Enter a valid command!" << endl;
                }
            }
            cout<<"Connection with client terminated successfully"<<endl;
            printUsers();
        }
    }

    const char* authenticateUser(User user, string clientid) {
        if(users.find(user.username) == users.end()) {
            ofstream file;
            file.open(USER_DATA_PATH, ios_base::app);
            file << user.username << " " << user.password << " " << endl;
            users[user.username] = user;
        }
        //TODO: add password validation also lol!
        if(activeUsers.find(user.username) != activeUsers.end())
            return "User with given username already logged in!";
        activeUsers[user.username] = user;
        clientMap[clientid] = user;
        return "OK";
    }

    bool addUser(User user) {
        if(users.find(user.username) == users.end())
            return false;
        
        activeUsers[user.username] = user;
        return true;
    }

    void checkout(string filename, int connFd) {
        if(filesMap.find(filename) == filesMap.end()) {
            write(connFd, "Given filename does not exists!", 29);
            return;
        }
        write(connFd, "OK", 2);
        FileMetaData fileMetaData = filesMap[filename];
        int fd = open(fileMetaData.path.c_str(), O_RDONLY);
        string serializedData = fileMetaData.toString();
        write(connFd, serializedData.c_str(), serializedData.length());
        char buff[BUF_SIZE] = {0};
        
        while(1) {
            int bytesread = read(fd, buff, BUF_SIZE-1);
            if(bytesread <= 0)
                break;
            buff[bytesread] = '\0';
            write(connFd, buff, bytesread);
        }
        write(connFd, "OK", 2);

        cout << "done\n";
    }

    void commit(string filename, int connFd) {
        if(filesMap.find(filename) == filesMap.end()) {
            write(connFd, "Given filename does not exists!", 29);
            return;
        }
        FileMetaData fileMetaData = filesMap[filename];
        if(fileMetaData.hasWriteLock) {
            write(connFd, "Write lock already exists on the file!", 38);
            return;
        }
        write(connFd, "OK", 2);
        fileMetaData.hasWriteLock = true;
        char buff[BUF_SIZE] = {0};
        ofstream file(fileMetaData.path);
        
        while(1) {
            int bytesread = read(connFd, buff, BUF_SIZE-1);
            if(bytesread <= 0)
                break;
            buff[bytesread] = '\0';
            file << buff;
        }
        file.close();
    }

    const char* addFile(string filename, string clientid) {
        if(filesMap.find(filename) != filesMap.end()) {
            return "File with given name already exists!";
        }
        FileMetaData fileMetaData(filename);
        fileMetaData.path = FILE_DIR_PATH + filename;
        fileMetaData.lastModified = time(nullptr); // unix epoch timestamp
        fileMetaData.owner = clientMap[clientid].username;
        // write to persist file meta data info
        ofstream file1(fileMetaData.path);
        file1.close();
        ofstream file;
        file.open(FILES_METADATA_PATH, ios_base::app);
        file << fileMetaData.filename << " " << fileMetaData.path << " " << fileMetaData.isModified 
                << " " << fileMetaData.hasWriteLock << " " << fileMetaData.owner << " " << fileMetaData.lastModified 
                << " " << fileMetaData.currentReaders << endl;
        file.close();
        filesMap[filename] = fileMetaData;
        return "OK";
    }

    string listall() {
        unordered_map<string, FileMetaData>:: iterator it;
        string resp = "";
        for (it = filesMap.begin(); it != filesMap.end(); ++it) {
            resp += "filename: " + it->first + ", current readers: " + to_string(it->second.currentReaders) + "\n";
        }
        return resp;
    }

    void printUsers() {
        int i = 1;
        for (auto user : users) {
            cout << "User " << i++ << "-> username: " << user.first
                 << ", password: " << user.second.password << endl;
        }
    }

    void printFileMetaData() {
        for (auto file : filesMap) {
            cout << "filename " << file.second.filename << endl;
        }
    }

    void quit(int connFd, string clientid) {
        cout<<"Inside quit" << endl;
        User user = clientMap[clientid];
        clientMap.erase(clientid);
        activeUsers.erase(user.username);
        close(connFd);
    }
};

int main(int argc, char** argv) {

    if(argc != 2) {
        cout<<"Usage: ./server <port_no>"<<endl;
        return 1;
    }

    int port = atoi(argv[1]);
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    Server* server = new Server();
    if (server->loadUserMetaData() < 0) {
        cout << "ERROR: server->loadUserMetaData()" << endl;
    }
    // server->printUsers();
    
    if (server->loadFileMetaData() < 0) {
        cout << "ERROR: server->loadFileMetaData()" << endl;
    }
    server->printFileMetaData();
    server->startServer(port, serverAddr);
    server->startListening();
}