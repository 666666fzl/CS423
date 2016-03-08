#include "userapp.h"
#include <stdlib.h>

#define PERIOD 3000
#define PROC_TIME 1000
#define PROC_FILE "/proc/mp2/status"

 // * params: the pid, period(ms) and process time(ms) for the task to be registered
 // * return: on success, return the number of bytes written; on failure return a negative number
int reg(pid_t pid, unsigned long period, unsigned long proc_time)
{
    FILE * fp = fopen (PROC_FILE, "a+");
    if(!fp)
    {   
        perror ("file doesn't exist\n");
        return -1; 
    }   
    int byte_write = fprintf(fp, "R:%d %lu %lu", pid, period, proc_time);
    fclose(fp);
    return byte_write;
}

 // * params: char *write_path: registration file
 // * return: on success, return the number of bytes written; on failure return a negative number
int unreg(pid_t pid)
{
    FILE * fp = fopen (PROC_FILE, "a+");
    if(!fp)
    {   
        perror ("file doesn't exist\n");
        return -1; 
    }   
    int byte_write = fprintf(fp, "D:%d", pid);
    fclose(fp);
    return byte_write;
}

int yield(pid_t pid)
{
	FILE * fp = fopen(PROC_FILE, "a+");
	if(!fp) {
		perror("file doesn't exist\n");
		return -1;
	}
	int byte_write = fprintf(fp, "Y:%d", pid);
	fclose(fp);
	return byte_write;
}

int read_status(void)
{

}

int main(int argc, char* argv[])
{
    /*if(argc < 4)
    {   
        perror("Number of arguments wrong, please follow [filepath] [cmd] [args]\n");
        return -1; 
    }

    char *cmd = argv[2];
	char *period = argv[3];
    char *write_path = argv[1];

    if (*cmd == 'r') {
        reg (write_path, period);
    }
    else if (*cmd == 'd') {
        pid_t pid = atoi(argv[3]);
        unreg (write_path, pid);
    }
    else{
		pid_t pid = atoi(argv[3]);
		yield (write_path, pid);
    }
    while(1);
    return 0;*/
    pid_t pid = getpid();
    reg(pid, PERIOD, PROC_TIME); //Proc filesystem
    list = read_status(); //Proc filesystem: Verify the process was admitted 
    if (!process in the list) exit 1;
    //setup everything needed for real-time loop: t0=gettimeofday() 
    yield(pid); //Proc filesystem
    //this is the real-time loop
    while(exist jobs)
    {
    do_job(); //wakeup_time=gettimeofday()-t0 and factorial computation
    YIELD(PID); //Proc filesystem. JobProcessTime=gettimeofday()-wakeup_time }
    UNREGISTER(PID); //Proc filesystem }
}
