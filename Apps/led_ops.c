#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


main()
{
	int choice;
	int fd = -1;
	unsigned char c;
	int cnt = 1;
	int i;
	int delay;

	do
	{
		printf("1: Open\n");
		printf("2: Get Led Status\n");
		printf("3: Set Led Status\n");
		printf("4: Close\n");
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
					perror("file_app open");
				}
				break;
			case 2:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				cnt = read(fd, &c, 1);
				if (cnt < 0)
				{
					printf("Can't get the status");
				}
				else
				{
					printf("\nGpio Value is %d", c);
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
					perror("file_app write");
					break;
				}
				break;
			case 4:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				close (fd);
				fd = -1;
				break;
			default:
				printf("Invalid Choice\n");
		}
	} while (choice != 0);

	return 0;
}
