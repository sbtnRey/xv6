#ifdef CS333_P5
#include "types.h"
#include "user.h"

int
main(int argc, char * argv[])
{
  int gid = atoi(argv[1]);

	if(chgrp(argv[2], gid) < 0)
		printf(1, "Invalid Arguments\n");
  exit();
}

#endif
