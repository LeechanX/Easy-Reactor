#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

struct commu_head
{
    int cmdid;
    int length;
};

#define COMMU_HEAD_LENGTH sizeof(commu_head)

#define MSG_LENGTH_LIMIT ((int)(65536 - COMMU_HEAD_LENGTH))

#endif
