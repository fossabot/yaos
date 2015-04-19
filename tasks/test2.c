#include <foundation.h>

#include <time.h>
static void test_task2()
{
	unsigned long long t;
	unsigned th, bh;
	unsigned v;

	printf("test2()\n");
	printf("sp : %x, lr : %x\n", GET_SP(), GET_LR());

	while (1) {
		t = get_systick_64();
		th = t >> 32;
		bh = t;

		dmb();
		v = systick;

		printf("%08x ", v);
		printf("%08x", th);
		printf("%08x %d (%d sec)\n", bh, v, v/HZ);

		mdelay(500);
	}
}

#include <kernel/task.h>
REGISTER_TASK(test_task2, DEFAULT_STACK_SIZE, DEFAULT_PRIORITY);
