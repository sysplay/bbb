#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define GPIO_FILE1	"/dev/mychar0"
#define GPIO_FILE2	"/dev/gpio0"

int main()
{
	int fd1, fd2;
	fd_set input, tset;
	int max_fd;
	char c;

	if ((fd1 = open(GPIO_FILE1, O_RDWR)) == -1)
	{
		perror("Error Opening File: ");
		return -1;
	}
	if ((fd2 = open(GPIO_FILE2, O_RDWR)) == -1)
	{
		perror("Error Opening File: ");
		return -1;
	}
	FD_ZERO(&input);
	FD_SET(fd1, &input);
	FD_SET(fd2, &input);
	tset = input;
	max_fd = ((fd1 > fd2)? (fd1 + 1):(fd2 + 1));

	while (select(max_fd, &tset, NULL, NULL, NULL) > 0) {
		if (FD_ISSET(fd1, &tset)) {
			printf("Got input on first device file\n");
			if (read(fd1, &c, 1) > 0) {
				printf("Value read is %c\n", c);
			}
		}
		else if (FD_ISSET(fd2, &tset)) {
			printf("Got input on second device file\n");
			if (read(fd2, &c, 1) > 0) {
				printf("Value read is %c\n", c);
			}
		}
		tset = input;
	}
	return 0;
}
