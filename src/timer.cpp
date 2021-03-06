#include"timer.h"
#include"threadpool.h"

Timer::Timer(int r,int p,UserData d,ThreadPool *t){
    rotation = r;
    position = p;
    data = d;
    tp = t;
}

int Timer::update(){
    if(rotation <= 0)
    {
        (*tp).end_connection(data.socketfd);
        return 1;
    }
    rotation--;
    return 0;
}

int Timer::get_position(){
    return position;
}

void Timer::rotation_plus(){
    rotation++;
}

TimerWheel::TimerWheel(ThreadPool *t){
    tp = t;
    current = 0;
}

std::list<Timer>::iterator TimerWheel::add_timer(UserData data,int timeout){
    assert(timeout >= 0);
    int ticks = timeout/SLOT_INTERVAL;
    int rotation = ticks / SLOT_NUM;
    int position = ((ticks % SLOT_NUM)+current) % SLOT_NUM;
    slots[position].push_front(Timer(rotation,position,data,tp));
    return slots[position].begin();
}

void TimerWheel::tick(){
    std::list<Timer>::iterator prev = slots[current].end();
    for(std::list<Timer>::iterator i = slots[current].begin();i != slots[current].end();i++)
    {
        if(i->update() == 1)
        {
            i = prev;
        }
        else
        {
            prev = i;
        }   
    }
    current = (current + 1) % SLOT_NUM;
}

void TimerWheel::remove_timer(std::list<Timer>::iterator it){
    slots[it->get_position()].erase(it);
}