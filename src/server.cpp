#include "server.h"
int Server::sigpipe[2] = {0};
int setnonblocking(int fd) 
{ 
    int old_opt = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,old_opt|O_NONBLOCK);
    return old_opt;
}

void addfd(int epfd, int fd,bool oneshot)
{ 
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN|EPOLLET;
    if(oneshot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void addsig(int sig) { 
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler= Server::sighandler;
    sa.sa_flags|= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa, NULL)!=-1);
}


void Server::sighandler(int sig){
    send(sigpipe[1],(char *)&sig,1,0);
}

Server::Server(){
}

Server::Server(int p){
    port = p;
}

void Server::set_port(int p){
    port = p;
}

void Server::start_listen(){
    listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >= 0);
    sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    int ret = bind(listenfd,(sockaddr *)&addr,sizeof(addr));
    if(ret == -1)
        perror("");
    assert(ret >= 0);
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT,&flag,sizeof(flag));
    ret = listen(listenfd,backlog);
    assert(ret >= 0);
    epfd = epoll_create(5);
    assert(epfd != -1);
    addfd(epfd,listenfd,false);
    tp.setepfd(epfd);
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,sigpipe);
    assert(ret != -1);
    addfd(epfd,sigpipe[0],false);
    addsig(SIGALRM);
    addsig(SIGTERM);
    addsig(SIGTERM);

}

void Server::event_loop(){
    bool alarm_triggered = false;
    bool stop = false;
    alarm(SLOT_INTERVAL);
    while(!stop)
    {
        int ret = epoll_wait(epfd,event,MAX_EVENT_NUM,-1);
        if(ret == -1 && errno != EINTR)
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
                    if(connfd == -1 && errno == EAGAIN)
                        break;
                    else if(connfd <= 0)
                    {
                        perror("Error");
                        return ;
                    }
                    debug("accept connect %d\n",connfd);
                    UserData data;
                    data.addr = addr;
                    data.socketfd = connfd;
                    tp.add_timer(data,TIMEOUT_VAL);
                    addfd(epfd,connfd,true);
                }
            }
            else if(event[i].data.fd == sigpipe[0] && (event[i].events & EPOLLIN)) 
            {
                char sig[1024];
                int ret = recv(sigpipe[0],sig,1024,0);
                if(ret > 0)
                {
                    for(int i = 0;i < ret;i++)
                    {
                        switch(sig[i])
                        {
                            case SIGALRM:alarm_triggered = true;break;
                            case SIGINT:case SIGTERM: stop = true;break;
                            default: ;
                        }
                    }
                }
            }
            else if(event[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
            {
                tp.end_connection(event[i].data.fd);
            }
            else if(event[i].events & EPOLLIN)
            {
                tp.append(event[i]);
            }
            else if(event[i].events & EPOLLOUT)
            {
                tp.append(event[i]);
            }
        }
        if(alarm_triggered)
        {
            alarm_triggered = false;
            alarm(SLOT_INTERVAL);
            tp.tick();
        }
    }
}