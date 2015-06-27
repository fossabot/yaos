#include <foundation.h>
#include <kernel/task.h>

static void test_task()
{
	unsigned long long t;
	unsigned int th, bh;
	unsigned int v;

	printf("test()\n");
	printf("sp : %x, lr : %x\n", GET_SP(), GET_LR());

	while (1) {
		t = get_jiffies_64();
		th = t >> 32;
		bh = t;

		dmb();
		v = jiffies;

		printf("%08x ", v);
		printf("%08x", th);
		printf("%08x %d (%d sec)\n", bh, v, v/HZ);

		sleep(1);
	}
}
REGISTER_TASK(test_task, 0, DEFAULT_PRIORITY);
