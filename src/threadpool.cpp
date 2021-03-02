#include"threadpool.h"
ThreadPool::ThreadPool() throw():tw(this){
    stop = false;
    for(int i = 0; i < THREAD_NUM;i++)
    {
        int ret = pthread_create(threads+i,NULL,start_routine,NULL);
        assert(ret == 0);
        ret = pthread_detach(threads[i]);
        assert(ret == 0);
    }
    try
    {
        pthread_mutex_init(&mutex,NULL);
        pthread_mutex_init(&timer_mutex,NULL);
        sem_init(&requests,0,0);
    }
    catch(...){
        throw std::exception();
    }
}

void ThreadPool::setepfd(int f){
    epfd = f;
}

void ThreadPool::add_timer(UserData *data,int timeout){
    pthread_mutex_lock(&timer_mutex);
    user_timer[data->socketfd] = tw.add_timer(data,timeout);
    pthread_mutex_unlock(&timer_mutex);
}

void ThreadPool::remove_timer(int fd){
    pthread_mutex_lock(&timer_mutex);
    tw.remove_timer(user_timer[fd]);
    pthread_mutex_unlock(&timer_mutex);
}

void ThreadPool::remove_connection(int fd){
    pthread_mutex_lock(&user_conn[fd]->mutex);
    user_conn.erase(fd);
    pthread_mutex_unlock(&user_conn[fd]->mutex);
}

void ThreadPool::append(epoll_event event){
    pthread_mutex_lock(&mutex);
    queue.push_back(event);
    pthread_mutex_unlock(&mutex);
    sem_post(&requests);
}

void * ThreadPool::start_routine(void *p){
    ThreadPool *pool = (ThreadPool *)p;
    pool->run();
    return pool;
}

void ThreadPool::run(){
    while(!stop){
        sem_wait(&requests);
        pthread_mutex_lock(&mutex);
        if(queue.empty())
        {
            pthread_mutex_unlock(&mutex);
            sem_post(&requests); //TODO:check 
            continue;
        }
        epoll_event event = queue.front();
        queue.pop_front();
        pthread_mutex_unlock(&mutex);
        int fd = event.data.fd;
        if(event.events & EPOLLIN)
        {
            if(user_conn.find(fd) == user_conn.end())
            {
                user_conn[fd] = new HttpConnection(fd);
            }
            pthread_mutex_lock(&user_conn[fd]->mutex);
            int ret = user_conn[fd]->run();
            if(ret == -1)
            {
                user_conn.erase(fd);
                pthread_mutex_unlock(&user_conn[fd]->mutex);
                epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                remove_timer(fd);
                close(fd);
            }
            else if(ret == 0)
            {
                pthread_mutex_unlock(&user_conn[fd]->mutex);
                epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
            }
            else if(ret == 1)
            {
                epoll_event event;
                event.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
            }
        }
        else if(event.events & EPOLLOUT)
        {
            assert(user_conn.find(fd) != user_conn.end());
            int ret = (*user_conn[fd]).send();
            if(ret == -1)
            {
                epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                remove_timer(fd);
                remove_connection(fd);
                close(fd);
            }
            else if(ret == 0)
            {
                epoll_event event;
                event.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
            }
            else if(ret == 1)
            {
                epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.fd = fd;
                epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
            }
        }
    }
}