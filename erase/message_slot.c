
// NOTE: This file was based on chardev.c file from rec 6

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

static channelList message_slot_device_files[257]; // minor numbers does not exceed 256 using register_chrdev

static int device_open(struct inode *inode,
                       struct file *file) {
    // (The initialization of channelLists for all minors is in module_init)
    return SUCCESS;
}

static ssize_t device_read( struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset){
    channelNode* cur_channel;
    int i, minor_num;
    if (buffer == NULL){
        printk("Buffer pointer is NULL\n");
        return -EINVAL;
    }
    minor_num = iminor(file->f_inode);
    cur_channel = (channelNode *)file->private_data;
    if (cur_channel == NULL){
        printk("No channel has been set for this file descriptor of minor %d\n", minor_num);
        return -EINVAL;
    }
    if (cur_channel->len_message == 0){
        printk("No message in channel %d of minor %d\n", cur_channel->channel_id, minor_num);
        return -EWOULDBLOCK;
    }
    if (cur_channel->len_message > length){
        printk("The provided buffer length is too small to hold the last message written in channel %d of minor %d\n", cur_channel->channel_id, minor_num);
        return -ENOSPC;
    }
    for (i = 0; i < cur_channel->len_message; i++){
        if (put_user(cur_channel->message[i], &buffer[i]) != 0){
            printk("put_user failed\n");
            return -EFAULT;
        }
    }
    printk("Read message from channel %d of minor %d\n", cur_channel->channel_id, minor_num);
    return i;
}

static ssize_t device_write( struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset){
    channelNode* cur_channel;
    int i, minor_num;
    char temp_message[BUF_LEN];
    if (buffer == NULL){
        printk("Buffer pointer is NULL\n");
        return -EINVAL;
    }
    minor_num = iminor(file->f_inode);
    cur_channel = (channelNode *)file->private_data;
    if (cur_channel == NULL){
        printk("No channel has been set for this file descriptor of minor %d\n", minor_num);
        return -EINVAL;
    }
    if (length == 0 || length > BUF_LEN){
        printk("The passed message length is 0 or more than 128\n");
        return -EMSGSIZE;
    }
    for (i = 0; i < length; i++){
        // get the message in the buffer to temp_message, so if error occurred cur_channel->message is not overridden
        if (get_user(temp_message[i], &buffer[i]) != 0){
            printk("get_user failed\n");
            return -EFAULT;
        }
    }
    // update message in the chanel if error is not occurred
    cur_channel->len_message = i;
    for (i = 0; i < cur_channel->len_message; i++){
        cur_channel->message[i] = temp_message[i];
}
    printk("Write message to channel %d of minor %d\n", cur_channel->channel_id, minor_num);
    return i;
}

static long device_ioctl( struct file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param ){
    channelNode *cur_channel_node, *last_node;
    int minor_num;
    if (ioctl_command_id != MSG_SLOT_CHANNEL){
        printk("The command in ioctl is not MSG_SLOT_CHANNEL\n");
        return -EINVAL;
    }
    if (ioctl_param == 0){
        printk("Channel id shouldn't be zero\n");
        return -EINVAL;
    }
    if (ioctl_param > UINT_MAX){
        printk("Channel id should be smaller then unsigned int max value\n");
        return -EINVAL;
    }
    minor_num = iminor(file->f_inode);
    cur_channel_node = message_slot_device_files[minor_num].head;
    last_node = NULL;
    while (cur_channel_node != NULL){
        last_node = cur_channel_node;
        if(cur_channel_node->channel_id == ioctl_param){
            break;
        }
        cur_channel_node = cur_channel_node->next;
    }
    if (cur_channel_node == NULL){
        // channel with the required channel id don't exist yet, so we need to allocate memory
        cur_channel_node = (channelNode *)kmalloc(sizeof(channelNode), GFP_KERNEL);
        if (cur_channel_node == NULL){
            printk("Failing to allocate memory\n");
            return -ENOMEM;
        }
        if (last_node == NULL){
            // No channels exist for this minor
            message_slot_device_files[minor_num].head = cur_channel_node;
        }
        else{
            last_node->next = cur_channel_node;
        }
        cur_channel_node->channel_id = ioctl_param;
        cur_channel_node->len_message = 0;
        cur_channel_node->next = NULL;
    }
    file->private_data = cur_channel_node;
    printk("Ioctl of channel %d of minor %d\n", cur_channel_node->channel_id, minor_num);
    return SUCCESS;
}

struct file_operations Fops = {
        .owner = THIS_MODULE,
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .unlocked_ioctl = device_ioctl,
};

static int __init init_message_slot(void){
    int rc = -1;
    int i;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 ) {
        printk( KERN_ERR "%s registration failed for  %d\n",
                DEVICE_NAME, MAJOR_NUM );
        return rc;
    }

    for (i = 0; i < 257; i++){
        message_slot_device_files[i].head = NULL;
    }

    printk( "Registration is successful \n");
    return SUCCESS;
}

static void __exit cleanup_message_slot(void){
    channelNode *cur_channel_node, *temp_node;
    int i;
    for (i = 0; i < 257; i++){
        cur_channel_node = message_slot_device_files[i].head;
        while (cur_channel_node != NULL){
            temp_node = cur_channel_node;
            cur_channel_node = cur_channel_node->next;
            kfree(temp_node);
        }
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(init_message_slot);
module_exit(cleanup_message_slot);