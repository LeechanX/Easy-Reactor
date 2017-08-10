#ifndef __MSG_DISPATCHER_H__
#define __MSG_DISPATCHER_H__

#include <assert.h>
#include <pthread.h>
#include <ext/hash_map>
#include "net_commu.h"

typedef void msg_callback(const char* data, uint32_t len, int cmdid, net_commu* commu, void* usr_data);

class msg_dispatcher
{
public:
    msg_dispatcher() {}

    int add_msg_cb(int cmdid, msg_callback* msg_cb, void* usr_data)
    {
        if (_dispatcher.find(cmdid) != _dispatcher.end()) return -1;
        _dispatcher[cmdid] = msg_cb;
        _args[cmdid] = usr_data;
        return 0;
    }

    bool exist(int cmdid) const { return _dispatcher.find(cmdid) != _dispatcher.end(); }

    void cb(const char* data, uint32_t len, int cmdid, net_commu* commu)
    {
        assert(exist(cmdid));
        msg_callback* func = _dispatcher[cmdid];
        void* usr_data = _args[cmdid];
        func(data, len, cmdid, commu, usr_data);
    }

private:
    __gnu_cxx::hash_map<int, msg_callback*> _dispatcher;
    __gnu_cxx::hash_map<int, void*> _args;
};

#endif
