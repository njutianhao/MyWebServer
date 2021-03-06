#ifndef SERVER_H
#define SERVER_H
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<assert.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>
#include<map>
#include<unistd.h>
#include<signal.h>
#include"threadpool.h"
#include"http.h"
#include<sys/types.h>
#include<bits/signum.h>
#define MAX_EVENT_NUM 10000
#define TIMEOUT_VAL 30
class Server{
private:
    int port = 2132;
    const int backlog = 5;
    int listenfd;
    int epfd;
    static int sigpipe[2];
    epoll_event event[MAX_EVENT_NUM];
    ThreadPool tp;
public:
    Server();
    Server(int);
    void set_port(int);
    void start_listen();
    void event_loop();
    static void sighandler(int sig);
    friend class Timer;
};
#endif