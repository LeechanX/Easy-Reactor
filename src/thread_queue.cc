#include <algorithm>
#include <unistd.h>
#include <sys/eventfd.h>
#include "thread_queue.h"

thread_queue::thread_queue()
{
    ::pthread_mutex_init(&_mutex, NULL);
    _evfd = ::eventfd(0, EFD_NONBLOCK);
    //exit_if
}

thread_queue::~thread_queue()
{
    ::pthread_mutex_destroy(&_mutex);
    ::close(_evfd);
}

void thread_queue::send_msg(const queue_msg& item)
{
    unsigned long long number = 1;
    ::pthread_mutex_lock(&_mutex);
    _queue.push(item);
    int ret = ::write(_evfd, &number, sizeof(unsigned long long));
    //report_if
    ::pthread_mutex_unlock(&_mutex);
}

void thread_queue::recv_msg(std::queue<queue_msg>& tmp_queue)
{
    unsigned long long number;
    ::pthread_mutex_lock(&_mutex);
    int ret = ::read(_evfd, &number, sizeof(unsigned long long));
    //report_if
    std::swap(tmp_queue, _queue);
    ::pthread_mutex_unlock(&_mutex);
}
