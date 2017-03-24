#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "led_ioctl.h"

int main(void)
{
	int choice;
	int fd = -1;
	unsigned char c;
	int cnt = 1;
	int i;
	int led_num;

	do
	{
		printf("1: Open\n");
		printf("2: Get LED Status\n");
		printf("3: Set LED Status\n");
		printf("4: Set LED Number\n");
		printf("5: Close\n");
		printf("0: Exit\n");
		printf("Enter choice: ");
		scanf("%d", &choice);
		getchar();
		switch (choice)
		{
			case 0: 
				break;
			case 1:
				fd = open("/dev/gpio_drv0", O_RDWR);
				if (fd == -1)
				{
					perror("led_ops open");
				}
				break;
			case 2:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				cnt = read(fd, &c, 1);
				if (cnt == -1)
				{
					perror("led_ops read");
				}
				else
				{
					printf("\nLED Value is %d", c);
				}
				printf("\n");
				break;
			case 3:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				printf("Enter your choice [0 - Turn Off, 1- Turn On] :\n");
				c = getchar();
				cnt = write( fd, &c, 1 );
				if (cnt == -1)
				{
					perror("led_ops write");
					break;
				}
				break;
			case 4:
				printf("Enter the LED Number (1 to 4): ");
				scanf("%d", &led_num);
				getchar();
				if (ioctl(fd, GPIO_SELECT_LED, led_num) == -1)
				{
					perror("led_ops ioctl");
					break;
				}
				break;
			case 5:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				close(fd);
				fd = -1;
				break;
			default:
				printf("Invalid Choice\n");
				break;
		}
	} while (choice != 0);

	return 0;
}
