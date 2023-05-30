
//=============== ALL BASED ON REC06 =============== //

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

//=============== Data Structure =============== //

typedef struct ms_channel {
    long id;
    struct ms_channel *next;
    int message_len;
    char message[BUF_LEN];
} ms_channel;

typedef struct ms_file {
    unsigned int minor;
    ms_channel *first;
} ms_file;

typedef struct ms_driver{
    ms_file all_files[257];
} ms_driver;

static ms_driver *msDriver;

//=============== File Operations  =============== //
//(msDriver->all_files)[f_minor].first = NULL because we initialize with kzalloc.
static int device_open( struct inode* inode, struct file* file){
    unsigned int f_minor = iminor(inode);
    ms_file* node = &(msDriver->all_files[f_minor]);
    node->minor = f_minor;
    file->private_data = node;
    return SUCCESS;
}

static int device_release( struct inode* inode, struct file*  file){
    return SUCCESS;
}

 int allocate_new_channel (ms_channel **curr_chanel, unsigned long ioctl_param){
    *curr_chanel = kmalloc(sizeof(ms_channel), GFP_KERNEL);
    if (*curr_chanel == NULL){
        printk("memory allocation failed");
        return -ENOMEM;
    }
    else{
        (*curr_chanel)->next = NULL;
        (*curr_chanel)->id = ioctl_param;
        (*curr_chanel)->message_len = 0;
        return 0;
    }
}

void make_first_channel(ms_channel *curr_chanel, ms_channel *prev, ms_file *node, ms_channel *old_first){
    node->first = curr_chanel;
    if (old_first == NULL){
        return;
    }
    prev->next = curr_chanel->next;
    curr_chanel->next = old_first;

}

static long device_ioctl( struct file* file,unsigned int ioctl_command_id, unsigned long ioctl_param ){
    ms_file *node;
    ms_channel *curr_chanel, *prev, *old_first;
    if ( (ioctl_param == 0) || (ioctl_command_id != MSG_SLOT_CHANNEL) ){
        printk("The passed params to ioctl is invalid");
        return -EINVAL;
    }
    node = (ms_file *)(file->private_data);
    curr_chanel = (node->first);
    old_first = curr_chanel;
    prev = curr_chanel;
    if (  curr_chanel == NULL){
        allocate_new_channel(&curr_chanel, ioctl_param);
        make_first_channel(curr_chanel, NULL, node, NULL);
        return SUCCESS;
    }
    else{
        while ( curr_chanel != NULL ){
            if (curr_chanel->id == ioctl_param){
                make_first_channel(curr_chanel, prev, node, old_first);
                return SUCCESS;
            }
            else{
                prev = curr_chanel;
                curr_chanel = prev->next;
                if(curr_chanel == NULL){
                    printk("NULL");
                }
                printk("id : %ld/n", curr_chanel);
            }
        }
        allocate_new_channel(&curr_chanel, ioctl_param);
        make_first_channel(curr_chanel, prev, node, old_first);
        return SUCCESS;
    }
}

static ssize_t device_write( struct file*  file,const char __user* buffer, size_t  length, loff_t*  offset){
    int i, j;
    char the_message[BUF_LEN];
    ms_channel *channel;
    ms_file *node;
    node = (ms_file *)(file->private_data);
    channel = node->first;
    if (channel == NULL ){
        printk("No channel has been set to the fd");
        return -EINVAL;
    }
    if ((length == 0) || (length > BUF_LEN)){
        printk("Invalid message length");
        return -EMSGSIZE;
    }
    for( i = 0; i < length ; ++i ) {
        if (get_user(the_message[i], &buffer[i]) != 0){
            printk("Faild in get_user");
            return -EINVAL;
        }
    }
    channel->message_len = i;
    for (j=0; j < i; j++){
        (channel->message)[j] = the_message[j];
    }
    return i;


}

static ssize_t device_read( struct file* file, char __user* buffer,size_t length, loff_t* offset ){
    int i;
    ms_file *node;
    ms_channel *channel;
    node = (ms_file *)(file->private_data);
    channel = node->first;
    if (channel == NULL ){
       printk("No channel has been set to the fd");
       return -EINVAL;
    }
    if (channel->message_len == 0){
        printk("Ne message exists on the channel");
        return -EWOULDBLOCK;
    }
    if (channel->message_len > length){
        printk("Buffer length to small");
        return -ENOSPC;
    }
    for( i = 0; i < (channel->message_len) ; ++i ) {
       if (put_user(channel->message[i], &buffer[i]) != 0){
           printk("Faild in get_user");
           return -EINVAL;
       }
    }
    return i;
}
//=============== File SETUP =============== //

struct file_operations Fops = {
        .owner	  = THIS_MODULE,
        .read           = device_read,
        .write          = device_write,
        .open           = device_open,
        .unlocked_ioctl = device_ioctl,
        .release        = device_release,
};

static int __init simple_init(void){
    int rc = -1;
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 ) {
        printk(KERN_ERR"%s registration failed for  %d\n",DEVICE_FILE_NAME, MAJOR_NUM );
        return rc;
    }

    // init message_slot struct
    msDriver = (ms_driver *)kzalloc(sizeof(ms_driver), GFP_KERNEL);
    if (msDriver == NULL){
        printk("memory allocation failed");
        return -ENOMEM;
    }

    return SUCCESS;
}

static void __exit simple_cleanup(void)
{
    // Free memory
    int i;
    ms_channel *curr;
    ms_channel *prev;
    ms_channel *next;
    ms_file *f_;
    for(i=0; i<257; i++){
        f_ = &msDriver->all_files[i];
        if (f_ != NULL){
            curr = f_->first;
            while (curr != NULL){
               prev = curr;
               next = curr->next;
               curr = next;
               kfree(prev);
            }
        }
    }
    kfree(msDriver);
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

}


module_init(simple_init);
module_exit(simple_cleanup);
