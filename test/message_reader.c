#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd;
    unsigned long channel_id;
    char buff[BUF_LEN];
    int len;
    if (argc != 3){
        fprintf(stderr, "The wrong number of arguments %s\n", strerror(errno));
        exit(1);
    }
    fd = open(argv[1], O_RDONLY);
    if (fd < 0){
        fprintf(stderr, "Open file failed %s\n", strerror(errno));
        exit(1);
    }
    channel_id = atoi(argv[2]);
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0){
        fprintf(stderr, "ioctl failed %s\n", strerror(errno));
        exit(1);
    }
    len = read(fd, buff, BUF_LEN);
    if (len < 0){
        fprintf(stderr, "read failed %s\n", strerror(errno));
        exit(1);
    }
    close(fd);
    if (write(1, buff, len) != len){
        fprintf(stderr, "write failed %s\n", strerror(errno));
        exit(1);
    }
    exit(0);
}
