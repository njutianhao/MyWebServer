#include"threadpool.h"
ThreadPool::ThreadPool(){
    for(int i = 0; i < THREAD_NUM;i++)
    {
        int ret = pthread_create(threads+i,NULL,start_routine,NULL);
        assert(ret == 0);
    }
    try
    {
        pthread_mutex_init(&mutex,NULL);
    }
    catch(...){
        throw std::exception();
    }
}

bool ThreadPool::exist(int fd){
    return fd_exist.find(fd) != fd_exist.end(); 
}

void ThreadPool::append(UserData *data){
    int fd = data->socketfd;
    pthread_mutex_lock(&mutex);
    if(!exist(fd))
    {
        fd_exist[fd] = true;
        queue.push_back(data);
    }
    pthread_mutex_unlock(&mutex);
}

void * ThreadPool::start_routine(void *p){
    ThreadPool *pool = (ThreadPool *)p;
    pool->run();
    return pool;
}

void ThreadPool::run(){
    while(!stop){
        pthread_mutex_lock(&mutex);
        UserData *data = queue.front();
        queue.pop_front();
        fd_exist.erase(data->socketfd);
        pthread_mutex_unlock(&mutex);
        //TODO:process data
    }
}