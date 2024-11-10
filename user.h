#include "file.h"

typedef struct User User;

struct User{
    string username;
    string password;
    unordered_map<string, FileMetaData> files;
    string filesDir;
    string metadataPath;

    User() {}

    User(string username, string pass) {
        this->username = username;
        password = pass;
    }
};