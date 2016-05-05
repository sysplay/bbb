#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/ioctl.h>

//#include "../CharDriver/led_ioctl.h"

main()
{
	int choice;
	int fd = -1;
	unsigned char data[128];
	int cnt;
	int i;
	int delay;

	do
	{
		printf("1: Open\n");
		printf("2: Read\n");
		printf("3: Write\n");
		printf("4: Close\n");
		printf("5: Ioctl\n");
		printf("0: Exit\n");
		printf("Enter choice: ");
		scanf("%d", &choice);
		getchar();
		switch (choice)
		{
			case 1:
				fd = open("/dev/mychar0", O_RDWR);
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
				printf("Bytes to read: ");
				scanf("%d", &cnt);
				getchar();
				cnt = read(fd, data, cnt);
				if (cnt == -1)
				{
					perror("file_app read");
					break;
				}
				printf("Read %d bytes in %p: ", cnt, data);
				for (i = 0; i < cnt; i++)
				{
					printf("%02X", data[i]);
				}
				printf("\n");
				break;
			case 3:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				printf("Enter your string: ");
				scanf("%[^\n]", data);
				getchar();
				cnt = write(fd, data, strlen(data));
				if (cnt == -1)
				{
					perror("file_app write");
					break;
				}
				printf("Wrote %d bytes from %p\n", cnt, data);
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
#if 0
			case 5:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				printf("Enter the delay in ms: ");
				scanf("%d", &delay);
				getchar();
				if (ioctl(fd, LED_SET_CHAR_DELAY, delay) == -1)
				{
					perror("file_app ioctl");
					break;
				}
				break;
#endif
		}
	} while (choice != 0);

	return 0;
}
