#include "event_loop.h"
#include "timer_queue.h"
#include "print_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>

void timerqueue_cb(event_loop* loop, int fd, void *args)
{
    std::vector<timer_event> fired_evs;
    loop->_timer_que->get_timo(fired_evs);
    for (std::vector<timer_event>::iterator it = fired_evs.begin();
        it != fired_evs.end(); ++it)
    {
        it->cb(loop, it->cb_data);
    }
}

event_loop::event_loop()
{
    _epfd = ::epoll_create1(0);
    exit_if(_epfd == -1, "when epoll_create1()");
    _timer_que = new timer_queue();
    exit_if(_timer_que == NULL, "when new timer_queue");
    //register timer event to event loop
    add_ioev(_timer_que->notifier(), timerqueue_cb, EPOLLIN, _timer_que);
}

void event_loop::process_evs()
{
    while (true)
    {
        //handle file event
        ioev_it it;
        int nfds = ::epoll_wait(_epfd, _fired_evs, MAXEVENTS, 10);
        for (int i = 0;i < nfds; ++i)
        {
            it = _io_evs.find(_fired_evs[i].data.fd);
            assert(it != _io_evs.end());
            io_event* ev = &(it->second);

            if (_fired_evs[i].events & EPOLLIN)
            {
                void *args = ev->rcb_args;
                ev->read_cb(this, _fired_evs[i].data.fd, args);
            }
            else if (_fired_evs[i].events & EPOLLOUT)
            {
                void *args = ev->wcb_args;
                ev->write_cb(this, _fired_evs[i].data.fd, args);
            }
            else if (_fired_evs[i].events & (EPOLLHUP | EPOLLERR))
            {
                if (ev->read_cb)
                {
                    void *args = ev->rcb_args;
                    ev->read_cb(this, _fired_evs[i].data.fd, args);
                }
                else if (ev->write_cb)
                {
                    void *args = ev->wcb_args;
                    ev->write_cb(this, _fired_evs[i].data.fd, args);
                }
                else
                {
                    error_log("fd %d get error, delete it from epoll", _fired_evs[i].data.fd);
                    del_ioev(_fired_evs[i].data.fd);
                }
            }
        }
        run_task();
    }
}

/*
 * if EPOLLIN in mask, EPOLLOUT must not in mask;
 * if EPOLLOUT in mask, EPOLLIN must not in mask;
 * if want to register EPOLLOUT | EPOLLIN event, just call add_ioev twice!
 */
void event_loop::add_ioev(int fd, io_callback* proc, int mask, void* args)
{
    int f_mask = 0;//finial mask
    int op;
    ioev_it it = _io_evs.find(fd);
    if (it == _io_evs.end())
    {
        f_mask = mask;
        op = EPOLL_CTL_ADD;
    }
    else
    {
        f_mask = it->second.mask | mask;
        op = EPOLL_CTL_MOD;
    }
    if (mask & EPOLLIN)
    {
        _io_evs[fd].read_cb = proc;
        _io_evs[fd].rcb_args = args;
    }
    else if (mask & EPOLLOUT)
    {
        _io_evs[fd].write_cb = proc;
        _io_evs[fd].wcb_args = args;
    }

    _io_evs[fd].mask = f_mask;
    struct epoll_event event;
    event.events = f_mask;
    event.data.fd = fd;
    int ret = ::epoll_ctl(_epfd, op, fd, &event);
    error_if(ret == -1, "epoll_ctl");
    listening.insert(fd);//加入到监听集合中
}

void event_loop::del_ioev(int fd, int mask)
{
    ioev_it it = _io_evs.find(fd);
    if (it == _io_evs.end())
        return ;
    int& o_mask = it->second.mask;
    int ret;
    //remove mask from o_mask
    o_mask = o_mask & (~mask);
    if (o_mask == 0)
    {
        _io_evs.erase(it);
        ret = ::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
        error_if(ret == -1, "epoll_ctl EPOLL_CTL_DEL");
        listening.erase(fd);//从监听集合中删除
    }
    else
    {
        struct epoll_event event;
        event.events = o_mask;
        event.data.fd = fd;
        ret = ::epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &event);
        error_if(ret == -1, "epoll_ctl EPOLL_CTL_MOD");
    }
}

void event_loop::del_ioev(int fd)
{
    _io_evs.erase(fd);
    listening.erase(fd);//从监听集合中删除
    ::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
}

int event_loop::run_at(timer_callback cb, void* args, uint64_t ts)
{
    timer_event te(cb, args, ts);
    return _timer_que->add_timer(te);
}

int event_loop::run_after(timer_callback cb, void* args, int sec, int millis)
{
    struct timespec tpc;
    clock_gettime(CLOCK_REALTIME, &tpc);
    uint64_t ts = tpc.tv_sec * 1000 + tpc.tv_nsec / 1000000UL;
    ts += sec * 1000 + millis;
    timer_event te(cb, args, ts);
    return _timer_que->add_timer(te);
}

int event_loop::run_every(timer_callback cb, void* args, int sec, int millis)
{
    uint32_t interval = sec * 1000 + millis;
    struct timespec tpc;
    clock_gettime(CLOCK_REALTIME, &tpc);
    uint64_t ts = tpc.tv_sec * 1000 + tpc.tv_nsec / 1000000UL + interval;
    timer_event te(cb, args, ts, interval);
    return _timer_que->add_timer(te);
}

void event_loop::del_timer(int timer_id)
{
    _timer_que->del_timer(timer_id);
}

void event_loop::add_task(pendingFunc func, void* args)
{
    std::pair<pendingFunc, void*> item(func, args);
    _pendingFactors.push_back(item);
}

void event_loop::run_task()
{
    std::vector<std::pair<pendingFunc, void*> >::iterator it;
    for (it = _pendingFactors.begin();it != _pendingFactors.end(); ++it)
    {
        pendingFunc func = it->first;
        void* args = it->second;
        func(this, args);
    }
    _pendingFactors.clear();
}
