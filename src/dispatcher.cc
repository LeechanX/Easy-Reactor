#include "dispatcher.h"
#include <assert.h>

dispatcher* dispatcher::_ins = NULL;
pthread_once_t dispatcher::_once = PTHREAD_ONCE_INIT;

void dispatcher::init()
{
    _ins = new dispatcher();
    assert(_ins);
}

dispatcher* dispatcher::ins()
{
    ::pthread_once(&_once, init);
    return _ins;
}

int dispatcher::add_msg_cb(int cmdid, msg_callback* msg_cb)
{
    if (_dispatcher.find(cmdid) != _dispatcher.end())
        return -1;
    _dispatcher[cmdid] = msg_cb;
    return 0;
}
