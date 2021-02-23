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
#define MAX_EVENT_NUM 10000
class Server{
private:
    int port;
    const int backlog = 128;
    const int epfdsize = 128;
    int listenfd;
    int epfd;
    epoll_event event[MAX_EVENT_NUM];
public:
    Server();
    void start_listen();
    void event_loop();
};
#endif