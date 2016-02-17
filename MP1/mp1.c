#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("23");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

#define FILENAME "status"
#define DIRECTORY "mp1"
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;

static struct timer_list my_timer;

static struct workqueue_struct update_workqueue;

static ssize_t mp1_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
// implementation goes here... 
}

static ssize_t mp1_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
// implementation goes here...
}

static const struct file_operations mp1_file = {
   .owner = THIS_MODULE,
   .read = mp1_read,
   .write = mp1_write,
};

static void update_workqueue_init()
{
   if(!update_workqueue)
   {
      update_workqueue = create_workqueue("update_workqueue");
   }

   struct work_struct update_time;
   INIT_WORK(&update_time, _worker_);
   queue_work(update_workqueue, &update_time);
}


static void interrupt_handler (){
   mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
// interrupt handler here => update register information
}

static void create_my_timer {
   init_timer(&my_timer);
   my_timer.data = 0;
   my_timer.expires = jiffies + msecs_to_jiffies(5000);
   my_timer.function = interrupt_handler;
   add_timer(&my_timer);
}

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   // Insert your code here ...

   // create proc directory and file entry
   proc_dir = proc_mkdir(DIRECTORY, NULL);
   proc_entry = proc_create(FILENAME, 0666, proc_dir, & mp1_file);

   // create Linux Kernel Timer
   create_my_timer();

   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
   
   remove_proc_entry(FILENAME, proc_entry);
   remove_proc_entry(DIRECTORY, proc_dir);
   del_timer(&my_timer);

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
