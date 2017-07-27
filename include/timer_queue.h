#ifndef __TIMER_QUEUE_H__
#define __TIMER_QUEUE_H__

#include <stdint.h>
#include <vector>
#include <ext/hash_map>
#include "event_base.h"

class timer_queue
{
public:
    timer_queue();
    ~timer_queue();

    int add_timer(timer_event& te);

    void del_timer(int timer_id);

    int notifier() const { return _timerfd; }
    int size() const { return _count; }

    void get_timo(std::vector<timer_event>& fired_evs);
private:
    void reset_timo();

    //heap operation
    void heap_add(timer_event& te);
    void heap_del(int pos);
    void heap_pop();
    void heap_hold(int pos);

    std::vector<timer_event> _event_lst;
    typedef std::vector<timer_event>::iterator vit;

    __gnu_cxx::hash_map<int, int> _position;

    typedef __gnu_cxx::hash_map<int, int>::iterator mit;
    
    int _count;
    int _next_timer_id;
    int _timerfd;
    uint64_t _pioneer;//recent timer's millis
};

#endif
