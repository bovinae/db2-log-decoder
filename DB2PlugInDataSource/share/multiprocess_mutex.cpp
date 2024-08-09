#include "multiprocess_mutex.h"
#include "log_imp.h"

static struct multi_process_mutex* mm = NULL;

void init_multi_process_mutex(bool init_mutex) {
    LOG_DEBUG("init_multi_process_mutex");
    // 创建映射区文件
    // int fd = open("multi_process_mutex",O_CREAT|O_RDWR,0777);
    // if( fd == -1 ) {
    //     LOG_ERROR("open file failed:{}", strerror(errno));
    //     perror("open file:"); 
    //     exit(1); 
    // }
    // ftruncate(fd,sizeof(struct multi_process_mutex));
    // mm = (multi_process_mutex*)mmap(NULL,sizeof(struct multi_process_mutex),PROT_READ|PROT_WRITE,MAP_SHARED ,fd,0);
    // close(fd);
    // unlink("multi_process_mutex");

    mm = (multi_process_mutex*)mmap(NULL,sizeof(struct multi_process_mutex),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANON,-1,0);
    if (init_mutex) {
        memset(mm,0x00,sizeof(struct multi_process_mutex));
        pthread_mutexattr_init(&mm->mutexattr);         // 初始化 mutex 属性
        pthread_mutexattr_setpshared(&mm->mutexattr, PTHREAD_PROCESS_SHARED);               // 修改属性为进程间共享
        pthread_mutex_init(&mm->mutex,&mm->mutexattr);      // 初始化一把 mutex 锁
    }

    LOG_DEBUG("init_multi_process_mutex ok");
}

void destroy_multi_process_mutex() {
    LOG_DEBUG("destroy_multi_process_mutex");
    pthread_mutexattr_destroy(&mm->mutexattr);  // 销毁 mutex 属性对象
    pthread_mutex_destroy(&mm->mutex);          // 销毁 mutex 锁
    munmap(mm, sizeof(struct multi_process_mutex));
    LOG_DEBUG("destroy_multi_process_mutex ok");
}

void multi_process_mutex_lock() {
    LOG_DEBUG("multi_process_mutex_lock");
    pthread_mutex_lock(&mm->mutex);
    LOG_DEBUG("multi_process_mutex_lock ok");
}

void multi_process_mutex_unlock() {
    LOG_DEBUG("multi_process_mutex_unlock");
    pthread_mutex_unlock(&mm->mutex);
    LOG_DEBUG("multi_process_mutex_unlock ok");
}
