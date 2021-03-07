#ifndef TIMER_H
#define TIMER_H
#include<netinet/in.h>
#include<time.h>
#include<list>
#include<array>
#define SLOT_NUM 10
#define SLOT_INTERVAL 3
class ThreadPool;
class Timer;
struct UserData{
    sockaddr_in addr;
    int socketfd;
};
class Timer{
private:
    int rotation;
    int position;
    UserData data;
    ThreadPool *tp;
public:
    Timer(int,int,UserData ,ThreadPool *);
    int update();
    int get_position();
    void rotation_plus();
};
class TimerWheel{
private:
    std::list<Timer *> slots[SLOT_NUM];
    int current;
    ThreadPool *tp;
public:
    TimerWheel(ThreadPool *);
    std::list<Timer *>::iterator add_timer(UserData data,int timeout);
    void remove_timer(std::list<Timer *>::iterator it);
    void tick();
};
#endif