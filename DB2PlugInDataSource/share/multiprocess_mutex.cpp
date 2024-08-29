#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "multiprocess_mutex.h"
#include "log_imp.h"
#include <dirent.h>

int mutex_wrapper::init_multi_process_mutex(bool is_new) {
    if (file_name_.empty()) {
        // 匿名方式，is_new由父进程传入。
        int rc = anon_mmap();
        if (rc < 0) return rc;
    } else {
        is_new = false;
        int rc = named_mmap(file_name_.c_str(), is_new);
        if (rc < 0) return rc;
    }

    if (is_new) {
        memset(mm, 0x00, sizeof(multi_process_mutex));
        pthread_mutexattr_init(&mm->mutexattr); // 初始化 mutex 属性
        pthread_mutexattr_setpshared(&mm->mutexattr, PTHREAD_PROCESS_SHARED); // 修改属性为进程间共享
        pthread_mutex_init(&mm->mutex,&mm->mutexattr); // 初始化一把 mutex 锁
    }

    LOG_DEBUG("init_multi_process_mutex ok");
    return 0;
}

// 只能由执行了pthread_mutexattr_init的进程调用destroy
void mutex_wrapper::destroy_multi_process_mutex() {
    LOG_DEBUG("destroy_multi_process_mutex");
    // 锁文件由服务启动时清理吧
    // if (!file_name_.empty()) {
    //     unlink(file_name_.c_str());
    // }
    pthread_mutexattr_destroy(&mm->mutexattr);  // 销毁 mutex 属性对象
    pthread_mutex_destroy(&mm->mutex);          // 销毁 mutex 锁
    munmap(mm, sizeof(multi_process_mutex));
    LOG_DEBUG("destroy_multi_process_mutex ok");
}

void mutex_wrapper::multi_process_mutex_lock() {
    LOG_DEBUG("multi_process_mutex_lock");
    pthread_mutex_lock(&mm->mutex);
    LOG_DEBUG("multi_process_mutex_lock ok");
}

int mutex_wrapper::multi_process_mutex_trylock() {
    int rc = pthread_mutex_trylock(&mm->mutex);
    if (rc == 0) {
        return 0;
    }
    if (errno == EBUSY) {
        return 1;
    }
    return rc;
}

void mutex_wrapper::multi_process_mutex_unlock() {
    LOG_DEBUG("multi_process_mutex_unlock");
    pthread_mutex_unlock(&mm->mutex);
    LOG_DEBUG("multi_process_mutex_unlock ok");
}

int mutex_wrapper::get_fd(const char* file_name, bool& is_new) {
    int fd = open(file_name, O_RDWR, 0777);
    if (fd >= 0) {
        return fd;
    }
    if (errno != ENOENT) {
        LOG_ERROR("open file failed:{}", strerror(errno));
        return -1;
    }

    // no file and then create
    fd = open(file_name, O_CREAT|O_EXCL|O_RDWR, 0777);
    if (fd >= 0) {
        is_new = true;
        return fd;
    }
    if (errno != EEXIST){
        LOG_ERROR("open file failed:{}", strerror(errno));
        return -1;
    }
    // open again
    fd = open(file_name, O_RDWR, 0777);
    if (fd == -1) {
        LOG_ERROR("open file failed:{}", strerror(errno));
        return -1;
    }
    return fd;
}

int mutex_wrapper::named_mmap(const char* file_name, bool& is_new) {
    // 创建映射区文件，命名map
    int fd = get_fd(file_name, is_new);
    if (fd < 0) {
        return -1;
    }
    if (is_new) {
        int rc = ftruncate(fd, sizeof(multi_process_mutex));
        if (rc == -1) {
            LOG_ERROR("open file failed:{}", strerror(errno));
            return -1;
        }
    }
    mm = (multi_process_mutex*)mmap(NULL, sizeof(multi_process_mutex), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mm == NULL) {
        LOG_ERROR("mmap return empty pointer");
        return -1;
    }
    return 0;
}

int mutex_wrapper::anon_mmap() {
    // 匿名mmap
    mm = (multi_process_mutex*)mmap(NULL, sizeof(multi_process_mutex), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
    if (mm == NULL) {
        LOG_ERROR("mmap return empty pointer");
        return -1;
    }
    return 0;
}

int clear_files_in_directory(const char* dir)
{
    // 防止删除根目录
    if (strcmp(dir, "/") == 0) return -1;

    // These are data types defined in the "dirent" header
    DIR *theFolder = opendir(dir);
    struct dirent *next_file;
    char filepath[256];

    while ( (next_file = readdir(theFolder)) != NULL )
    {
        // skip "." and ".." entries
        if (strcmp(next_file->d_name, ".")==0 || strcmp(next_file->d_name, "..")==0)
            continue;

        // build the path for each file in the folder
        snprintf(filepath, strlen(dir) + strlen(next_file->d_name) + 2, "%s/%s", dir, next_file->d_name);
        remove(filepath);
    }
    closedir(theFolder);
    return 0;
}
