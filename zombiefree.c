#ifdef CS333_P3P4
#include "types.h"
#include "user.h"


int 
testOneTwoSix()
{

	int pid;

		
	while((pid = fork()) != -1){
		if (pid == 0)
			exit();
	}
  sleep(10000);
	while(wait() != -1);
		exit();

	return 0;
}

int 
testThree()
{
	int pid = fork();
	
	if(!pid)
			while(1);
		printf(1, "PID: %d\n", pid);

	while (wait() != -1);
	
	return 0;
}

int 
testSeven()
{

	int i;

	for(i = 0; i < 5; i++){
		if(!fork())
			for(;;);
	}

	while(wait() != -1);
	
	return 0;
}

int 
main(int argc, char *argv[])
{
	//testThree();
	//testOneTwoSix();
	testSeven();
	
	exit();
}
#endif
