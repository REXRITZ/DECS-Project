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
    filesMap.clear();
}

int ServerSession:: getsockfd() {
    return sockfd;
}

int ServerSession::loadFileMetaData() {

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
    // check if someone has already checked out file
    if(fileMetaData.hasWriteLock) {
        write(connFd, "Write lock already exists on the file!", 38);
        pthread_mutex_unlock(&sessionLock);
        return;
    }
    else
        write(connFd, "OK\n", 3);
    
    User user = activeUsers[username];
    if(!user.checkedoutFiles.count(filename)) {
        fileMetaData.currentReaders++;
        user.checkedoutFiles.insert(filename);
        activeUsers[username] = user;
    }
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);

    int fd = open(fileMetaData.path.c_str(), O_RDONLY);
    char buff[BUF_SIZE] = {0};
    
    while(1) {
        int bytesread = read(fd, buff, BUF_SIZE-1);
        if(bytesread <= 0)
            break;
        buff[bytesread] = '\0';
        write(connFd, buff, bytesread);
        cout<< "wrote:" << buff << endl;
    }
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
    if(fileMetaData.hasWriteLock) {
        if(!(fileMetaData.currentReaders == 1 && username == fileMetaData.whoHasWriteLock)) {
            write(connFd, "Write lock already exists on the file! Try running checkout after some time.", 38);
            pthread_mutex_unlock(&sessionLock);
            return;
        }
    }
    write(connFd, "OK", 2);

    fileMetaData.hasWriteLock = true;
    fileMetaData.whoHasWriteLock = username;
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);

    pthread_mutex_lock(&fileMetaData.fileMutex);
    string respStr = "";
    char resp[BUF_SIZE] = {0};
    while (true) {
        int readSz = 0;
        if ((readSz = read(connFd, resp, BUF_SIZE-1)) < 0) {
            perror("commit: read failed");
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

    ofstream file;
    file.open(fileMetaData.path);
    file << respStr;
    pthread_mutex_unlock(&fileMetaData.fileMutex);

    pthread_mutex_lock(&sessionLock);
    fileMetaData.hasWriteLock = false;
    fileMetaData.whoHasWriteLock = "";
    filesMap[filename] = fileMetaData;
    pthread_mutex_unlock(&sessionLock);
}

const char* ServerSession::createFile(string filename, string username) {
    pthread_mutex_lock(&sessionLock);
    if(filesMap.find(filename) != filesMap.end()) {
        pthread_mutex_unlock(&sessionLock);
        return "File with given name already exists!";
    }
    FileMetaData fileMetaData(filename);
    fileMetaData.path = FILE_DIR_PATH + filename;
    fileMetaData.lastModified = time(nullptr); // unix epoch timestamp
    ofstream file1(fileMetaData.path);
    file1.close();
    ofstream file;
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

void ServerSession::quit(int connFd, string username) {
    pthread_mutex_lock(&sessionLock);
    User user = activeUsers[username];
    for(string filename : user.checkedoutFiles) {
        FileMetaData metadata = filesMap[filename];
        metadata.currentReaders--;
        if(metadata.whoHasWriteLock == user.username) {
            metadata.hasWriteLock = false;
            metadata.whoHasWriteLock = "";
        }
        filesMap[filename] = metadata;
    }
    activeUsers.erase(user.username);
    pthread_mutex_unlock(&sessionLock);
    close(connFd);
}