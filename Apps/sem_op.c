#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main () 
{
	int i, fd;
	char ch, write_buf[100], read_buf[100];

	fd = open("/dev/mychar0", O_RDWR);

	if (fd == -1)
	{
		printf("Error in opening file \n");
		return 1;
	}
	printf ("Press r to read from device or w to write the device ");
	scanf ("%c", &ch); 

	switch (ch)
	{
		case 'w':
			printf (" Enter the data to be written into device");
			scanf (" %[^\n]", write_buf);
			write(fd, write_buf, sizeof(write_buf));
			break;
		case 'r':
			read(fd, read_buf, sizeof(read_buf));
			printf ("The data in the device is %s\n", read_buf);
			break;
		default:
			printf("Wrong choice \n");
			break;
	}
	close(fd);
}
