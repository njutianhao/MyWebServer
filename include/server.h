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
#include"timer.h"
#include<map>
#include<unistd.h>
#define MAX_EVENT_NUM 10000
#define TIMEOUT_VAL 180
class Server{
private:
    int port;
    const int backlog = 128;
    const int epfdsize = 128;
    int listenfd;
    int epfd;
    epoll_event event[MAX_EVENT_NUM];
    TimerWheel tw;
    std::map<int,Timer> user_timer;
public:
    Server();
    void start_listen();
    void event_loop();
    void delfd(int);
    void end_connection(Timer &);
    bool fd_exist(int fd);
    friend class Timer;
};
#endif