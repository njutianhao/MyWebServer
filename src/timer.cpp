#include"timer.h"

Timer::Timer(int r,int p,UserData* d,Server *s){
    rotation = r;
    position = p;
    data = d;
    server = s;
}

int Timer::update(){
    if(rotation <= 0)
    {
        (*server).timeout_func(data);
        return 1;
    }
    rotation--;
    return 0;
}

int Timer::get_position(){
    return position;
}

TimerWheel::TimerWheel(Server *s){
    server = s;
    current = 0;
    for(int i = 0;i < SLOT_NUM;i++)
    {
        slots[i].clear();
    }
}

int TimerWheel::add_timer(UserData *data,int timeout){
    if(timeout < 0)
        return ;
    int ticks = timeout/SI;
    int rotation = ticks / SLOT_NUM;
    int position = ((ticks % SLOT_NUM)+current) % SLOT_NUM;
    slots[position].push_front(Timer(rotation,position,data,server));
}

void TimerWheel::tick(){
    for(std::list<Timer>::iterator i = slots[current].begin();i != slots[current].end();i++)
    {
        if(i->update() == 1)
        {
            i = slots[current].erase(i);
            i--;
        }
    }
    current++;
}

void TimerWheel::remove_timer(Timer *t){
    slots[t->get_position()].remove(*t);
}