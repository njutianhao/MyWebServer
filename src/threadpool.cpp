#include"threadpool.h"
ThreadPool::ThreadPool():tw(this){
    requests = 0;
    stop = false;
    for(int i = 0; i < THREAD_NUM;i++)
    {
        threads[i] = std::thread(start_routine,this);
        threads[i].detach();
    }
}

void ThreadPool::setepfd(int f){
    epfd = f;
}

void ThreadPool::tick(){
    timer_mutex.lock();
    tw.tick();
    timer_mutex.unlock();
}

void ThreadPool::add_timer(UserData data,int timeout){
    timer_mutex.lock();
    user_timer.insert(std::pair<int,std::list<Timer *>::iterator>(data.socketfd,tw.add_timer(data,timeout)));
    timer_mutex.unlock();
}

void ThreadPool::remove_timer(int fd){
    timer_mutex.lock();
    if(user_timer.find(fd) != user_timer.end())
    {
        tw.remove_timer(user_timer[fd]);
        user_timer.erase(fd);
    }
    timer_mutex.unlock();
}

void ThreadPool::remove_user_connection(int fd){
    conn_mutex.lock();
    if(user_conn.find(fd) != user_conn.end())
    {
        if(user_conn[fd] != NULL)
            delete user_conn[fd];
        user_conn.erase(fd);
    }
    conn_mutex.unlock();
}

void ThreadPool::end_connection(int fd){
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
    remove_timer(fd);
    remove_user_connection(fd);
    if(close(fd) != 0)
    {
        perror("Error");
        assert(0);
    }
}

void ThreadPool::append(epoll_event event){
    queue_mutex.lock();
    queue.push_back(event);
    sem.post();
    queue_mutex.unlock();
}

void ThreadPool::adjust_timer(int fd){
    timer_mutex.lock();
    if(user_timer.find(fd) != user_timer.end() && *user_timer[fd] != NULL)
        (**user_timer[fd]).rotation_plus();
    timer_mutex.unlock();
}

void * ThreadPool::start_routine(void *p){
    ThreadPool *pool = (ThreadPool *)p;
    pool->run();
    return pool;
}

void ThreadPool::run(){
    while(!stop){
        sem.wait();
        queue_mutex.lock();
        assert(!queue.empty());
        epoll_event event = queue.front();
        queue.pop_front();
        queue_mutex.unlock();
        int fd = event.data.fd;
        if(event.events & EPOLLIN)
        {
            //because of oneshot of epoll_event,no need to use exclusive lock to call run of user_conn[fd].
            conn_mutex.lock_shared();
            if(user_conn.find(fd) == user_conn.end())
            {
                conn_mutex.unlock_shared();
                conn_mutex.lock();
                user_conn[fd] = new HttpConnection(fd);
                assert(user_conn[fd] != NULL);
                conn_mutex.unlock();
                conn_mutex.lock_shared();
            }
            int ret = user_conn[fd]->run();
            conn_mutex.unlock_shared();
            if(ret == -1)
            {
                end_connection(fd);
            }
            else if(ret == 0)
            {
                epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                adjust_timer(fd);
            }
            else if(ret == 1)
            {
                epoll_event event;
                event.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                adjust_timer(fd);
            }
        }
        else if(event.events & EPOLLOUT)
        {
            conn_mutex.lock_shared();
            if(user_conn.find(fd) == user_conn.end()){
                std::cerr << "can't find user_conn when send\n";
                assert(0);
            }
            int ret = (*user_conn[fd]).send();
            conn_mutex.unlock_shared();
            debug("thread %d send data to fd %d returns %d\n",gettid(),fd,ret);
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
                adjust_timer(fd);
            }
            else if(ret == 1)
            {
                epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
                adjust_timer(fd);
            }
        }
    }
}