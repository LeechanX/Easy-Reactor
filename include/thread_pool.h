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

    ~thread_pool();

    thread_queue<queue_msg>* get_next_thread();

private:
    int _curr_index;
    int _thread_cnt;
    thread_queue<queue_msg>** _pool;
    pthread_t* _tids;
};

#endif
