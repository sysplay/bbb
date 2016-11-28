#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "debug_ioctl.h"

int main(int argc, char *argv[])
{
	char *file_name = "/dev/debug";
	int choice;
	int fd = -1;
	unsigned char wdata[128];
	unsigned char *rdata;
	int cnt;
	int i;
	off_t seek;
	struct debug_st d;

	if (argc > 2)
	{
		fprintf(stderr, "Usage: %s <device file name>\n", argv[0]);
		return 1;
	}
	else if (argc == 2)
	{
		file_name = argv[1];
	}

	fd = open(file_name, O_RDWR);
	if (fd == -1)
	{
		perror("file_ops open");
		return 2;
	}

	do
	{
		printf("1: Read\n");
		printf("2: Write\n");
		printf("3: Seek\n");
		printf("4: Debug Get\n");
		printf("0: Exit\n");
		printf("Enter choice: ");
		scanf("%d", &choice);
		getchar();
		switch (choice)
		{
			case 1:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				printf("Bytes to read: ");
				scanf("%d", &cnt);
				getchar();
				cnt = read(fd, rdata, cnt);
				if (cnt == -1)
				{
					perror("file_ops read");
					break;
				}
				printf("Read %d bytes in %p: ", cnt, rdata);
				for (i = 0; i < cnt; i++)
				{
					printf("%c (%02X) ", rdata[i], rdata[i]);
				}
				printf("\n");
				break;
			case 2:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				printf("Enter your string: ");
				scanf("%[^\n]", wdata);
				getchar();
				cnt = write(fd, wdata, strlen(wdata));
				if (cnt == -1)
				{
					perror("file_ops write");
					break;
				}
				printf("Wrote %d bytes from %p\n", cnt, wdata);
				break;
			case 3:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				printf("Enter the seek from start of device file: ");
				scanf("%lld", &seek);
				getchar();
				if (lseek(fd, seek, SEEK_SET) == -1)
				{
					perror("file_ops lseek");
					break;
				}
				break;
			case 4:
				if (fd == -1)
				{
					printf("File not open\n");
					break;
				}
				if (ioctl(fd, GET_DEBUG_INFO, &d) == -1)
				{
					perror("file_ops ioctl");
					break;
				}
				printf("Debug state: %d; Value: %c\n", d.state, d.buf);
				break;
		}
	} while (choice != 0);

	close (fd);

	return 0;
}
