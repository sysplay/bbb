#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "gpio.h"

int main(int argc, char *argv[])
{
	char *filename;
	int fd;
	int choice;
	LEDData ld = { .num = 3 };
	GPIOData gd = { .num = 72 };

	if (argc != 2)
	{
		printf("Usage: %s <device_file_name>\n", argv[0]);
		return 1;
	}
	else
	{
		filename = argv[1];
	}

	fd = open(filename, O_RDWR);
	if (fd == -1)
	{
		perror("gpio_ops open");
		return 1;
	}

	do
	{
		printf(" 0: Exit\n");
		printf(" 1: Get LED %d Status from BBB\n", ld.num);
		printf(" 2: Switch on LED %d of BBB\n", ld.num);
		printf(" 3: Switch off LED %d of BBB\n", ld.num);
		printf(" 4: Change LED selection of BBB\n");
		printf(" 5: Read button of BBB\n");
		printf("Enter choice: ");
		scanf("%d", &choice);
		getchar();
		switch (choice)
		{
			case 1:
				if (ioctl(fd, BBB_LED_GET, &ld) == -1)
				{
					perror("gpio_ops ioctl");
					break;
				}
				printf(" LED is %s\n", (ld.val == 0) ? "Off" : "On");
				break;
			case 2:
				printf(" Switching On LED ... ");
				ld.val = 1;
				if (ioctl(fd, BBB_LED_SET, &ld) == -1)
				{
					perror("gpio_ops ioctl");
					printf("failed\n");
					break;
				}
				printf("done\n");
				break;
			case 3:
				printf(" Switching Off LED ... ");
				ld.val = 0;
				if (ioctl(fd, BBB_LED_SET, &ld) == -1)
				{
					perror("gpio_ops ioctl");
					printf("failed\n");
					break;
				}
				printf("done\n");
				break;
			case 4:
				printf("Enter LED to select [0-3]: ");
				scanf("%d", &ld.num);
				break;
			case 5:
				if (ioctl(fd, BBB_GPIO_GET, &gd) == -1)
				{
					perror("gpio_ops ioctl");
					break;
				}
				printf(" Switch is %s\n", (gd.val == 0) ? "Pressed" : "Released");
				break;
		}
	} while (choice != 0);

	close (fd);
	return 0;
}
