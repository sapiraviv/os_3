#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd;
    unsigned long channel_id;
    if (argc != 4){
        fprintf(stderr, "The wrong number of arguments %s\n", strerror(errno));
        exit(1);
    }
    fd = open(argv[1], O_WRONLY);
    if (fd < 0){
        fprintf(stderr, "Open file failed %s\n", strerror(errno));
        exit(1);
    }
    channel_id = atoi(argv[2]);
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0){
        fprintf(stderr, "ioctl failed %s\n", strerror(errno));
        exit(1);
    }
    if (write(fd, argv[3], strlen(argv[3])) != strlen(argv[3])){
        fprintf(stderr, "write failed %s\n", strerror(errno));
        exit(1);
    }
    close(fd);
    exit(0);
}