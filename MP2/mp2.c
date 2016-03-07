#define LINUX

#include "mp2_given.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("23");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG 1
#define FILENAME "status"
#define DIRECTORY "mp2"
#define MAX_BUF_SIZE 128
#define SLEEPING_STATE 0
#define READY_STATE 1
#define RUNNING_STATE 2

typedef struct mp2_task_struct {
	struct task_struct* linux_task;
	struct timer_list wakeup_timer;
	pid_t pid;
	// 0 = SLEEPING
	// 1 = READY
	// 2 = RUNNING
	int state;
	unsigned long proc_time;
	unsigned long period;
	struct list_head process_node;
} task_node_t;

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static struct mutex my_mutex;
static struct kmem_cache *task_cache;

LIST_HEAD(taskList);
task_node_t *current_running_task;
struct task_struct *dispatching_task;

// Called when user application use "cat" or "fopen"
// The function read the status file and print the information related out
static ssize_t mp2_read(struct file *file, char __user * buffer, size_t count, loff_t * data)
{
    size_t copied = 0;
    char * buf = NULL;
    struct list_head *pos = NULL;
    task_node_t *tmp = NULL;
    char currData[MAX_BUF_SIZE];
    int currByte;
    buf = (char*) kmalloc(1024, GFP_KERNEL);
    
	mutex_lock(&my_mutex);
    list_for_each(pos, &taskList) {
        tmp = list_entry(pos, task_node_t, process_node);
        memset(currData, 0, MAX_BUF_SIZE);
        currByte = sprintf(currData, "%u: %lu, %lu\n", tmp->pid, tmp->period, tmp->proc_time);
        strcat(buf, currData);
        copied += currByte;
    }
    mutex_unlock(&my_mutex);
    
    if(*data>0)
    {
        return 0;
    }
    copy_to_user(buffer, buf, copied);
    kfree(buf);
    *data += copied;

    return copied;
	
}

// Called when one of the tasks is waked up
// The function checks if a context switch is needed and do the context switch
int dispatching_thread(void *data)
{
	while(1)
	{
		task_node_t *entry;
		task_node_t *prev_task;
		struct sched_param new_sparam; 
		struct sched_param old_sparam; 
		struct list_head *pos;
		task_node_t *next_task=NULL;
		if(current_running_task)
		{
			list_for_each(pos, &taskList) {
				entry = list_entry(pos, task_node_t, process_node);
				if (entry->period < current_running_task->period && entry->state == READY_STATE) {
					next_task = entry;
					break;
				}
			}
		}
		
		prev_task = current_running_task;
		prev_task->state = READY_STATE;
		
		//old task
		old_sparam.sched_priority=0; 
		sched_setscheduler(prev_task->linux_task, SCHED_NORMAL, &old_sparam);
		
		if(next_task)
		{	
			// new task
			wake_up_process(next_task->linux_task); 
			new_sparam.sched_priority=99;
			sched_setscheduler(next_task->linux_task, SCHED_FIFO, &new_sparam);

			current_running_task = next_task;
			current_running_task->state = RUNNING_STATE;
		}
			
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return 0;
}

// Called when one of the task's timer is expired
// Set the task to ready state and call the dispatching thread
void _wakeup_timer_handler(unsigned long arg) 
{
	task_node_t *curr_node = (task_node_t *)arg;
	if (curr_node != current_running_task) { 
		curr_node -> state = READY_STATE;
		wake_up_process(dispatching_task);
	}
}

// Called when a new self-defined task node is allocated
// Store user input, set task state and create timer for it
void init_node(task_node_t* new_task, char* buf) 
{
	int i = 0;
	char *pch;
    char *dataHolder = (char*)kmalloc(strlen(buf)+1, GFP_KERNEL);
	struct timer_list *curr_timer;
	
	if(dataHolder)
	{
		strcpy(dataHolder, buf);
	}
	
	pch = strsep(&dataHolder, " ");
	
	// parse user input and store it into the node
	for(i = 0; i < 3 && pch!=NULL; i ++)
	{
		if(i==0)
		{
			sscanf(pch, "%u", &(new_task->pid));
		}
		else if(i==1)
		{
			sscanf(pch, "%lu", &(new_task->period));
		}
		else
		{
			sscanf(pch, "%lu", &(new_task->proc_time));
		}
		pch = strsep(&dataHolder, " ,");
	}	
	
	new_task -> state = SLEEPING_STATE;
	new_task -> linux_task = find_task_by_pid(new_task->pid);

	// create task wakeup timer
	curr_timer = &(new_task->wakeup_timer);
	init_timer(curr_timer);
	curr_timer->data = (unsigned long)new_task;
	curr_timer->expires = jiffies + msecs_to_jiffies(new_task->period - new_task->proc_time);
    curr_timer->function = _wakeup_timer_handler;
	add_timer(curr_timer);
}

// Add a newly created task node into the existing task linked list
// Ordered bt task period (shortest period first)
int add_to_list(char *buf)
{
	struct list_head *pos;
	task_node_t *entry;
	task_node_t *new_task = kmem_cache_alloc(task_cache, GFP_KERNEL);

	init_node(new_task, buf);

    list_for_each(pos, &taskList) {
        entry = list_entry(pos, task_node_t, process_node);
        if (entry->period > new_task->period) {
		    list_add_tail(&new_task->process_node, pos);
			return -1;
        }
    }

	list_add_tail(&(new_task->process_node), &taskList);	
	return -1;
}

// Free a allocated task node
void destruct_node(struct list_head *pos)
{
	task_node_t *entry;
	list_del(pos);
	entry = list_entry(pos, task_node_t, process_node);
	kmem_cache_free(task_cache, entry);
	del_timer(&(entry->wakeup_timer));
}

// Traverse the entire task linked list and find a task according to its pid
struct list_head *find_task_node_by_pid(char *pid)
{
    struct list_head *pos;
    struct list_head *next;
    task_node_t *curr;
    char curr_pid[20];

    mutex_lock(&my_mutex);

    list_for_each_safe(pos, next, &taskList){
        curr = list_entry(pos, task_node_t, process_node);
        memset(curr_pid, 0, 20);
        sprintf(curr_pid, "%u", curr->pid);
        if(strcmp(curr_pid, pid)==0)
        {
            mutex_unlock(&my_mutex);
            return pos;
        }
    }

    mutex_unlock(&my_mutex);
    return NULL;
}

// Called when user input "Y" as command
// Put yield task into sleeping state and start its wakeup timer
int yield_handler(char *pid)
{
	task_node_t *yield_task;
    struct list_head *yield_pos;

	yield_pos = find_task_node_by_pid(pid);
    yield_task = list_entry(yield_pos, task_node_t, process_node);

	yield_task->state = SLEEPING_STATE;
    mod_timer(&(yield_task->wakeup_timer), 
        jiffies + msecs_to_jiffies(yield_task->period - yield_task->proc_time));
	set_task_state(yield_task->linux_task, TASK_UNINTERRUPTIBLE);
	current_running_task = NULL;
	wake_up_process(dispatching_task);
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();	

	return 0;	
}

bool admission_control(void)
{
    return false;
}

// Called when user application registered a process
// The function get the pid from the user and put it on the linked list, which actually write it in the status file
static ssize_t mp2_write(struct file *file, const char __user *buffer, size_t count, loff_t * data){
	char * buf = (char*)kmalloc(count+1, GFP_KERNEL);
	int ret = -1;
    struct list_head *pos;
    {
        /* data */
    };

	if (count > MAX_BUF_SIZE - 1) {
		count = MAX_BUF_SIZE - 1;
	}

	copy_from_user(buf, buffer, count);
	buf[count] = '\0';

	printk(KERN_ALERT "MP2_WRITE CALLED, INPUT:%s\n", buf);
	
	// Check the starting char of buf, if:
	// 1.register: R,PID,PERIOD,COMPUTATION
	if (buf[0] == 'R') {
		ret = add_to_list(buf+2);
		printk(KERN_ALERT "REGISTERED PID:%s", buf+2);
	}
	else if (buf[0] == 'Y') {
	// 2.yield: Y,PID
	}
	else if (buf[0] == 'D') {
	// 3.unregister: D,PID
        pos = find_task_node_by_pid(buf+2);
        destruct_node(pos);
        ret = -1;
		printk(KERN_ALERT "UNREGISTERED PID: %s", buf+2);
	}
	else {
		return 0;
	}

	return ret;
}

static const struct file_operations mp2_file = {
    .owner = THIS_MODULE,
    .read = mp2_read,
    .write = mp2_write,
};

// mp2_init - Called when module is loaded
int __init mp2_init(void)
{
    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE LOADING\n");
    #endif
    // create proc directory and file entry
    proc_dir = proc_mkdir(DIRECTORY, NULL);
    proc_entry = proc_create(FILENAME, 0666, proc_dir, & mp2_file);
	current_running_task = NULL;

	dispatching_task = kthread_run(dispatching_thread, NULL, "mp2");

	// create cache for slab allocator
	task_cache = kmem_cache_create("task_cache", sizeof(task_node_t), 0, SLAB_HWCACHE_ALIGN, NULL);

    // init mutex lock
    mutex_init(&my_mutex);

    printk(KERN_ALERT "MP2 MODULE LOADED\n");
    return 0;
}

// mp2_exit - Called when module is unloaded
void __exit mp2_exit(void)
{
    struct list_head *pos;
    struct list_head *next;

    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
    #endif

    // remove every node on linked list and remove the list     
    list_for_each_safe(pos, next, &taskList){
		destruct_node(pos);
	}

    // remove file entry and repository  
    remove_proc_entry(FILENAME, proc_dir);
    remove_proc_entry(DIRECTORY, NULL);

    printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);

