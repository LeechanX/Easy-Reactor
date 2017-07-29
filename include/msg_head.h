#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

#define MSG_LENGTH_LIMIT 65536

struct commu_head
{
    int cmdid;
    int length;
};

#define COMMU_HEAD_LENGTH sizeof (commu_head)

#endif
