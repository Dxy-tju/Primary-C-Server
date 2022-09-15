#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<pthread.h>
#include<list>
// #include<locker.h>
#include "locker.h"
#include<cstdio>
#include<iostream>

template<typename T>
class threadpool{
public:
    threadpool(int thread_num = 8, int max_thread_num = 10000);
    ~threadpool();
    bool append(T* request);

private:
    static void* worker(void* arg);
    void run();
private:
    // 线程数组实体
    // 线程数量
    int m_thread_number;
    // 请求队列中最多允许的等待请求数量
    int m_max_requests;
    // 线程数组
    pthread_t* m_threads;

    // 请求队列
    std::list<T*> m_workqueue;

    // 线程通信私有成员
    // 互斥锁
    locker m_queuelocker;
    // 信号量，判断是否有任务要处理
    sem m_queuestat;
    // 是否结束线程
    bool m_stop;

};

template<typename T>
threadpool<T>::threadpool(int thread_num, int max_thread_num):
        m_thread_number(thread_num), m_max_requests(max_thread_num),
        m_stop(false), m_threads(NULL){
            if(thread_num<=0||max_thread_num<=0){
                throw std::exception();
            }
            m_threads = new pthread_t[max_thread_num];
            if(m_threads==nullptr){
                throw std::exception();
            }
            
            for(int i=0;i<thread_num;i++){
                std::cout<<"creating thread "<<i<<std::endl;
                if(pthread_create(m_threads+i, NULL, worker, this)!=0){
                    delete[] m_threads;
                    std::exception();
                }
                if(pthread_detach(m_threads[i])){
                    delete[] m_threads;
                    throw std::exception();
                }
            }
        }

template<typename T>
threadpool<T>::~threadpool() {
    delete [] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request) {
    m_queuelocker.lock();
    if(m_workqueue.size()>m_max_requests){
        m_queuelocker.unlock();
        return false;
    }

    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg){
    threadpool* pool = (threadpool*) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();

        if ( m_workqueue.empty() ) {
            m_queuelocker.unlock();
            continue;
        }

        T* request = m_workqueue.front();
        m_workqueue.pop_front();

        m_queuelocker.unlock();

        if (!request) {
            continue;
        }
        request->process();
    }
}

#endif