#include <stdio.h>

static inline long read_rsp(void)
{
	volatile register long rsp asm("rsp");
	return rsp;
}

static void foo(void *p)
{
	printf("%s:%d: p=%p\n", __func__, __LINE__, p);
}

static void bar(void)
{
	long rsp_enter, rsp_exit;

	rsp_enter = read_rsp();

	foo((void *)rsp_enter);

	rsp_exit = read_rsp();

	if (rsp_enter == (rsp_exit + 8)) {
		volatile register long rsp asm("rsp");
		/* Fix the rsp register. */
		rsp += 8;
		printf("Fix rsp: rsp enter %lx, rsp exit %lx.\n",
				rsp_enter, rsp_exit);
	} else if (rsp_enter == rsp_exit) {
		printf("rsp enter=%lx, rsp exit=%lx, rsp exit+8=%lx\n",
				rsp_enter, rsp_exit, rsp_exit + 8);
	}
}

int main()
{
	bar();
	return 0;
}
