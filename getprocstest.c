#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "syscall.h"
#include "uproc.h"
#include "syscall.h"


int
testgetproc(int size)
{

	int numproc;
	struct uproc *u = malloc(size * sizeof(struct uproc));
	
	numproc = getprocs(size, u);
	
	printf(1, "MAX = %d\n\n", numproc);

	printf(1, "PID\tName\n");
	int i;	
	for (i = 0; i < numproc; ++i){
	
		printf(1, "%d\t", u[i].pid);
		printf(1, "%s\t", u[i].name);
		printf(1, "\n");
	}
	
	
	free(u);


	return 0;
}

int 
main(int argc, char *argv[])
{
	int pid;

	while ((pid = fork()) != -1) {

		if(pid != 0){
			wait();
			kill(pid);
			exit();
		}
	}
	
	testgetproc(1);
	testgetproc(16);
	testgetproc(64);
	testgetproc(72);

	exit();

}
#endif
