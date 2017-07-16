#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

#include <stdint.h>

#define MSG_HEAD_LENGTH (sizeof (int) * 2)
#define MSG_LENGTH_LIMIT 65536

struct msg_head
{
    int cmdid;
    int length;
};

#endif
