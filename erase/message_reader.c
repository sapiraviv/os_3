#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "message_slot.h"

int main(int argc, char const *argv[]) {
    int file_desc, len_message;
    unsigned int channel_id;
    char buffer_for_message[BUF_LEN];

    if (argc != 3) {
        perror("You should pass correct number of arguments: 1 - message slot file path, 2 - message channel id");
        exit(1);
    }
    file_desc = open(argv[1], O_RDONLY);
    if (file_desc == -1) {
        perror("Failed to open device file \n");
        exit(1);
    }
    channel_id = atoi(argv[2]); // If no valid conversion could be performed, atoi returns 0 and errno will set to EINVAL in message_slot.c
    if (ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id) < 0){
        perror("Failed to set the channel id \n");
        exit(1);
    }
    len_message = read(file_desc, buffer_for_message, BUF_LEN);
    if (len_message < 0) {
        perror("Failed to read from message slot file \n");
        exit(1);
    }
    close(file_desc);
    if (write(1, buffer_for_message, len_message) != len_message) {
        perror("Failed to write the message to stdout \n");
        exit(1);
    }
    exit(0);
}
