#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("23");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

typedef struct list_head{
	void *myitem;
	struct list_head * next;
	struct list_head * prev;
} list_head;

typedef struct list{
	list_head node;
	int data;
	void * voidP;
}list_t;



list_t * t;
list_head tail;

static ssize_t mp1_read(struct file *file, char __user * buffer, size_t count, loff_t * data){
	int copied;
	char * buf;
	buf = (char*) kmalloc( count, GFP_KERNEL);
	copied = 0;
	
	
	copy_to_user(buffer, buf, copied);
	kfree(buf);
	return copied;
}

static ssize_t mp1_write(struct file *file, char __user *buffer, size_t count, loff_t * data){
	char * buf = (char*)kmalloc(count. GFP_KERNEL);
	copy_from_user(buf, buffer, count);

	tail->next = kmalloc(sizeof(list_head), GFP_KERNEL);
	tail->next->prev = tail;
	tail = tail->next;
	tail->myitem = buf;
	
}

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   // Insert your code here ...
//   proc_dir = proc_mkdir(DIRECTORY, NULL);
//	proc_entry = proc_create(FILENAME, 0666, proc_dir, &mp1_file);
  
	list = kmalloc(sizeof(list), GFP_KERNEL);
	list->voidP = NULL= NULL;
	list->node.myitem = getpid();
	list->node.next = NULL;
	list->node.prev = NULL;
 	tail = &list->node;

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
   
   freeEverything();

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
