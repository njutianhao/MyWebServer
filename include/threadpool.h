#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include<unordered_map>
#include"timer.h"
#include<pthread.h>
#include<semaphore.h>
class ThreadPool{
private:
    std::list<int> queue;
    const int THREAD_NUM = 8;
    pthread_t *threads;
    pthread_mutex_t mutex;
    pthread_mutex_t timer_mutex;
    TimerWheel tw;
    std::unordered_map<int,bool> fd_exist;
    std::map<int,HttpConnection *> user_conn;
    std::map<int,Timer&> user_timer;
    sem_t requests;
    bool stop ;
public:
    ThreadPool() throw();
    void delfd(int);
    void remove_timer(int);
    void remove_connection(int);
    void add_timer(UserData *data,int timeout);
    void append(int);
    static void *start_routine(void *);
    void run();
};
#endif