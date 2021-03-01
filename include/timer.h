#ifndef TIMER_H
#define TIMER_H
#include<netinet/in.h>
#include<time.h>
#include<list>
#include<array>
#include"server.h"
#include"threadpool.h"
#define SLOT_NUM 10
#define SI 1
class Timer;
struct UserData{
    sockaddr_in addr;
    int socketfd;
    Timer *timer;
};
class Timer{
private:
    int rotation;
    int position;
    UserData *data;
    ThreadPool *tp;
public:
    Timer(int,int,UserData *,ThreadPool *);
    int update();
    int get_position();
    void rotation_plus();
};
class TimerWheel{
private:
    std::array<std::list<Timer>,SLOT_NUM> slots;
    int current;
    ThreadPool *tp;
public:
    TimerWheel(ThreadPool *);
    Timer &add_timer(UserData *data,int timeout);
    void remove_timer(Timer &);
    void tick();
};

#endif