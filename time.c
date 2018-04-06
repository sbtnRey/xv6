#ifdef CS333_P2
#include "types.h"
#include "user.h"

// look date.c , usertests.c for exec and echo, uptime in sysproc.
int
main(int argc, char *argv[])
{

	int starttime;
	int endtime;
	int pid;
	int seconds;
	int miliseconds;
	
	// create pipe
	int fd[2];
	int flag;
	pipe(fd);

	
	starttime = uptime();
	endtime = uptime();
	seconds = (endtime - starttime)/1000;
	miliseconds = (endtime - starttime) % 1000;	
	
	if(argc == 1){
		
	printf(1, "time ran in ");
	if(miliseconds < 10)
		printf(1, "%d.00%d", seconds, miliseconds);
	else if(miliseconds < 100)
		printf(1, "%d.0%d", seconds, miliseconds);
	else
		printf(1, "%d.%d", seconds, miliseconds);
	
		printf(1, " seconds.\n");	
		exit();
	}	


	
	// if less than 0 print error of some sort
	if ((pid = fork()) == 0){ // added argc == 1
  	if(exec(argv[1], &argv[1]) < 0){
			printf(1, "Invalid command\n");
			close(fd[0]);
			flag = 1;
			write(fd[1], &flag, sizeof(flag));
			close(fd[1]);
			exit();
		}
	}
	

	wait(); // parent is waiting
	kill(pid); // killing the child :D

	close(fd[1]);
	read(fd[0], &flag, sizeof(flag));
	// invalid command, exit 
	if (flag == 1)
		exit();
	close(fd[0]);
	
	endtime = uptime();
	seconds = (endtime - starttime)/1000;
	miliseconds = (endtime - starttime) % 1000;
		
	printf(1, "%s ran in ", argv[1]);
	if(miliseconds < 10)
		printf(1, "%d.00%d", seconds, miliseconds);
	else if(miliseconds < 100)
		printf(1, "%d.0%d", seconds, miliseconds);
	else
		printf(1, "%d.%d", seconds, miliseconds);
	
	printf(1, " seconds.\n");	

	

  exit();
}

#endif
