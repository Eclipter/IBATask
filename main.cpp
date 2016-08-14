#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

sem_t thread_semaphore;
pthread_mutex_t output_mutex;
string data;
string current_dir;
bool logging = false;
vector<pthread_t> thread_vector;

void* scan_file(void* param) {
    char* path = (char*) param;

    if (logging) {
        cout << "Thread " << pthread_self() << ", path: " << path << endl;
    }

    ifstream fin(path);
    stringstream stream;
    stream << fin.rdbuf();
    string content = stream.str();

    int occurrences_count = 0;
    size_t found_ind = 0;

    found_ind = content.find(data);

    while (found_ind != string::npos) {
        occurrences_count++;
        found_ind = content.find(data, found_ind + 1);
    }

    if (occurrences_count > 0) {
        pthread_mutex_lock(&output_mutex);
        cout << "FILE: " << path << ". OCCURRENCES: " << occurrences_count << endl;
        pthread_mutex_unlock(&output_mutex);
    }

    fin.close();

    sem_post(&thread_semaphore);
    pthread_exit(NULL);
}

void search_in_folder(string path) {
    DIR* directory = opendir(path.c_str());
    struct dirent* entry;
    if (!directory) {
        cout << ("ERROR: Error while opening directory %s", path) << endl;
        return;
    }
    while ((entry = readdir(directory)) != NULL) {

        if (entry == NULL) {
            cerr << "WARN: Failed to read directory " << path << endl;
            break;
        }

        string entry_path;
        entry_path.append(path).append("/").append(entry->d_name);
        if (!entry_path.compare(current_dir)) {
            continue;
        } else {
            current_dir.assign(entry_path);
        }

        struct stat entry_stat;
        if (stat(entry_path.c_str(), &entry_stat)) {
            cerr << "WARN: Failed to retrieve entry status for " << entry_path << endl;
            continue;
        }

        if ((entry_stat.st_mode & S_IFMT) == S_IFDIR) {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
                continue;
            }

            if (logging) {
                cout << "Directory: " << entry_path << endl;
            }

            search_in_folder(entry_path);
        }
        else if ((entry_stat.st_mode & S_IFMT) == S_IFREG) {

            if (logging) {
                cout << "File: " << entry_path << endl;
            }

            pthread_t thread_t;

            sem_wait(&thread_semaphore);

            int create_res = pthread_create(&thread_t, NULL, scan_file, (void*) entry_path.c_str());

            thread_vector.push_back(thread_t);

            if (create_res) {
                cerr << "ERROR: Error while creating thread." << endl;
            }

        }
    }
    closedir(directory);
}

int main(int argc, char* argv[]) {

    if (argc == 2) {
        if (!strcmp(argv[1], "-help")) {
            cout << "Usage: task [-options]\n where options include:\n\t-log - enable logging to console\n\t-help - get help\n";
            return 0;
        }
        else if (!strcmp(argv[1], "-log")) {
            logging = true;
        }
    }

    string path;
    cout << "Enter the data to search (1 to 255 symbols):" << endl;
    getline(cin, data);
    if (data.empty() || data.size() > 255) {
        cout << "The data must be non-empty and less than 255 symbols" << endl;
        return 0;
    }
    cout << "Enter the path where to search the data:" << endl;
    getline(cin, path);
    DIR* root_dir = opendir(path.c_str());
    if (root_dir) {
        cout << "Directory exists. Started searching..." << endl;
        closedir(root_dir);

        int max_threads = sysconf(_SC_NPROCESSORS_ONLN) - 1;

        if (sem_init(&thread_semaphore, 0, max_threads)) {
            cerr << "ERROR: Failed to create semaphore." << endl;
            return 1;
        }

        if (pthread_mutex_init(&output_mutex, NULL)) {
            cerr << "ERROR: Failed to create mutex." << endl;
            return 1;
        }

        search_in_folder(path);

        for (vector<pthread_t>::iterator it = thread_vector.end() - max_threads; it != thread_vector.end(); it++) {
            pthread_join(*it, NULL);
        }

        cout << "Jobs complete!" << endl;

        sem_destroy(&thread_semaphore);
        pthread_mutex_destroy(&output_mutex);
    }
    else if (errno == ENOENT) {
        cout << "Directory does not exist." << endl;
    }
    else {
        cerr << "ERROR: Failed to open directory." << endl;
        return 1;
    }
    return 0;
}
