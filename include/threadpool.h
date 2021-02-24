#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<unordered_map>
#include<list>
#include"timer.h"
#include<pthread.h>
#include<semaphore.h>
class ThreadPool{
private:
    std::list<UserData *> queue;
    std::unordered_map<int,bool> fd_exist;
    const int THREAD_NUM = 8;
    pthread_t *threads;
    pthread_mutex_t mutex;
    sem_t requests;
    bool stop = false;
    bool exist(int);
public:
    ThreadPool() throw();
    void append(UserData *);
    static void *start_routine(void *);
    void run();
};
#endif