#ifdef CS333_P5
#include "types.h"
#include "user.h"

int atoo(char *);

int
main(int argc, char * argv[])
{
  int perms = atoo(argv[1]);
	
	int len;
	len = strlen(argv[1]);
	
	if (len != 4){
		printf(1, "\nInvalid mode\n");
		exit();
	}

	if(chmod(argv[2], perms) < 0)
		printf(1, "Invalid Arguments\n");
  exit();
}

#endif
