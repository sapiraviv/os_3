#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message_slot.h"

int main(int argc, char const *argv[]) {
    int file_desc, len_message;
    unsigned int channel_id;
    
    if (argc != 4) {
        perror("You should pass correct number of arguments: 1 - message slot file path, 2 - message channel id, 3 - message");
        exit(1);
    }
    file_desc = open(argv[1], O_WRONLY);
    if (file_desc == -1) {
        perror("Failed to open device file \n");
        exit(1);
    }
    channel_id = atoi(argv[2]); // If no valid conversion could be performed, atoi returns 0 and errno will set to EINVAL in message_slot.c
    if (ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id) < 0){
        perror("Failed to set the channel id \n");
        exit(1);
    }
    len_message = strlen(argv[3]);
    if (write(file_desc, argv[3], len_message) != len_message) {
        perror("Failed to write to the message slot file \n");
        exit(1);
    }
    close(file_desc);
    exit(0);
}