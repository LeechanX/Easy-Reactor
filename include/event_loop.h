#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

#include "event_base.h"
#include "timer_queue.h"
#include <sys/epoll.h>
#include <ext/hash_map>
#include <ext/hash_set>

#define MAXEVENTS 10

class event_loop
{
public:
    event_loop();
    void process_evs();

    //operator for IO event
    void add_ioev(int fd, io_callback* proc, int mask, void* args = NULL);
    //delete only mask event for fd in epoll
    void del_ioev(int fd, int mask);
    //delete event for fd in epoll
    void del_ioev(int fd);
    //get all fds this loop is listening
    void nlistenings(__gnu_cxx::hash_set<int>& conns) { conns = listening; }

    //operator for timer event
    int run_at(timer_callback cb, void* args, uint64_t ts);
    int run_after(timer_callback cb, void* args, int sec, int millis = 0);
    int run_every(timer_callback cb, void* args, int sec, int millis = 0);
    void del_timer(int timer_id);

    void add_task(pendingFunc func, void* args);
    void run_task();

private:
    int _epfd;
    struct epoll_event _fired_evs[MAXEVENTS];
    //map: fd->io_event
    __gnu_cxx::hash_map<int, io_event> _io_evs;
    typedef __gnu_cxx::hash_map<int, io_event>::iterator ioev_it;
    timer_queue* _timer_que;
    //此队列用于:暂存将要执行的任务
    std::vector<std::pair<pendingFunc, void*> > _pendingFactors;

    __gnu_cxx::hash_set<int> listening;

    friend void timerqueue_cb(event_loop* loop, int fd, void *args);
};

#endif
