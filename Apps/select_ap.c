#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define GPIO_FILE	"/dev/mychar0"

int main()
{
	int fd;
	fd_set input, tset;
	int max_fd;
	char c;

	if ((fd = open(GPIO_FILE, O_RDWR)) == -1)
	{
		perror("Error Opening File: ");
		return -1;
	}
	FD_ZERO(&input);
	FD_SET(fd, &input);
	tset = input;
	max_fd = fd + 1;
	while (select(max_fd, &tset, NULL, NULL, NULL) > 0) 
	{
		if (FD_ISSET(fd, &tset)) 
		{
			printf("Got Select\n");
			if (read(fd, &c, 1) > 0) 
			{
				printf("Value read is %c\n", c);
			}
		}
		tset = input;
	}
	return 0;
}
