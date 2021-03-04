#ifndef TIMER_H
#define TIMER_H
#include<netinet/in.h>
#include<time.h>
#include<list>
#include<array>
#define SLOT_NUM 10
#define SI 3
class ThreadPool;
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
    std::list<Timer>::iterator add_timer(UserData *data,int timeout);
    void remove_timer(std::list<Timer>::iterator it);
    void tick();
};

#endif