#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "tcp_conn.h"
#include "tcp_server.h"
#include "print_error.h"
#include "config_reader.h"

void accepter_cb(event_loop* loop, int fd, void *args)
{
    tcp_server* server = (tcp_server*)args;
    server->do_accept();
}

tcp_conn** tcp_server::conns = NULL;
int tcp_server::_conns_size = 0;
int tcp_server::_max_conns = 0;
int tcp_server::_curr_conns = 0;
pthread_mutex_t tcp_server::_mutex = PTHREAD_MUTEX_INITIALIZER;
msg_dispatcher tcp_server::dispatcher;

tcp_server::tcp_server(event_loop* loop, const char* ip, uint16_t port, const char* conf_path)
{
    //ignore SIGHUP and SIGPIPE
    if (::signal(SIGHUP, SIG_IGN) == SIG_ERR)
    {
        error_log("signal ignore SIGHUP");
    }
    if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        error_log("signal ignore SIGPIPE");
    }

    //create socket
    _sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    exit_if(_sockfd == -1, "socket()");

    _reservfd = ::open("/tmp/reactor_accepter", O_CREAT | O_RDONLY | O_CLOEXEC, 0666);
    error_if(_reservfd == -1, "open()");

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    int ret = ::inet_aton(ip, &servaddr.sin_addr);
    exit_if(ret == 0, "ip format %s", ip);
    servaddr.sin_port = htons(port);

    int open_flag = 1;
    ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &open_flag, sizeof(open_flag));
    error_if(ret < 0, "setsockopt SO_REUSEADDR");

    ret = ::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &open_flag, sizeof(open_flag));
    error_if(ret < 0, "setsockopt TCP_NODELAY");

    ret = ::bind(_sockfd, (const struct sockaddr*)&servaddr, sizeof servaddr);
    exit_if(ret == -1, "bind()");

    ret = ::listen(_sockfd, 500);
    exit_if(ret == -1, "listen()");

    _loop = loop;

    _addrlen = sizeof (struct sockaddr_in);
    //load configure
    config_reader::setPath(conf_path);

    //if mode is multi-thread reactor, create thread pool
    int thread_cnt = config_reader::ins()->GetNumber("reactor", "threadNum", 0);
    _thd_pool = NULL;
    if (thread_cnt)
    {
        _thd_pool = new thread_pool(thread_cnt);
        exit_if(_thd_pool == NULL, "new thread_pool");
    }

    //create connection pool
    _max_conns = config_reader::ins()->GetNumber("reactor", "maxConns", 10000);
    int next_fd = ::dup(1);
    _conns_size = _max_conns + next_fd;
    ::close(next_fd);

    conns = new tcp_conn*[_conns_size];
    exit_if(conns == NULL, "new conns[%d]", _conns_size);
    for (int i = 0;i < _max_conns + next_fd; ++i)
        conns[i] = NULL;

    //add accepter event
    _loop->add_ioev(_sockfd, accepter_cb, EPOLLIN | EPOLLET, this);
}

tcp_server::~tcp_server()
{
    _loop->del_ioev(_sockfd);
    ::close(_sockfd);
    ::close(_reservfd);
    delete _loop;
}

void tcp_server::keep_alive()
{
    int open_flag = 1;
    int ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &open_flag, sizeof(open_flag));
    error_if(ret < 0, "setsockopt SO_KEEPALIVE");
}

void tcp_server::do_accept()
{
    int connfd;
    bool conn_full = false;
    while (true)
    {
        connfd = ::accept(_sockfd, (struct sockaddr*)&_connaddr, &_addrlen);
        if (connfd == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EMFILE)
            {
                conn_full = true;
                ::close(_reservfd);
            }
            else if (errno == EAGAIN)
            {
                break;
            }
            else
            {
                exit_log("accept()");
            }
        }
        else if (conn_full)
        {
            ::close(connfd);
            _reservfd = ::open("/tmp/reactor_accepter", O_CREAT | O_RDONLY | O_CLOEXEC, 0666);
            error_if(_reservfd == -1, "open()");
        }
        else
        {
            //connfd and max connections
            int curr_conns;
            get_conn_num(curr_conns);
            if (curr_conns >= _max_conns)
            {
                error_log("connection exceeds the maximum connection count %d", _max_conns);
                ::close(connfd);
            }
            else
            {
                assert(connfd < _conns_size);
                //multi-thread reactor model: round-robin a event loop and give message to it
                if (_thd_pool)
                {
                    thread_queue* cq = _thd_pool->get_next_thread();
                    queue_msg msg;
                    msg.cmd_type = queue_msg::NEW_CONN;
                    msg.connfd = connfd;
                    cq->send_msg(msg);
                }
                else//register in self thread
                {
                    tcp_conn* conn = conns[connfd];
                    if (conn)
                    {
                        conn->init(connfd, _loop);
                    }
                    else
                    {
                        conn = new tcp_conn(connfd, _loop);
                        exit_if(conn == NULL, "new tcp_conn");
                        conns[connfd] = conn;
                    }
                }
            }
        }
    }
}

void tcp_server::inc_conn()
{
    ::pthread_mutex_lock(&_mutex);
    _curr_conns++;
    ::pthread_mutex_unlock(&_mutex);
}

void tcp_server::get_conn_num(int& cnt)
{
    ::pthread_mutex_lock(&_mutex);
    cnt = _curr_conns;
    ::pthread_mutex_unlock(&_mutex);
}

void tcp_server::dec_conn()
{
    ::pthread_mutex_lock(&_mutex);
    _curr_conns--;
    ::pthread_mutex_unlock(&_mutex);
}
