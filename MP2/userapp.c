#include "userapp.h"
#include <stdlib.h>

 // * params: char *write_path: registration file
 // * return: on success, return the number of bytes written; on failure return a negative number
int reg(char* write_path)
{
    FILE * fp = fopen (write_path, "a+");
    if(!fp)
    {   
        perror ("file doesn't exist\n");
        return -1; 
    }   
    pid_t pid = getpid();
    int byte_write = fprintf(fp, "R:%d", pid);
    fclose(fp);
    return byte_write;
}

 // * params: char *write_path: registration file
 // * return: on success, return the number of bytes written; on failure return a negative number
int unreg(char* write_path, pid_t pid)
{
    FILE * fp = fopen (write_path, "a+");
    if(!fp)
    {   
        perror ("file doesn't exist\n");
        return -1; 
    }   
    int byte_write = fprintf(fp, "D:%d", pid);
    fclose(fp);
    return byte_write;
}

int main(int argc, char* argv[])
{
    if(argc < 3)
    {   
        perror("Number of arguments wrong, please follow [filepath] [cmd] [args]\n");
        return -1; 
    }

    char *cmd = argv[2]
    char *write_path = argv[1];
    if (*cmd == 'r') {
        reg (write_path);
    }
    else if (*cmd == 'd') {
        pid_t pid = atoi(argv[3]);
        unreg (write_path, pid);
    }
    else{

    }
    while(1);
    return 0;
}