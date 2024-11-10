#include "file.h"


typedef struct {
    string username;
    string name;
    string password;
    unordered_map<string, FileMetaData> files;
    string filesDir;
    string metadataPath;
} User;