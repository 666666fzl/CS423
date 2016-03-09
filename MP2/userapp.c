#include "userapp.h"
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
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

 // * params: the pid for the task to be unregistered
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

// * params: the pid for the task to be unregistered
// * return: on success, return the number of bytes written; on failure return a negative number
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

void do_job(void)
{
    int n = 100, first = 1, second = 1, ret, i;

    // fibonacci calculation
    for(i = 0; i < n; i++) {
        ret = first + second;
        first = second;
        second = ret;
    }
    ret = first + second;


	/*time_t rawtime;
	struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    printf ( "Current local time and date: %s", asctime (timeinfo) );*/
}

bool check_status(pid_t pid) 
{
    ssize_t read;
    char *line = NULL;
    size_t len = 0;
    char *pid_buf;
    FILE * fp = fopen(PROC_FILE, "a+");
    if(!fp) {
        perror("file doesn't exist\n");
        return -1;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        pid_buf = strtok(pid_buf, ":")
        if(atoi(pid_buf) == pid)){
            return true;
        }
    }

    return false;
}

int main(int argc, char* argv[])
{
    unsigned long per = strtoul(argv[1], NULL, 10);
    pid_t pid = getpid();
    int i, wakeup_time, job_process_time;
    struct timeval t0, t1;

    if(argc < 2)
    {   
        perror("Number of arguments wrong, please follow: ./userapp [period]\n");
        return -1; 
    }

    // determine the processing time for the job
    gettimeofday(&old_tv, NULL);
    for(i = 0; i < 10000; i++) {

    }

    reg(pid, per, proc_t); //Proc filesystem

    if (!check_status(pid)) exit 1; //Proc filesystem: Verify the process was admitted 

    gettimeofday(&t0, NULL);
    yield(pid); //Proc filesystem

    // real-time loop
    while(/*exist jobs*/1) {
		do_job();
        gettimeofday(&t1, NULL);
        wakeup_time = t1.msec - t0.msec;
		yield(pid); //Proc filesystem
        gettimeofday(&t1, NULL);
        job_process_time = t1.msec-wakeup_time;
        printf ( "pid: %u, wake-up time: %d ms, spent: %d ms\n"
            , pid, wakeup_time, job_process_time);
	}
    unreg(pid);
}

