#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

class event_loop;

struct commu_head
{
    int cmdid;
    int length;
};

//for accepter communicate with connections
//for give task to sub-thread
struct queue_msg
{
    enum MSG_TYPE
    {
        NEW_CONN,
        STOP_THD,
        NEW_TASK,
    };
    MSG_TYPE cmd_type;

    union {
        int connfd;//for NEW_CONN, 向sub-thread下发新连接
        struct
        {
            void (*task)(event_loop*, void*);
            void *args;
        };//for NEW_TASK, 向sub-thread下发待执行任务
    };
};

#define COMMU_HEAD_LENGTH 8

#define MSG_LENGTH_LIMIT (65536 - COMMU_HEAD_LENGTH)

#endif
