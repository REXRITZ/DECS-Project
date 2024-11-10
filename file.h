#include<string>
#include<unordered_map>
#include <ctime>
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
    int currentReaders;
    pthread_mutex_t mutex;
    pthread_cond_t cond; 
    // add methods here

    FileMetaData() {}

    FileMetaData(string filename) {
        this->filename = filename;
    }

};