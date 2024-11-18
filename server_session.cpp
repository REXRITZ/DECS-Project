#include "server_session.h"

namespace fs = std::filesystem;

ServerSession::ServerSession() {
    sessionLock = PTHREAD_MUTEX_INITIALIZER;
    sessionWait = PTHREAD_COND_INITIALIZER;
}

ServerSession:: ~ServerSession() {
    pthread_mutex_destroy(&sessionLock);
    pthread_cond_destroy(&sessionWait);
    users.clear();
    activeUsers.clear();
    clientMap.clear();
    filesMap.clear();
}

int ServerSession:: getsockfd() {
    return sockfd;
}

int ServerSession::loadFileMetaData() {

    // ifstream file(FILES_METADATA_PATH);
    // if(!file.is_open()) {
    //     cout<<"file open error"<<endl;
    //     return -1;
    // }
    // string line;
    // FileMetaData metadata;
    // while(getline(file, line)) {
    //     stringstream ss(line);
    //     string perm;
    //     ss  >> metadata.filename >> metadata.path >> metadata.isModified 
    //         >> metadata.hasWriteLock >> metadata.owner >> metadata.lastModified 
    //         >> metadata.currentReaders;
    //     filesMap[metadata.filename] = metadata;
    // }

    try {
        for (const auto &entry : fs::directory_iterator(FILE_DIR_PATH)) {
            // Print the relative path and filename
            std::cout << "File: " << entry.path().filename() 
                      << " | Relative Path: " << entry.path() << std::endl;
            FileMetaData metadata;
            metadata.filename = entry.path().filename();
            metadata.path = entry.path();
            filesMap[metadata.filename] = metadata;
        }
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}

int ServerSession::loadUserMetaData() {
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

int ServerSession::startServer(int port, sockaddr_in serverAddr) {
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

const char* ServerSession::authenticateUser(User user, string clientid) {
    pthread_mutex_lock(&sessionLock);
    if(users.find(user.username) == users.end()) {
        ofstream file;
        file.open(USER_DATA_PATH, ios_base::app);
        file << user.username << " " << user.password << " " << endl;
        users[user.username] = user;
    }
    //TODO: add password validation also lol!
    if(activeUsers.find(user.username) != activeUsers.end()) {
        pthread_mutex_unlock(&sessionLock);
        return "User with given username already logged in!";
    }
    if(users[user.username].password != user.password) {
        pthread_mutex_unlock(&sessionLock);
        return "Invalid password!";
    }
    activeUsers[user.username] = user;
    // clientMap[clientid] = user;
    pthread_mutex_unlock(&sessionLock);
    return "OK";
}

void ServerSession:: checkout(string filename, int connFd, string username) {
    pthread_mutex_lock(&sessionLock);
    if(filesMap.find(filename) == filesMap.end()) {
        write(connFd, "Given filename does not exists!", 29);
        pthread_mutex_unlock(&sessionLock);
        return;
    }
    
    FileMetaData fileMetaData = filesMap[filename];
    // check if user has already checked out file
    if(fileMetaData.hasWriteLock) {
        write(connFd, "Write lock already exists on the file!", 38);
        pthread_mutex_unlock(&sessionLock);
        return;
    }
    else
        write(connFd, "OK\n", 3);
    User user = activeUsers[username];
    if(!user.checkedoutFiles.count(filename))
        fileMetaData.currentReaders++;
    string serializedData = fileMetaData.toString() + '\n';
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);

    int fd = open(fileMetaData.path.c_str(), O_RDONLY);
    // const char* data = serializedData.c_str();
    // write(connFd, data, strlen(data)+1);
    char buff[BUF_SIZE] = {0};
    
    while(1) {
        int bytesread = read(fd, buff, BUF_SIZE-1);
        if(bytesread <= 0)
            break;
        buff[bytesread] = '\0';
        write(connFd, buff, bytesread);
        cout<< "wrote:" << buff << endl;
    }
    // shutdown(sockfd,SHUT_RDWR);
    cout << "done\n";
}

void ServerSession:: commit(string filename, int connFd, string username) {
    pthread_mutex_lock(&sessionLock);
    if(filesMap.find(filename) == filesMap.end()) {
        write(connFd, "Given filename does not exists!", 29);
        pthread_mutex_unlock(&sessionLock);
        return;
    }
    FileMetaData fileMetaData = filesMap[filename];
    if(fileMetaData.hasWriteLock || fileMetaData.currentReaders > 0) {
        if(fileMetaData.currentReaders > 0)
            write(connFd, "Multiple readers are currently reading. Wait for them to finish!", 64);
        else
            write(connFd, "Write lock already exists on the file!", 38);
        pthread_mutex_unlock(&sessionLock);
        return;
    }
    write(connFd, "OK", 2);

    fileMetaData.hasWriteLock = true;
    fileMetaData.whoHasWriteLock = username;
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);

    pthread_mutex_lock(&fileMetaData.fileMutex);
    char buff[BUF_SIZE] = {0};
    ofstream file(fileMetaData.path);
    bool oAtEnd = false;
    while (true) {
        int readSz = 0;
        if ((readSz = read(connFd, buff, BUF_SIZE-1)) < 0) {
            perror("commit: read failed");
            break;
        }

        // cout << buff << '\n' << readSz << '\n';
        // cout << "readSz: " << readSz << "; readSz - 2: " << buff[readSz - 2] << "; readSz - 1: " << buff[readSz - 1] << "oAtEnd: " << oAtEnd << '\n';
        if (readSz == 0) {
            break;
        }
        buff[readSz] = '\0';

        if (readSz >= 2 && buff[readSz - 2] == 'O' && buff[readSz - 1] == 'K') {
            buff[readSz - 2] = buff[readSz - 1] = '\0';
            if (oAtEnd) {
                file << "O";
            }
            file << buff;
            break;
        }

        if (readSz == BUF_SIZE - 1 && buff[readSz - 1] == 'O') {
            buff[readSz - 1] = '\0';
            file << buff;
            oAtEnd = true;
            continue;
        }

        if (oAtEnd) {
            if (readSz == 1 && buff[readSz - 1] == 'K') {
                break;
            } else {
                oAtEnd = false;
                file << "O";
            }
        }

        file << buff;
    }
    file.close();
    pthread_mutex_unlock(&fileMetaData.fileMutex);

    pthread_mutex_lock(&sessionLock);
    fileMetaData.hasWriteLock = false;
    fileMetaData.whoHasWriteLock = "";
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);
}

const char* ServerSession::createFile(string filename, string clientid) {
    pthread_mutex_lock(&sessionLock);
    if(filesMap.find(filename) != filesMap.end()) {
        pthread_mutex_unlock(&sessionLock);
        return "File with given name already exists!";
    }
    FileMetaData fileMetaData(filename);
    fileMetaData.path = FILE_DIR_PATH + filename;
    fileMetaData.lastModified = time(nullptr); // unix epoch timestamp
    // fileMetaData.owner = clientMap[clientid].username;
    // write to persist file meta data info
    ofstream file1(fileMetaData.path);
    file1.close();
    ofstream file;
    // file.open(FILES_METADATA_PATH, ios_base::app);
    // file << fileMetaData.filename << " " << fileMetaData.path << " " << fileMetaData.isModified 
    //         << " " << fileMetaData.hasWriteLock << " " << fileMetaData.owner << " " << fileMetaData.lastModified 
    //         << " " << fileMetaData.currentReaders << endl;
    // file.close();
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);
    return "OK";
}

string ServerSession:: listall() {
    unordered_map<string, FileMetaData>:: iterator it;
    string resp = "";
    for (it = filesMap.begin(); it != filesMap.end(); ++it) {
        resp += "filename: " + it->first + ", current readers: " + to_string(it->second.currentReaders) + "\n";
    }
    return resp;
}

void ServerSession::printUsers() {
    int i = 1;
    for (auto user : users) {
        cout << "User " << i++ << "-> username: " << user.first
                << ", password: " << user.second.password << endl;
    }
}

void ServerSession::printFileMetaData() {
    for (auto file : filesMap) {
        cout << "filename " << file.second.filename << endl;
    }
}

void ServerSession::quit(int connFd, string clientid) {
    pthread_mutex_lock(&sessionLock);
    cout<<"Inside quit" << endl;
    //TODO remove clientmap and use username passed as argument later
    User user = clientMap[clientid];
    for(string filename : user.checkedoutFiles) {
        FileMetaData metadata = filesMap[filename];
        metadata.currentReaders--;
        if(metadata.whoHasWriteLock == user.username) {
            metadata.hasWriteLock = false;
            metadata.whoHasWriteLock = "";
        }
        filesMap[filename] = metadata;
    }
    clientMap.erase(clientid);
    activeUsers.erase(user.username);
    pthread_mutex_unlock(&sessionLock);
    close(connFd);
}