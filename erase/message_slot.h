#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

#define DEVICE_NAME "message_slot"
#define BUF_LEN 128
#define SUCCESS 0

typedef struct channelNode {
    unsigned int channel_id;
    char message[BUF_LEN];
    int len_message;
    struct channelNode *next;
} channelNode;

typedef struct channelList {
    channelNode *head;
} channelList;

#endif //MESSAGE_SLOT_H