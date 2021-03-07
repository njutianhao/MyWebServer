 #ifndef SEM_H
 #define SEM_H
 #include<iostream>
 #include<thread>
 #include<mutex>
 #include<condition_variable>
class semaphore{
private:
    int count;
    std::mutex mutex;
    std::condition_variable cv;
public:
    semaphore(int value = 0){
        count = value;
    }
    void wait(){
        std::unique_lock<std::mutex> lck(mutex);
        if(--count < 0)
            cv.wait(lck);
    }
 
    void post(){
        std::unique_lock<std::mutex> lck(mutex);
        if(++count <= 0)
            cv.notify_one();
    }
};
#endif