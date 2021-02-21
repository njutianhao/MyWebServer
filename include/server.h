#ifndef SERVER_H
#define SERVER_H
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<assert.h>
class Server{
private:
    int port;
public:
    Server();
    void start_listen();
};
#endif