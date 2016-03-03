#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("23");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG 1
#define FILENAME "status"
#define DIRECTORY "mp2"
#define MAX_BUF_SIZE 128

typedef struct list_t{
    struct list_head node;
    char * data;
    unsigned long cpu_time;
} list_t;


static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static struct timer_list my_timer;
static struct workqueue_struct * update_workqueue;
static struct mutex my_mutex;
LIST_HEAD(pidList);

// Called when user application use "cat" or "fopen"
// The function read the status file and print the information related out
static ssize_t mp2_read(struct file *file, char __user * buffer, size_t count, loff_t * data){
    size_t copied = 0;
    char * buf = NULL;
    struct list_head *pos = NULL;
    list_t *tmp = NULL;
    char *pidInList = NULL;
    char currData[MAX_BUF_SIZE];
    int currByte;
    buf = (char*) kmalloc(1024, GFP_KERNEL);
    mutex_lock(&my_mutex);
    list_for_each(pos, &pidList) {
        tmp = list_entry(pos, struct list_t, node);
        pidInList = tmp->data;
        memset(currData, 0, MAX_BUF_SIZE);
        currByte = sprintf(currData, "%s: %lu\n", tmp->data, tmp->cpu_time);
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

int add_to_list(char *buf)
{
    list_t *insert_node = (list_t*)kmalloc(sizeof(list_t), GFP_KERNEL);
    insert_node->data = buf;
    insert_node->cpu_time = 0;
    mutex_lock(&my_mutex);
    list_add_tail(&(insert_node->node), &pidList);
    mutex_unlock(&my_mutex);
    return strlen(buf);
}

int delete_from_list(char *pid)
{
    struct list_head *pos;
    struct list_head *next;
    struct list_t *curr;
    mutex_lock(&my_mutex);

    list_for_each_safe(pos, next, &pidList){
        curr = list_entry(pos, struct list_t, node);
        if(strcmp(curr->data, pid)==0)
        {
            list_del(pos);
            kfree(list_entry(pos, struct list_t, node));
        }
    }

    mutex_unlock(&my_mutex);
    return 0;
}

int admission_control(struct file *file, const char __user *buffer, size_t count, loff_t * data)
{

    char * buf = (char*)kmalloc(count+1, GFP_KERNEL);
    int ret = -1;

    if (count > MAX_BUF_SIZE - 1) {
        count = MAX_BUF_SIZE - 1;
    }


    copy_from_user(buf, buffer, count);

    buf[count]='\0';
    printk(KERN_ALERT "mp2_write called, pid: %s", buf);

    // Check the starting char of buf, if:
    // 1) register: R,PID,PERIOD,COMPUTATION
    if (buf[0] == 'R') {
        ret = add_to_list(buf+2);
    }
    else if (buf[0] == 'Y'){
    // 2) yield: Y,PID 
    }
    else if (buf[0] == 'D') {
    // 3) unregister: D,PID
        ret = delete_from_list(buf+2);
    }
    else {
        return 0;
    }
    return ret;
}

// Called when user application registered a process
// The function get the pid from the user and put it on the linked list, which actually write it in the status file
static ssize_t mp2_write(struct file *file, const char __user *buffer, size_t count, loff_t * data){
    int ret = admission_control(file, buffer, count, data);
    return ret;
}

// Get the scheduled work and work on updating the cpu use time for processes corrsponding to nodes on the linked list
static void my_worker(struct work_struct * work) {
//      unsigned long cpu_time;
    struct list_head *pos;
    struct list_t *tmp = NULL;
    unsigned int base = 10;
    int pid;
    printk(KERN_ALERT "my_woker func called");
    mutex_lock(&my_mutex);
    list_for_each(pos, &pidList) {
        tmp = list_entry(pos, struct list_t, node);
        kstrtoint(tmp->data, base, &pid);
        printk(KERN_ALERT "%d", pid);
    /*  if(get_cpu_use(pid, &cpu_time) == 0) {
            // update each node
            tmp->cpu_time = cpu_time;
        }*/
    }
    mutex_unlock(&my_mutex);
}

// Create a new work and put it into the schedule without delaye
void _update_workqueue_init(void)
{
   struct work_struct *update_time = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_ATOMIC);

    INIT_WORK(update_time, my_worker);
    queue_work(update_workqueue, update_time);
}

// Called when timer expired, this will stark the work queue and restart timer ticking
void _interrupt_handler (unsigned long arg){
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
    _update_workqueue_init();
}

// Set default member variable value for timer, start timer ticking
static void _create_my_timer(void) {
    init_timer(&my_timer);
    my_timer.data = 0;
    my_timer.expires = jiffies + msecs_to_jiffies(5000);
    my_timer.function = _interrupt_handler;
    add_timer(&my_timer);
    printk(KERN_ALERT "TIMER CREATED");
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
    printk(KERN_ALERT "MP1 MODULE LOADING\n");
    #endif
    // create proc directory and file entry
    proc_dir = proc_mkdir(DIRECTORY, NULL);
    proc_entry = proc_create(FILENAME, 0666, proc_dir, & mp2_file);

    // create work queue
    update_workqueue = create_workqueue("update_workqueue");

    // create Linux Kernel Timer
    _create_my_timer();

    // init mutex lock
    mutex_init(&my_mutex);

    printk(KERN_ALERT "MP1 MODULE LOADED\n");
    return 0;
}

// mp2_exit - Called when module is unloaded
void __exit mp2_exit(void)
{
    struct list_head *pos;
    struct list_head *next;

    #ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
    #endif

    // delete timer
    del_timer(&my_timer);

    // clean and remove work queue
    flush_workqueue(update_workqueue);
    destroy_workqueue(update_workqueue);

    // remove every node on linked list and remove the list     
    list_for_each_safe(pos, next, &pidList){
        list_del(pos);
        kfree(list_entry(pos, struct list_t, node));
    }

    // remove file entry and repository  
    remove_proc_entry(FILENAME, proc_dir);
    remove_proc_entry(DIRECTORY, NULL);

    printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);

