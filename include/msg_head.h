#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

struct commu_head
{
    int cmdid;
    int length;
};

//for accepter communicate with connections
struct queue_msg
{
    enum MSG_TYPE
    {
        NEW_CONN,
        STOP_THD
    };
    MSG_TYPE cmd_type;
    int connfd;
};

#define COMMU_HEAD_LENGTH 8

#define MSG_LENGTH_LIMIT (65536 - COMMU_HEAD_LENGTH)

#endif
