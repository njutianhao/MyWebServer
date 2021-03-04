#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include"timer.h"
#include<thread>
#include<semaphore.h>
#include<map>
#include"http.h"
#include<sys/epoll.h>
#include<assert.h>
#include<mutex>
class ThreadPool{
private:
    std::list<epoll_event *> queue;
    const int THREAD_NUM = 8;
    int epfd;
    std::vector<std::thread> threads;
    std::mutex queue_mutex;
    std::mutex timer_mutex;
    TimerWheel tw;
    std::map<int,HttpConnection *> user_conn;
    std::map<int,std::list<Timer>::iterator> user_timer;
    int requests;
    bool stop ;
public:
    ThreadPool();
    void setepfd(int);
    void delfd(int);
    void remove_timer(int);
    void remove_user_connection(int);
    void end_connection(int);
    void add_timer(UserData *data,int timeout);
    void append(epoll_event *);
    static void *start_routine(void *);
    void run();
    void tick();
};
#endif