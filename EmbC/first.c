static inline void init_shell(void)
{
}

static inline void loop_forever(void)
{
}

int main(void)
//int c_entry(void)
{
	init_shell();
	for (;;)
		loop_forever();

	return 0;
}
