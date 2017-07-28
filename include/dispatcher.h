#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <pthread.h>
#include <ext/hash_map>
#include "net_commu.h"

typedef void msg_callback(const char* data, uint32_t len, int cmdid, net_commu* commu);

class dispatcher
{
public:
    static void init();

    static dispatcher* ins();

    int add_msg_cb(int cmdid, msg_callback* msg_cb);

    bool exist(int cmdid) const { return _dispatcher.find(cmdid) != _dispatcher.end(); }

    msg_callback* cb(int cmdid)
    {
        return _dispatcher.find(cmdid) != _dispatcher.end()? _dispatcher[cmdid]: NULL;
    }

private:
    dispatcher() {}

    dispatcher(const dispatcher& other);
    const dispatcher& operator=(const dispatcher& other);

    __gnu_cxx::hash_map<int, msg_callback*> _dispatcher;
    typedef __gnu_cxx::hash_map<int, msg_callback*>::iterator chm_it;
    static dispatcher* _ins;
    static pthread_once_t _once;
};

#endif
