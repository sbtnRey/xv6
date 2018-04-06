#ifdef CS333_P2
#include "types.h"
#include "user.h"

int
testuidgid(void)
{
	uint uid, gid, ppid;

	uid = getuid();
	printf(2, "Current UID is: %d\n", uid);

	printf(2, "Setting UID to 100\n");
	if (setuid(100) < 0)
		printf(1, "Error setting UID\n");
	
	uid = getuid();
	printf(2, "Current UID is: %d\n\n", uid);

	printf(2, "Setting UID to -1\n");
	if (setuid(-1) < 0)
		printf(1, "Error setting UID, UID was not set to a negative number\n");
	
	uid = getuid();
	printf(2, "Current UID is: %d\n\n", uid);

	printf(2, "Setting UID to 65478\n");
	if (setuid(65478) < 0)
		printf(1, "Error setting UID, UID not set to value over max\n");
	
	uid = getuid();
	printf(2, "Current UID is: %d\n\n", uid);



	gid = getgid();
	printf(2, "Current GID is %d\n", gid);
	printf(2, "Setting GID to 100\n");
	if(setgid(100) < 0)
		printf(2, "Error setting GID");
	gid = getgid();
	printf(2, "Current GID is: %d\n\n", gid);

	gid = getgid();
	printf(2, "Setting GID to -1\n");
	if(setgid(-1) < 0)
		printf(2, "Error setting GID. GID was not set to a negative value\n");
	gid = getgid();
	printf(2, "Current GID is: %d\n\n", gid);

	gid = getgid();
	printf(2, "Setting GID to 65478\n");
	if(setgid(65478) < 0)
		printf(2, "Error setting GID. GID not set to value over max\n");
	gid = getgid();
	printf(2, "Current GID is: %d\n\n", gid);

	ppid = getppid();
	printf(2, "MY parent process is: %d\n", ppid);
	printf(2, "Done!\n");

	return 0;
}

 

int
main(int argc, char *argv[])
{
	testuidgid();
	exit();
}
#endif
