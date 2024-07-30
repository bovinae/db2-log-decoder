/*
    互斥量 实现 多进程 之间的同步 
*/

#pragma once

#include<unistd.h>
#include<sys/mman.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

struct multi_process_mutex
{
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;
};

void init_multi_process_mutex(bool init_mutex = false);

void destroy_multi_process_mutex();

void multi_process_mutex_lock();

void multi_process_mutex_unlock();

