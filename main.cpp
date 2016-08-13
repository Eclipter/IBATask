#define MAX_THREADS 4

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

sem_t thread_semaphore;
string data;
vector<pthread_t> thread_vector;

void* scan_file(void* param) {
	char* path = (char*) param;
    //cout << "Thread " << pthread_self() << ", path: " << path << endl;
    ifstream fin(path);
    stringstream stream;
    stream << fin.rdbuf();
    string content = stream.str();

    int occurrences_count = 0;
    size_t found_ind = 0;

    found_ind = content.find(data);

    while(found_ind != string::npos) {
        occurrences_count++;
        found_ind = content.find(data, found_ind + 1);
    }

    if(occurrences_count > 0) {
        cout << "FILE: " << path << ". OCCURRENCES: " << occurrences_count << endl;
    }

    fin.close();

    sem_post(&thread_semaphore);
    pthread_exit(NULL);
}

void search_in_folder(string path) {
    DIR* directory = opendir(path.c_str());
    struct dirent* entry;
    if(!directory) {
        cout << ("ERROR: Error while opening directory %s", path) << endl;
        return;
    }
    while((entry = readdir(directory)) != NULL) {
        if(entry->d_type == DT_DIR) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            //cout << "Directory: " << path << "/" << entry->d_name << endl;

            string dir_path;
            dir_path.append(path).append("/").append(entry->d_name);

            search_in_folder(dir_path);
        }
        else if(entry->d_type == DT_REG) {

            //cout << "File: " << path << "/" << entry->d_name << endl;

            string file_path;
            file_path.append(path).append("/").append(entry->d_name);

            pthread_t thread_t;

            sem_wait(&thread_semaphore);

            int create_res = pthread_create(&thread_t, NULL, scan_file, (void*) file_path.c_str());

            thread_vector.push_back(thread_t);

            if(create_res) {
                cerr << "ERROR: Error while creating thread." << endl;
            }

        }
        else if(entry->d_type == DT_UNKNOWN) {
            cout << "Unknown entry: " << entry->d_name << ", type: " << entry->d_type << endl;
        }
    }
    closedir(directory);
}

int main() {
	
    string path;
    cout << "Enter the data to search (1 to 255 symbols):" << endl;
    getline(cin, data);
    if(data.empty() || data.size() > 255) {
        cout << "The data must be non-empty and less than 255 symbols" << endl;
        return 0;
    }
    cout << "Enter the path where to search the data:" << endl;
    getline(cin, path);
    DIR* root_dir = opendir(path.c_str());
    if(root_dir) {
        cout << "Directory exists. Started searching..." << endl;
        closedir(root_dir);
		
		if (sem_init(&thread_semaphore, 0, MAX_THREADS)) {
			cerr << "ERROR: Failed to create semaphore." << endl;
			return 1;
		}
		
        search_in_folder(path);

        cout << "Waiting for threads..." << endl;

        for(vector<pthread_t>::iterator it = thread_vector.end() - MAX_THREADS; it != thread_vector.end(); it++) {
            pthread_join(*it, NULL);
        }

        cout << "Jobs complete!" << endl;

        sem_destroy(&thread_semaphore);
    }
    else if(errno == ENOENT) {
        cout << "Directory does not exist." << endl;
    }
    else {
        cerr << "ERROR: Failed to open directory." << endl;
        return 1;
    }
    return 0;
}
