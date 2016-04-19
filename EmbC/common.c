#include "common.h"

void __attribute__((optimize("O0"))) delay(uint32_t nr_of_nops)
{
	// compiler optimizations disabled for this function using __attribute__,
	// pragma is as well possible to protect bad code...
	// http://stackoverflow.com/questions/2219829/how-to-prevent-gcc-optimizing-some-statements-in-c

	uint32_t counter = 0;

	for (; counter < nr_of_nops; ++counter)
	{
		;
	}
}
/*
void __attribute__((optimize("O0"))) delay_us(uint32_t usecs)
{
	delay(usecs);
	delay(usecs);
	delay(usecs);
}
 */
