/*
 * "[...] Sincerity (comprising truth-to-experience, honesty towards the self,
 * and the capacity for human empathy and compassion) is a quality which
 * resides within the laguage of literature. It isn't a fact or an intention
 * behind the work [...]"
 *
 *             - An introduction to Literary and Cultural Theory, Peter Barry
 *
 *
 *                                                   o8o
 *                                                   `"'
 *     oooo    ooo  .oooo.    .ooooo.   .oooo.o     oooo   .ooooo.
 *      `88.  .8'  `P  )88b  d88' `88b d88(  "8     `888  d88' `88b
 *       `88..8'    .oP"888  888   888 `"Y88b.       888  888   888
 *        `888'    d8(  888  888   888 o.  )88b .o.  888  888   888
 *         .8'     `Y888""8o `Y8bod8P' 8""888P' Y8P o888o `Y8bod8P'
 *     .o..P'
 *     `Y8P'                   Kyunghwan Kwon <kwon@yaos.io>
 *
 *  Welcome aboard!
 */

#include <kernel/lock.h>
#include <error.h>

#if 0
int __attribute__((naked, noinline, leaf)) __ldrex(void *addr)
{
	__asm__ __volatile__(
			"ldrex	r0, [r0]		\n\t"
			"bx	lr			\n\t"
			::: "cc", "memory");
}

int __attribute__((naked, noinline, leaf)) __strex(int val, void *addr)
{
	__asm__ __volatile__(
			"strex	r0, r0, [r1]		\n\t"
			"bx	lr			\n\t"
			::: "cc", "memory");
}
#endif

void __attribute__((naked, noinline)) __semaphore_dec(struct semaphore *sem, int ms)
{
	__asm__ __volatile__(
			"push	{r8, lr}		\n\t"
			"mov	r8, r0			\n\t"
		"1:"	"ldrex	r2, [r8]		\n\t"
			"cmp	r2, #0			\n\t"
			"bgt	2f			\n\t"
			"add	r0, r8, #4		\n\t"
			"bl	sleep_in_waitqueue	\n\t"
			"b	1b			\n\t"
		"2:"	"sub	r2, #1			\n\t"
			"strex	r0, r2, [r8]		\n\t"
			"cmp	r0, #0			\n\t"
			"bne	1b			\n\t"
			"dmb				\n\t"
			"pop	{r8, pc}		\n\t"
			::: "r0", "r1", "r2", "r8", "lr", "cc", "memory");
	(void)sem;
	(void)ms;
}

int __attribute__((naked, noinline)) __semaphore_dec_wait(struct semaphore *sem, int ms)
{
	__asm__ __volatile__(
			"push	{r8, lr}		\n\t"
			"mov	r8, r0			\n\t"
		"1:"	"ldrex	r2, [r8]		\n\t"
			"cmp	r2, #0			\n\t"
			"bgt	2f			\n\t"
			"add	r0, r8, #4		\n\t"
			"bl	sleep_in_waitqueue	\n\t"
			"cmp	r0, %0			\n\t"
			"beq	3f			\n\t"
			"b	1b			\n\t"
		"2:"	"sub	r2, #1			\n\t"
			"strex	r0, r2, [r8]		\n\t"
			"cmp	r0, #0			\n\t"
			"bne	1b			\n\t"
		"3:"	"dmb				\n\t"
			"pop	{r8, pc}		\n\t"
			:: "L"(-ETIME)
			: "r0", "r1", "r2", "r8", "lr", "cc", "memory");
	(void)sem;
	(void)ms;
}

void __attribute__((naked, noinline)) __semaphore_inc(struct semaphore *sem)
{
	__asm__ __volatile__(
			"push	{r8, lr}		\n\t"
			"mov	r8, r0			\n\t"
		"1:"	"ldrex	r2, [r8]		\n\t"
			"add	r2, #1			\n\t"
			"strex	r0, r2, [r8]		\n\t"
			"cmp	r0, #0			\n\t"
			"bne	1b			\n\t"
			"dmb				\n\t"
			"cmp	r2, #0			\n\t"
			"itt	gt			\n\t"
			"addgt	r0, r8, #4		\n\t"
			"blgt	shake_waitqueue_out	\n\t"
			"pop	{r8, pc}		\n\t"
			::: "r0", "r2", "r8", "lr", "cc", "memory");
	(void)sem;
}

void __attribute__((naked, noinline)) __lock_atomic(lock_t *counter)
{
	__asm__ __volatile__(
			"mov	r1, r0			\n\t"
		"1:"	"ldrex	r2, [r1]		\n\t"
			"cmp	r2, #0			\n\t"
			"ble	1b			\n\t"
			"sub	r2, #1			\n\t"
			"strex	r0, r2, [r1]		\n\t"
			"cmp	r0, #0			\n\t"
			"bne	1b			\n\t"
			"dmb				\n\t"
			"bx	lr			\n\t"
			::: "r0", "r1", "r2", "cc", "memory");
	(void)counter;
}

void __attribute__((naked, noinline)) __unlock_atomic(lock_t *counter)
{
	__asm__ __volatile__(
			"mov	r1, r0			\n\t"
		"1:"	"ldrex	r2, [r1]		\n\t"
			"add	r2, #1			\n\t"
			"strex	r0, r2, [r1]		\n\t"
			"cmp	r0, #0			\n\t"
			"bne	1b			\n\t"
			"dmb				\n\t"
			"bx	lr			\n\t"
			::: "r0", "r1", "r2", "cc", "memory");
	(void)counter;
}
