#include "userapp.h"

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
int unreg(char* write_path)
{
    FILE * fp = fopen (write_path, "a+");
    if(!fp)
    {   
        perror ("file doesn't exist\n");
        return -1; 
    }   
    pid_t pid = getpid();
    int byte_write = fprintf(fp, "D:%d", pid);
    fclose(fp);
    return byte_write;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {   
        perror("Number of arguments wrong, please follow [cmd] [filepath]\n");
        return -1; 
    }

    char *cmd = argv[1];
    char *write_path = argv[2];
    if (*cmd == 'r') {
        reg (write_path);
    }
    else if (*cmd == 'd') {
        unreg (write_path);
    }
    else{

    }
    while(1);
    return 0;
}
