#include<string>
#include<unordered_map>
#include <ctime>
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
    time_t lastModified;
    int currentReaders = 0;
    pthread_mutex_t mutex;
    pthread_cond_t cond; 
    // add methods here

    FileMetaData() {}

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