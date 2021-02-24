#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<unordered_map>
#include<list>
#include"timer.h"
#include<pthread.h>
class ThreadPool{
private:
    std::list<UserData *> queue;
    std::unordered_map<int,bool> fd_exist;
    const int THREAD_NUM = 8;
    pthread_t *threads;
    pthread_mutex_t mutex;
    bool stop = false;
    bool exist(int);
public:
    ThreadPool();
    void append(UserData *);
    static void *start_routine(void *);
    void run();
};
#endif