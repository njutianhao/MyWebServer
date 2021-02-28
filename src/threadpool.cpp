#include"threadpool.h"
ThreadPool::ThreadPool() throw(){
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
        sem_init(&requests,0,0);
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
        UserData *data = queue.front();
        queue.pop_front();
        fd_exist.erase(data->socketfd);
        pthread_mutex_unlock(&mutex);
        if(user_conn.find(data->socketfd) == user_conn.end())
        {
            user_conn[data->socketfd] = new HttpConnection(data->socketfd);
        }
        user_conn[data->socketfd]->run();
    }
}