#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void
main(void) {
	int file;
	char filename[40];
	sprintf(filename,"/dev/i2c_drv0");
	if ((file = open(filename,O_RDWR)) < 0) {
		printf("Failed to open the bus.");
		/* ERROR HANDLING; you can check errno to see what went wrong */
		exit(1);
	}
	//char buf[32] = {0X00, 0X50, 0XAA, 0X55, 0X33, 0XEE};
	char buf[32] = {0X50, 0x00, 0x55, 0X80, 0X40, 0X45};
	float data;
	char channel;
	int i;
	 //Write the eeprom offset and data
	if (write(file,buf,6) == 6) {
		sleep(1);
		// Write the offset for reading
		//write(file, buf, 2);
#if 1
		for (i = 0; i < 32; i++)
			buf[i] = 0;
#endif
		// Read 32 bytes
		read(file, buf, 32);
		for (i = 0; i < 32; i++)
		{
			if (!(i % 15))
				printf("\n");
			printf("%d\t", buf[i]);
		}
#if 1
	} else {
		printf("Failed to write the i2c bus.\n");
		printf("\n\n");
	}
#endif
}
