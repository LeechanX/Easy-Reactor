#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "msg_head.h"
#include "thread_queue.h"
#include <pthread.h>

extern void* thread_domain(void* args);

class thread_pool
{
public:
    thread_pool(int thread_cnt);

    //~thread_pool();
    //由于thread_pool是给tcp_server类使用的，而tcp_server类使用时往往具有程序的完全生命周期，即程序退出时会自动释放内存,不需要析构函数去delete内存

    thread_queue<queue_msg>* get_next_thread();

    void run_task(int thd_index, pendingFunc task, void* args = NULL);

    void run_task(pendingFunc task, void* args = NULL);

private:
    int _curr_index;
    int _thread_cnt;
    thread_queue<queue_msg>** _pool;
    pthread_t* _tids;
};

#endif
