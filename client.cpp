#include<bits/stdc++.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include "user.h"
using namespace std;

#define IP_SIZE 16
#define FILES_METADATA_PATH "./filemetadata.txt"
#define BUF_SIZE 1024
#define USERS_DIR "./users"

// Client-side commands.
#define QUIT "quit"
#define LISTALL "listall"
#define CHECKOUT "checkout"
#define COMMIT "commit"
#define ADD "add"
#define DELETE "delete"

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

    Client(int port, char* ip) {
        this->port = port;
        strcpy(this->ip, ip);
    }

    void checkDir(string dirPath) {
        struct stat st;
        if (stat(dirPath.c_str(), &st) == -1) {
            if (mkdir(dirPath.c_str(), 0777) == -1) {
                std::cerr << "Error: Could not create folder " << dirPath << std::endl;
                return;
            }
        }
    }

    void checkDirs() {
        checkDir(USERS_DIR);
        string path = USERS_DIR;
        path += "/" + user.username;
        checkDir(path);
        checkDir(user.filesDir);
    }

    int loadFileMetaData() {
        checkDirs();
        ifstream file(user.metadataPath);
        if(!file.is_open()) {
            cout << "Its empty here." << endl;
            return -1;
        }
        string line;
        FileMetaData metadata;
        while(getline(file, line)) {
            stringstream ss(line);
            string perm;
            ss  >> metadata.filename >> metadata.path
                >> metadata.hasWriteLock >> metadata.owner >> metadata.lastModified 
                >> metadata.currentReaders;
            filesMap[metadata.filename] = metadata;
        }
        return 0;
    }

    int startServer() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            perror("socket creation failed\n");
            return -1;
        }
        memset(&serverAddr, 0, sizeof(sockaddr_in));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        if(inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0) {
            cout<<"Invalid address/ Address not supported"<<endl;
            return -1;
        }

        if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Connection Failed\n");
            return -1;
        }
        if(user.loggedIn)
            return 0;
        login();
        char resp[BUF_SIZE] = {0};
        read(sockfd, resp, BUF_SIZE);
        cout<<"login response: " << resp << endl;
        if(strcmp(resp,"OK") == 0) {
            user.metadataPath = "./" + user.username + "/filemetadata.txt";
            user.filesDir = USERS_DIR;
            user.filesDir += "/" + user.username + "/files";
            user.loggedIn = true;
            cout << "metadataPath: " << user.metadataPath << " filesDir: " << user.filesDir << endl;
            loadFileMetaData();
            cout<<"Connected to server successfully."<<endl;
            close(sockfd);
            return 0;
        }
        cout<<resp<<endl;
        return -1;
    }

    void login() {
        //check from server side whether user is registered or not
        cout<<"----Login----"<<endl;
        cout<<"Enter username: ";
        cin>>user.username;
        cout<<"Enter password: ";
        cin>>user.password;
        char data[BUF_SIZE] = {0};
        snprintf(data, BUF_SIZE, "login %s %s", user.username.c_str(), user.password.c_str());
        cout<<"sending request:" << data << endl;
        write(sockfd, data, strlen(data));
    }

    int commit(string fileName, string filePath, bool add) {
        // commit feature
        string command;

        if (add) {
            command = ADD;
        } else {
            command = COMMIT;
        }

        int fd = open(filePath.c_str(), O_RDONLY);

        if (fd < 0) {
            cerr << "Couldn't open the file " << fileName << endl;
            return -1;
        }

        command += " " + fileName + " " + user.username;

        if (write(sockfd, command.c_str(), command.length() + 1) < 0) {
            cout << "commit: write failed" << endl;
            exit(EXIT_FAILURE);
        }

        char resp[BUF_SIZE] = {0};
        if (read(sockfd, resp, BUF_SIZE) < 0) {
            cout << "commit: read failed" << endl;
            exit(EXIT_FAILURE);
        }

        if(strcmp(resp,"OK") != 0) {
            cout << resp << endl;
            return 0;
        }
                
        char buff[BUF_SIZE] = {0};
        

        while(1) {
            int bytesread = read(fd, buff, BUF_SIZE-1);
            if(bytesread <= 0)
                break;
            buff[bytesread] = '\0';
            if(write(sockfd, buff, bytesread) < 0) {
                cerr<<"commit: write failed" << endl;
                break;
            }
        }

        if (add) {
            cout << "File added in the server. Please checkout the file using its filename." << endl;
        }

        return 0;
    }

    int deleteFile(string fileName) {
        string command = DELETE;
        command += " " + fileName + " " + user.username;
        if (write(sockfd, command.c_str(), command.length() + 1) < 0) {
            cout << "delete: write failed" << endl;
            exit(EXIT_FAILURE);
        }

        char resp[BUF_SIZE] = {0};
        if (read(sockfd, resp, BUF_SIZE) < 0) {
            cout << "delete: read failed" << endl;
            exit(EXIT_FAILURE);
        }

        if(strcmp(resp,"OK") != 0) {
            cout << resp << endl;
            return 0;
        }

        // delete file from server and locally also
        if(remove((user.filesDir + "/" + fileName).c_str()) != 0)
            write(sockfd, "Err: Unable to delete file", 26);

        return 0;
    }

    int listAll() {
        if (write(sockfd, LISTALL, strlen(LISTALL) + 1) < 0) {
            cout << "listAll: write failed" << endl;
            exit(EXIT_FAILURE);
        }

        char resp[BUF_SIZE] = {0};
        if (read(sockfd, resp, BUF_SIZE) < 0) {
            cout << "listAll: read failed" << endl;
            exit(EXIT_FAILURE);
        }

        cout << resp << endl;
        return 0;
    }

    void writeFileMetaData() {
        ofstream file;
        checkDirs();

        string fileMetaDataPath = "./" + user.username + "/filemetadata.txt";
        file.open(fileMetaDataPath);
        for (auto fileMD : filesMap) {
            file << fileMD.second.filename << " " << fileMD.second.path << " "  
                 << " " << fileMD.second.hasWriteLock << " " << fileMD.second.owner << " " << fileMD.second.lastModified 
                 << " " << fileMD.second.currentReaders << endl;
        }
    }

    int checkout(string fileName) {
        string command = CHECKOUT;
        command += " " + fileName + " " + user.username;
        if (write(sockfd, command.c_str(), command.length() + 1) < 0) {
            cout << "checkout: write failed" << endl;
            exit(EXIT_FAILURE);
        }

        string respStr = "";
        char resp[BUF_SIZE] = {0};
        while (true) {
            int readSz = 0;
            if ((readSz = read(sockfd, resp, BUF_SIZE-1)) < 0) {
                perror("checkout: read failed");
                break;
            }
            // resp[readSz] = '\0';
            if (readSz == 0) {
                break;
            }

            for (int i = 0; i < readSz; ++i) {
                respStr += resp[i];
            }
            // cout << "readSz: " << readSz << ", resp: " << resp << endl;
        }
        // cout << "respStr: " << respStr << endl;
        stringstream respSS(respStr);

        string buf;
        getline(respSS, buf);
        if (buf == "OK") {
            ofstream file;
            file.open(user.filesDir + "/" + fileName);
            file << respSS.rdbuf();
        } else {
            cout << resp << endl;
        }

        return 0;
    }

    void displayFileMetaData() {
        for(auto file : filesMap) {
            cout<<file.second.filename<<" "<<file.second.currentReaders<<endl;
        }
    }

    bool fileNameIsNotValid(string fileName) {
        return (fileName.length() < 5 || fileName.substr(fileName.length() - 4).compare(".txt") != 0);
    }

    string getFileNameFromPath(string filePath) {
        string fileName = "";
        for (int i = filePath.size() - 1; i >= 0; --i) {
            if (filePath[i] == '/') {
                break;
            }

            fileName += filePath[i];
        }

        reverse(fileName.begin(), fileName.end());
        return fileName;
    }

    void startClientLoop() {
        string command;
        cin.ignore();
        while (true) {
            cout << ">> ";
            if (getline(cin, command)) {
                stringstream cmdSS(command);
                vector<string> cmds;
                string word;
                while (cmdSS >> word) {
                    cmds.push_back(word);
                }
                if (cmds.size() == 0) {
                    continue;
                } else if (cmds[0].compare(QUIT) == 0) {
                    // TODO: Inform server.
                    startServer();
                    quit();
                    break;
                } else if (cmds[0].compare(LISTALL) == 0) {
                    startServer();
                    listAll();
                    close(sockfd);
                } else if (cmds[0].compare(CHECKOUT) == 0) {
                    if (cmds.size() < 2) {
                        cout << "Usage: checkout <file-name>" << endl;
                        continue;
                    }
                    
                    // if (fileNameIsNotValid(cmds[1])) {
                    //     cout << "checkout: Enter a valid file name ending with '.txt'." << endl;
                    //     continue;
                    // }
                    startServer();
                    checkout(cmds[1]);
                    close(sockfd);
                } else if (cmds[0].compare(COMMIT) == 0) {
                    if (cmds.size() < 2) {
                        cout << "Usage: commit <file-name>" << endl;
                        continue;
                    }
                    
                    // if (fileNameIsNotValid(cmds[1])) {
                    //     cout << "commit: Enter a valid file name ending with '.txt'." << endl;
                    //     continue;
                    // }
                    startServer();
                    commit(cmds[1], user.filesDir + "/" + cmds[1], false);
                    close(sockfd);
                } else if (cmds[0].compare(ADD) == 0) {
                    if (cmds.size() < 2) {
                        cout << "Usage: add <file-path>" << endl;
                        continue;
                    }
                    
                    // if (fileNameIsNotValid(cmds[1])) {
                    //     cout << "add: Enter a valid file path ending with '.txt'." << endl;
                    //     continue;
                    // }
                    startServer();
                    commit(getFileNameFromPath(cmds[1]), cmds[1], true);
                    close(sockfd);
                } else if (cmds[0].compare(DELETE) == 0) {
                    if (cmds.size() < 2) {
                        cout << "Usage: delete <file-path>" << endl;
                        continue;
                    }

                    startServer();
                    deleteFile(cmds[1]);
                    close(sockfd);
                } else {
                    cout << "Enter a valid command!" << endl;
                }
            }
        }
    }

    void quit() {
        string command = QUIT;
        command += " " + user.username;
        write(sockfd, command.c_str(), command.length() + 1);
        shutdown(sockfd, SHUT_WR);
        close(sockfd);
    }

};

// global variable
Client* client;

void handleSignal(int signal) {
    cout<<"Inside handler\n";
    client->quit();
    exit(0);
}

int main(int argc, char **argv) {
    
    if(argc != 3) {
        cout<<"Usage: ./client <server_ip> <port_no>"<<endl;
        return 1;
    }

    if (signal(SIGINT, handleSignal) == SIG_ERR) {
        cerr << "Error setting up signal handler" << endl;
        return 1;
    }

    int port = atoi(argv[2]);
    char* ip = argv[1];

    client = new Client(port, ip);

    while (client->startServer() < 0) {
        // return 1;
    }

    client->startClientLoop();

    // Client side commands:
    // Done -- listall - to list all files present on server with current readers/writers count.
    // Done -- checkout - download file from server and store on client side.
    // Done -- commit - update file on server -> syncronized
    // Done -- create - create a new file to the server
    // delete - delete a file on the server
    
    // Questions so far:
    // Datastructure for client and server?
    // How to store metadata of files?
    // How to apply writelock over a file on server?
    // User adding a file can set permissions for access or just give access to all?
    // Pull changes from client or else what ??


    
    
}
