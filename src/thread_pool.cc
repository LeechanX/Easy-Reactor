#include "thread_pool.h"
#include "tcp_conn.h"
#include "event_loop.h"
#include "print_error.h"
#include "tcp_server.h"

void msg_comming_cb(event_loop* loop, int fd, void *args)
{
    thread_queue* queue = (thread_queue*)args;
    std::queue<queue_msg> msgs;
    queue->recv_msg(msgs);
    while (!msgs.empty())
    {
        queue_msg msg = msgs.front();
        msgs.pop();
        if (msg.cmd_type == queue_msg::NEW_CONN)
        {
            tcp_conn* conn = tcp_server::conns[msg.connfd];
            if (conn)
            {
                conn->init(msg.connfd, loop);
            }
            else
            {
                conn = new tcp_conn(msg.connfd, loop);
                exit_if(conn == NULL, "new tcp_conn");
            }
        }
        else
        {
            //TODO: other message between threads
        }
    }
}

void* thread_domain(void* args)
{
    thread_queue* queue = (thread_queue*)args;
    event_loop* loop = new event_loop();
    exit_if(loop == NULL, "new event_loop()");
    loop->add_ioev(queue->notifier(), msg_comming_cb, EPOLLIN | EPOLLET, queue);
    loop->process_evs();
    return NULL;
}

thread_pool::thread_pool(int thread_cnt): _curr_index(0), _thread_cnt(thread_cnt)
{
    exit_if(thread_cnt <= 0 || thread_cnt > 30, "error thread_cnt %d", thread_cnt);
    _pool = new thread_queue*[thread_cnt];
    _tids = new pthread_t[thread_cnt];
    int ret;
    for (int i = 0;i < thread_cnt; ++i)
    {
        _pool[i] = new thread_queue();
        ret = ::pthread_create(&_tids[i], NULL, thread_domain, _pool[i]);
        exit_if(ret == -1, "pthread_create");

        ret = ::pthread_detach(_tids[i]);
        error_if(ret == -1, "pthread_detach");
    }
}

thread_pool::~thread_pool()
{
    for (int i = 0;i < _thread_cnt; ++i)
    {
        //TODO: how to kill thread?
        //will let other thread coredump
        delete _pool[i];//problem
    }
    delete[] _pool;
    delete[] _tids;
}

thread_queue* thread_pool::get_next_thread()
{
    if (_curr_index == _thread_cnt)
        _curr_index = 0;
    return _pool[_curr_index++];
}