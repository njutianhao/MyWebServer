#ifndef TIMER_H
#define TIMER_H
#include<netinet/in.h>
#include<time.h>
#include<list>
#include<array>
#include"server.h"
#define BUFFER_SIZE 64
#define SLOT_NUM 60
#define SI 1
class Timer;
struct UserData{
    sockaddr_in addr;
    int socketfd;
    char buff[BUFFER_SIZE];
    Timer *timer;
};
class Timer{
private:
    int rotation;
    int position;
    UserData *data;
    Server *server;
public:
    Timer(int,int,UserData *,Server *server);
    int update();
    int get_position();
    friend class Server;
};
class TimerWheel{
private:
    std::array<std::list<Timer>,SLOT_NUM> slots;
    int current;
    Server *server;
public:
    TimerWheel(Server *server);
    Timer add_timer(UserData *data,int timeout);
    void remove_timer(Timer &);
    void tick();
};

#endif