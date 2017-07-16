#include "event_loop.h"
#include "timer_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>

void timerqueue_cb(event_loop* loop, int fd, void *args)
{
    std::vector<timer_event> fired_evs;
    loop->_timer_que->get_timo_ev(fired_evs);
    for (std::vector<timer_event>::iterator it = fired_evs.begin();
        it != fired_evs.end(); ++it)
    {
        it->cb(loop, it->cb_data);
    }
}

event_loop::event_loop()
{
    _epfd = ::epoll_create1(0);
    if (_epfd == -1)
    {
        fprintf(stderr, "epoll_create1 return error %s", strerror(errno));
        exit(1);
    }
    _timer_que = new timer_queue();
    //exit if
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
                    fprintf(stderr, "fd %d get error, delete it from epoll\n", _fired_evs[i].data.fd);
                    del_ioev(_fired_evs[i].data.fd);
                }
            }
        }
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
    if (::epoll_ctl(_epfd, op, fd, &event) == -1)
        fprintf(stderr, "epoll_ctl error: %s\n", strerror(errno));
}

void event_loop::del_ioev(int fd, int mask)
{
    ioev_it it = _io_evs.find(fd);
    if (it == _io_evs.end())
        return ;
    int& o_mask = it->second.mask;
    //remove mask from o_mask
    o_mask = o_mask & (~mask);
    if (o_mask == 0)
    {
        _io_evs.erase(it);
        if (::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
            fprintf(stderr, "epoll_ctl EPOLL_CTL_DEL error: %s\n", strerror(errno));
    }
    else
    {
        struct epoll_event event;
        event.events = o_mask;
        event.data.fd = fd;
        if (::epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &event) == -1)
            fprintf(stderr, "epoll_ctl EPOLL_CTL_MOD error: %s\n", strerror(errno));
    }
}

void event_loop::del_ioev(int fd)
{
    _io_evs.erase(fd);
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

void event_loop::cancel_timer(int timer_id)
{
    _timer_que->cancel_timer(timer_id);
}