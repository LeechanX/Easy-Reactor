#ifndef __THREAD_QUEUE_H__
#define __THREAD_QUEUE_H__

#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <algorithm>
#include <sys/eventfd.h>
#include "event_loop.h"

template <typename T> 
class thread_queue
{
public:
    thread_queue(): _loop(NULL)
    {
        ::pthread_mutex_init(&_mutex, NULL);
        _evfd = ::eventfd(0, EFD_NONBLOCK);
        if (_evfd == -1)
        {
            perror("eventfd(0, EFD_NONBLOCK)");
            ::exit(1);
        }
    }

    ~thread_queue()
    {
        ::pthread_mutex_destroy(&_mutex);
        ::close(_evfd);
    }

    void send_msg(const T& item)
    {
        unsigned long long number = 1;
        ::pthread_mutex_lock(&_mutex);
        _queue.push(item);
        int ret = ::write(_evfd, &number, sizeof(unsigned long long));
        if (ret == -1) perror("eventfd write");
        ::pthread_mutex_unlock(&_mutex);
    }

    void recv_msg(std::queue<T>& tmp_queue)
    {
        unsigned long long number;
        ::pthread_mutex_lock(&_mutex);
        int ret = ::read(_evfd, &number, sizeof(unsigned long long));
        if (ret == -1) perror("eventfd read");
        std::swap(tmp_queue, _queue);
        ::pthread_mutex_unlock(&_mutex);
    }

    event_loop* get_loop() { return _loop; }

    //set loop and install message comming event's callback: proc
    void set_loop(event_loop* loop, io_callback* proc, void* args = NULL)
    {
        _loop = loop;
        _loop->add_ioev(_evfd, proc, EPOLLIN, args);
    }

private:
    int _evfd;
    event_loop* _loop;
    std::queue<T> _queue;
    pthread_mutex_t _mutex;
};
#endif
