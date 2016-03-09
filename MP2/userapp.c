#include "userapp.h"
#include <stdlib.h>
#include <time.h>
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

void do_job(void)
{
	time_t rawtime;
	struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    printf ( "Current local time and date: %s", asctime (timeinfo) );
}

bool check_status(pid_t pid) 
{
    ssize_t read;
    char *line = NULL;
    size_t len = 0;
    FILE * fp = fopen(PROC_FILE, "a+");
    if(!fp) {
        perror("file doesn't exist\n");
        return -1;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        char[20] pid_buf;
        if(strcmp(line, itoa(pid, pid_buf, 10))==0)){
            return true;
        }
    }

    return false;
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
    unsigned long per = strtoul(argv[1], NULL, 10);
    unsigned long proc_t = strtoul(argv[2], NULL, 10);

    pid_t pid = getpid();

    reg(pid, per, proc_t); //Proc filesystem

    if (!check_status(pid)) exit 1; //Proc filesystem: Verify the process was admitted 

    //setup everything needed for real-time loop: t0=gettimeofday() 
    yield(pid); //Proc filesystem
    //this is the real-time loop
    while(/*exist jobs*/1)
    {
		do_job(); //wakeup_time=gettimeofday()-t0 and factorial computation
		yield(pid); //Proc filesystem. JobProcessTime=gettimeofday()-wakeup_time }
		//UNREGISTER(pid); //Proc filesystem }
	}
}

