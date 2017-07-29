#ifndef __MSG_DISPATCHER_H__
#define __MSG_DISPATCHER_H__

#include <pthread.h>
#include <ext/hash_map>
#include "net_commu.h"

typedef void msg_callback(const char* data, uint32_t len, int cmdid, net_commu* commu);

class msg_dispatcher
{
public:
    msg_dispatcher() {}

    int add_msg_cb(int cmdid, msg_callback* msg_cb)
    {
        if (_dispatcher.find(cmdid) != _dispatcher.end()) return -1;
        _dispatcher[cmdid] = msg_cb;
        return 0;
    }

    bool exist(int cmdid) const { return _dispatcher.find(cmdid) != _dispatcher.end(); }

    msg_callback* cb(int cmdid)
    {
        return _dispatcher.find(cmdid) != _dispatcher.end()? _dispatcher[cmdid]: NULL;
    }

private:
    __gnu_cxx::hash_map<int, msg_callback*> _dispatcher;
    typedef __gnu_cxx::hash_map<int, msg_callback*>::iterator chm_it;
};

#endif
