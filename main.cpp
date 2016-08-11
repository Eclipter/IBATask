#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>

using namespace std;

void search_in_folder(const char* path, string data) {
    DIR* directory = opendir(path);
    struct dirent* entry;
    if(!directory) {
        perror("Error while opening directory");
    }
    while((entry = readdir(directory)) != NULL) {
        if(entry->d_type == DT_DIR) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            cout << entry->d_name << " is a directory." << endl;
            search_in_folder(entry->d_name, data);
        }
        else if(entry->d_type == DT_REG) {
            cout << entry->d_name << endl;
        }
        else if(entry->d_type == DT_UNKNOWN) {
            cout << "Unknown entry: " << entry->d_name << ", type: " << entry->d_type << endl;
        }
    }
    closedir(directory);
}

int main() {
    string path;
    string data;
    cout << "Enter the data to search (1 to 255 symbols):" << endl;
    getline(cin, data);
    if(data.empty()) {
        cout << "The data must be non-empty" << endl;
        return 0;
    }
    cout << "Enter the path where to search the data:" << endl;
    getline(cin, path);
    DIR* root_dir = opendir(path.c_str());
    if(root_dir) {
        cout <<"Directory exists. Started searching..." << endl;
        closedir(root_dir);
        search_in_folder(path.c_str(), data);
    }
    else if(errno == ENOENT) {
        cout << "Directory does not exist." << endl;
    }
    else {
        cerr << "Failed to open directory" << endl;
    }
    return 0;
}