#ifndef TIMER_H
#define TIMER_H
#include<netinet/in.h>
#include<time.h>
#include<list>
#include<array>
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
    UserData *data;
    int (*callback)(UserData *data);
public:
    Timer(int,UserData *,int (UserData *));
    int update();
};
class TimerWheel{
private:
    std::array<std::list<Timer>,SLOT_NUM> slots;
    int current;
public:
    TimerWheel();
    int add_timer(UserData data,int timeout);
    void tick();
};
#endif