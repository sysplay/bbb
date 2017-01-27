#include <pthread.h>
#include <stdio.h>

struct my_val
{
	int a;
	int b;
};

void *thread(void *var)
{
	struct my_val *p = (struct my_val *)var;
	static int c = 0;

	if (c == 0)
	{
		c = p->a + p->b;
		printf("The result is %d, addr: %p\n", c, &c);
	}
	else
	{
		c = p->a - p->b;
		printf("The result is %d, addr: %p\n", c, &c);
	}
	return ((void*)&c);
}

int main()
{
	struct my_val var;
	pthread_t id1, id2;
	int *rel1, *rel2;

	printf("Enter 2 integer values: ");
	scanf("%d%d", &var.a, &var.b);
	pthread_create(&id1, NULL, &thread, &var);
	pthread_create(&id2, NULL, &thread, &var);
	pthread_join(id1, (void*)&rel1);
	printf("The result from the first thread: %d, addr: %p\n", *rel1, rel1);
	pthread_join(id2, (void*)&rel2);
	printf("The result from the second thread: %d, addr: %p\n", *rel2, rel2);
	return 0;
}
