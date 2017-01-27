#ifndef _GPIO_H
#define _GPIO_H

#include <linux/ioctl.h>

typedef struct
{
	int num;
	int val;
} GPIOData, LEDData;

#define BBB_GPIO_GET _IOR('b', 1, GPIOData *)
#define BBB_GPIO_SET _IOW('b', 2, GPIOData *)
#define BBB_LED_GET _IOR('b', 3, LEDData *)
#define BBB_LED_SET _IOW('b', 4, LEDData *)

#ifdef __KERNEL__

#define _BBB_GPIO_GET 0
#define _BBB_GPIO_SET 1

typedef	struct
{
	int cmd;
	GPIOData d;
} GPIOFData;

#endif

#endif /* _GPIO_H */
