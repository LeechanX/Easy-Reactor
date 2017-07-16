#include "tcp_conn.h"
#include "msg_head.h"
#include "dispatcher.h"
#include <string.h>

static void tcp_rcb(event_loop* loop, int fd, void *args)
{
    tcp_conn* conn = (tcp_conn*)args;
    conn->handle_read();
    //TODO
}

static void tcp_wcb(event_loop* loop, int fd, void *args)
{
    tcp_conn* conn = (tcp_conn*)args;
    conn->handle_write();
    //TODO
}

tcp_conn::tcp_conn(int connfd, event_loop* loop): _connfd(connfd), _loop(loop)
{
    _loop->add_ioev(connfd, tcp_rcb, EPOLLIN | EPOLLET, this);
}

void tcp_conn::handle_read()
{
    int ret = ibuf.read_data(_connfd);
    if (ret != 0)
    {
            //pass
    }
    msg_head header;
    while (ibuf.length() >= MSG_HEAD_LENGTH)
    {
        ::memcpy(&header, ibuf.data(), MSG_HEAD_LENGTH);
        if (header.length > MSG_LENGTH_LIMIT || header.length < 0)
        {
            //data format is messed up
            //clear buf, close
            break;
        }
        if (ibuf.length() < MSG_HEAD_LENGTH + header.length)
        {
            //this is half-package
            break;
        }
        //find in dispatcher
        msg_callback* cb = dispatcher::ins()->cb(header.cmdid);
        if (!cb)
        {
            //data format is messed up
            //clear buf, close
            //todo
            break;
        }
        ibuf.pop(MSG_HEAD_LENGTH);
        //domain: call user callback
        cb(ibuf.data(), header.length, this);
        ibuf.pop(header.length);
    }
}

void tcp_conn::handle_write()
{
    if (obuf.length())
    {
        int ret = obuf.write_fd(_connfd);
        //ret......
        //TODO
        if (ret != 0)
        {
            //......
        }
    }
    if (!obuf.length())
    {
        _loop->del_ioev(_connfd, EPOLLOUT);
    }
}

void tcp_conn::send_data(const char* data, uint32_t datlen)
{
    bool need_listen = false;
    if (!obuf.length())
        need_listen = true;
    obuf.send_data(data, datlen);
    if (need_listen)
    {
        _loop->add_ioev(_connfd, tcp_wcb, EPOLLOUT, this);
    }
}
