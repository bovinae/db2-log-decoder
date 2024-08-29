/*
    互斥量 实现 多进程 之间的同步 
*/

#pragma once

#include <pthread.h>
#include <string>

typedef struct multi_process_mutex {
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;
}multi_process_mutex;

class mutex_wrapper
{
public:
    mutex_wrapper() {
        file_name_ = "";
    }
    mutex_wrapper(std::string file_name) : file_name_(file_name) {}

    int init_multi_process_mutex(bool is_new = false);
    void destroy_multi_process_mutex();
    void multi_process_mutex_lock();
    int multi_process_mutex_trylock();
    void multi_process_mutex_unlock();

private:
    multi_process_mutex *mm;
    std::string file_name_;

    int get_fd(const char* file_name, bool& is_new);
    int named_mmap(const char* file_name, bool& is_new);
    int anon_mmap();
};

int clear_files_in_directory(const char* dir);