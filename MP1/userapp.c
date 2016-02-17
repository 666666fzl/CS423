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
	int byte_write = fprintf(fp, "%d\n", pid);
	return byte_write;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		perror("Number of arguments wrong\n");
		return -1;
	}

	char *write_path = argv[1];
	reg (write_path);
	return 0;
}

