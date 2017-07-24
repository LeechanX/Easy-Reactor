#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

#include <stdint.h>

#define MSG_LENGTH_LIMIT 65536

struct req_head
{
    int cmdid;
    uint32_t length;
};

struct rsp_head
{
    uint32_t length;
};

#define REQ_HEAD_LENGTH sizeof (req_head)
#define RSP_HEAD_LENGTH sizeof (rsp_head)

#endif
