#ifdef CS333_P3P4
#include "types.h"
#include "user.h"
#include "uproc.h"
#include "syscall.h"

int main(int argc, char *argv[])
{
	//int rc;
	
	if(argc != 3)
		exit();
	
	setpriority(atoi(argv[1]), atoi(argv[2]));

	exit();
}
#endif

	
