#include <foundation.h>
#include <kernel/sched.h>

static void rt_task1()
{
	while (1) {
		printf("REALTIME START\n");
		mdelay(5000);
		printf("REALTIME END\n");
		reset_task_state(current, TASK_RUNNING);
		schedule();
	}
}

REGISTER_TASK(rt_task1, DEFAULT_STACK_SIZE, RT_LEAST_PRIORITY);