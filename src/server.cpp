#include "server.h"
Server::Server(){
    
}

void Server::start_listen(){
    int listenfd = socket(PF_INET,SOCK_STREAM,0);
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
}