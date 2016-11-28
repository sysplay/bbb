#ifndef _IOCTL_DEFS

#define _IOCTL_DEFS

#include <linux/ioctl.h>

struct debug_st
{
	int state;
	char buf;
};

#define GET_DEBUG_INFO _IOR('d', 1, struct debug_st *)

#endif
