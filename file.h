#include<string>
#include<unordered_map>
#include <ctime>
#include<pthread.h>
using namespace std;

enum Permission {
    READ,
    WRITE,
    BOTH
};

typedef struct {
    // int id;
    string filename; // filename can also be id??
    Permission perms;
    string path;
    bool isModified;
    bool hasWriteLock;
    string owner;   // username of file owner
    time_t lastModified;
    int currentReaders;
    pthread_mutex_t mutex;
    pthread_cond_t cond; 
    // add methods here
} FileMetaData;