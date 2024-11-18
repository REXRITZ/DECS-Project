#include "file.h"

typedef struct User User;

struct User{
    string username;
    string password;
    unordered_set<string> checkedoutFiles;
    string filesDir;
    string metadataPath;
    bool loggedIn;
    User() {
        loggedIn = false;
        checkedoutFiles.clear();
    }

    User(string username, string pass) {
        this->username = username;
        password = pass;
    }
};