#include<string>
#include<unordered_map>
#include<unordered_set>
#include<ctime>
#include<sstream>
#include<pthread.h>
using namespace std;

typedef struct FileMetaData FileMetaData;

struct FileMetaData{
    // int id;
    string filename; // filename can also be id??
    string path;
    bool isModified = false;
    bool hasWriteLock = false;
    string owner;   // username of file owner
    string whoHasWriteLock; // who has write lock ?
    time_t lastModified;
    int currentReaders = 0;
    pthread_mutex_t fileMutex;
    pthread_cond_t fileWait; 
    // add methods here

    FileMetaData() {
        fileMutex = PTHREAD_MUTEX_INITIALIZER;
        fileWait = PTHREAD_COND_INITIALIZER;
    }

    ~FileMetaData() {
        pthread_mutex_destroy(&fileMutex);
        pthread_cond_destroy(&fileWait);
    }

    FileMetaData(string filename) {
        this->filename = filename;
    }

    string toString() {
        stringstream oss;
        oss << filename << " " << path << " " << isModified 
            << " " << hasWriteLock << " " << owner 
            << " " << lastModified << " " << currentReaders;
        return oss.str();
    }

    void fromString(string str) {
        stringstream iss(str);
        iss  >> filename >> path >> isModified 
                >> hasWriteLock >> owner >> lastModified 
                >> currentReaders;
    }

};