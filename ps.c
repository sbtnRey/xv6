#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"
#include "syscall.h"

#define MAX 64

int
main(void)
{
	
	int numproc;
	int miliseconds;
	int seconds;
	struct uproc *u = malloc(MAX * sizeof(struct uproc));


	numproc = getprocs(MAX, u);

	printf(1, "PID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\n");
	int i;
	for ( i = 0; i < numproc; ++i){

		miliseconds = u[i].elapsed_ticks / 1000;
		seconds = u[i].elapsed_ticks % 1000;
  	
		printf(1, "%d\t", u[i].pid);


		printf(1, "%s\t", u[i].name);

		printf(1, "%d\t", u[i].uid);

		printf(1, "%d\t", u[i].gid);

		printf(1, "%d\t", u[i].ppid);
		#ifdef CS333_P3P4	
		printf(1, "%d\t", u[i].priority);
		#endif
		//pass elapsed time in procs	
		if(miliseconds < 10)
			printf(1, "%d.00%d\t", seconds, miliseconds);
		else if(miliseconds < 100)
			printf(1, "%d.0%d\t", seconds, miliseconds);
		else
			printf(1, "%d.%d\t", seconds, miliseconds);

		printf(1, "0.%d\t", u[i].CPU_total_ticks % 100);
		
		printf(1, "%s\t", u[i].state);

		printf(1, "%d\t", u[i].size);

		printf(1, "\n");
	}

	free(u);
  exit();
}
#endif
