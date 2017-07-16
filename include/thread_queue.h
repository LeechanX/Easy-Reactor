#ifndef __THREAD_QUEUE_H__
#define __THREAD_QUEUE_H__

#include <queue>
#include <pthread.h>

struct queue_msg
{
    enum MSG_TYPE
    {
        NEW_CONN,
        STOP_THD
    };
    MSG_TYPE cmd_type;
    int connfd;
};

class thread_queue
{
public:
    thread_queue();

    ~thread_queue();

    void send_msg(const queue_msg& item);

    void recv_msg(std::queue<queue_msg>& tmp_queue);

    int notifier() const { return _evfd; }

private:
    int _evfd;
    std::queue<queue_msg> _queue;
    pthread_mutex_t _mutex;
};

#endif