#include"threadpool.h"
ThreadPool::ThreadPool():tw(this){
    requests = 0;
    stop = false;
    for(int i = 0; i < THREAD_NUM;i++)
    {
        threads.push_back(std::thread(start_routine,this));
        threads.back().detach();
    }
}

void ThreadPool::setepfd(int f){
    epfd = f;
}

void ThreadPool::tick(){
    tw.tick();
}

void ThreadPool::add_timer(UserData *data,int timeout){
    timer_mutex.lock();
    user_timer[data->socketfd] = tw.add_timer(data,timeout);
    timer_mutex.unlock();
}

void ThreadPool::remove_timer(int fd){
    timer_mutex.lock();
    tw.remove_timer(user_timer[fd]);
    timer_mutex.unlock();
}

void ThreadPool::remove_user_connection(int fd){
    user_conn[fd]->mutex.lock();
    user_conn.erase(fd);
    user_conn[fd]->mutex.unlock();
}

void ThreadPool::end_connection(int fd){
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
    remove_timer(fd);
    remove_user_connection(fd);
    close(fd);
}

void ThreadPool::append(epoll_event *event){
    queue_mutex.lock();
    queue.push_back(event);
    requests++;
    queue_mutex.unlock();
}

void * ThreadPool::start_routine(void *p){
    ThreadPool *pool = (ThreadPool *)p;
    pool->run();
    return pool;
}

void ThreadPool::run(){
    while(!stop){
        queue_mutex.lock();
        if(requests <= 0 )
        {
            queue_mutex.unlock();
            continue;
        }
        epoll_event *event = queue.front();
        queue.pop_front();
        queue_mutex.unlock();
        int fd = event->data.fd;
        if(event->events & EPOLLIN)
        {
            if(user_conn.find(fd) == user_conn.end())
            {
                user_conn[fd] = new HttpConnection(fd);
            }
            user_conn[fd]->mutex.lock();
            int ret = user_conn[fd]->run();
            if(ret == -1)
            {
                user_conn.erase(fd);
                user_conn[fd]->mutex.unlock();
                epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                remove_timer(fd);
                close(fd);
            }
            else if(ret == 0)
            {
                user_conn[fd]->mutex.unlock();
                epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                (*user_timer[fd]).rotation_plus();
            }
            else if(ret == 1)
            {
                user_conn[fd]->mutex.unlock();
                epoll_event event;
                event.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                (*user_timer[fd]).rotation_plus();
            }
        }
        else if(event->events & EPOLLOUT)
        {
            assert(user_conn.find(fd) != user_conn.end());
            int ret = (*user_conn[fd]).send();
            if(ret == -1)
            {
                end_connection(fd);
            }
            else if(ret == 0)
            {
                epoll_event event;
                event.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                (*user_timer[fd]).rotation_plus();
            }
            else if(ret == 1)
            {
                epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                (*user_timer[fd]).rotation_plus();
            }
        }
    }
}