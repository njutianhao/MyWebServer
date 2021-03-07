#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include"timer.h"
#include<thread>
#include<semaphore.h>
#include<iostream>
#include<map>
#include"http.h"
#include<sys/epoll.h>
#include<sys/types.h>
#include<assert.h>
#include<mutex>
#include<shared_mutex>
#include"sem.h"
#define THREAD_NUM 8
class ThreadPool{
private:
    std::list<epoll_event> queue;
    int epfd;
    std::thread threads[THREAD_NUM];
    std::mutex queue_mutex;
    std::shared_mutex timer_mutex;
    std::shared_mutex conn_mutex;
    semaphore sem;
    TimerWheel tw;
    std::map<int,HttpConnection *> user_conn;
    std::map<int,std::list<Timer *>::iterator> user_timer;
    int requests;
    bool stop;
public:
    ThreadPool();
    void setepfd(int);
    void delfd(int);
    void remove_timer(int);
    void remove_user_connection(int);
    void end_connection(int);
    void add_timer(UserData data,int timeout);
    void append(epoll_event);
    void adjust_timer(int);
    static void *start_routine(void *);
    void run();
    void tick();
};
#endif