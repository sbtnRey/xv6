#ifdef CS333_P5
#include "types.h"
#include "user.h"

int
main(int argc, char * argv[])
{
  int uid = atoi(argv[1]);

	if(chown(argv[2], uid) < 0)
		printf(1, "Invalid Arguments\n");
  exit();
}

#endif
