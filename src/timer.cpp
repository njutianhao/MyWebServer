#include"timer.h"

int callback(UserData *d){
    return 0;
}

Timer::Timer(int r,UserData* d,int (*c)(UserData *)){
    rotation = r;
    data = d;
    callback = c;
}
int Timer::update(){
    if(rotation <= 0)
    {
        (*callback)(data);
        return 1;
    }
    rotation--;
    return 0;
}
TimerWheel::TimerWheel(){
    current = 0;
    for(int i = 0;i < SLOT_NUM;i++)
    {
        slots[i].clear();
    }
}
int TimerWheel::add_timer(UserData data,int timeout){
    if(timeout < 0)
        return ;
    int ticks = timeout/SI;
    int rotation = ticks / SLOT_NUM;
    int position = ((ticks % SLOT_NUM)+current) % SLOT_NUM;
    slots[position].push_front(Timer(rotation,&data,callback));
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