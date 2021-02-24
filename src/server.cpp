#include "server.h"

int setnonblocking(int fd) 
{ 
    int old_opt = fcntl(fd,F_GETFL);
    fcntl(fd,old_opt|O_NONBLOCK);
    return old_opt;
}

void addfd(int epfd, int fd)
{ 
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

Server::Server():tw(this){
    //TODO:
    ;
}

void Server::delfd(int fd){
    //TODO:
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,0);
    user_timer.erase(fd);
    close(fd);
}

void Server::end_connection(Timer &data){
    delfd(data.data->socketfd);
    tw.remove_timer(data);
}

bool Server::fd_exist(int fd){
    return ;
}

void Server::start_listen(){
    listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >= 0);
    sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htonl(port);
    int ret = bind(listenfd,(sockaddr *)&addr,sizeof(addr));
    assert(ret >= 0);
    ret = listen(listenfd,backlog);
    assert(ret >= 0);
    epfd = epoll_create(epfdsize);
    assert(epfd != -1);
    addfd(epfd,listenfd);
}

void Server::event_loop(){

    while(1)
    {
        // TODO: TIMEOUT
        int ret = epoll_wait(epfd,event,MAX_EVENT_NUM,-1);
        if(ret == -1)
        {
            perror("Error");
            return;
        }
        for(int i = 0;i < ret;i++)
        {
            if(event[i].data.fd == listenfd)
            {
                sockaddr_in addr;
                unsigned long addrlen = sizeof(addr);
                while(1)
                {
                    int connfd = accept(listenfd,(sockaddr *)&addr,(socklen_t *)&addrlen);
                    if(connfd < 0)
                        break;
                    //TODO:over user count
                    UserData data;
                    //TODO:buff?
                    data.addr = addr;
                    data.socketfd = connfd;
                    user_timer[connfd] = tw.add_timer(&data,TIMEOUT_VAL);
                }
            }
            else if(event[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
            {
                end_connection(user_timer[event[i].data.fd]);
            }
            else if(event[i].events & EPOLLIN)
            {
                //TODO:
            }
            else if(event[i].events & EPOLLOUT)
            {
                //TODO:
            }
        }
    }
}