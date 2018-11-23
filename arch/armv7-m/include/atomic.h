#ifndef __YAOS_ARMv7M_ATOMIC_H__
#define __YAOS_ARMv7M_ATOMIC_H__

#include <stdint.h>

#if !defined(TEST)
#define __ldrex(addr) __extension__ ({			\
	unsigned int __result = 0;			\
	__asm__ __volatile__("ldrex %0, [%1]"		\
		: "=r"(__result)			\
		: "r"(addr)				\
		: "cc", "memory");			\
	__result;					\
})

/* return 0 if success */
#define __strex(value, addr) __extension__ ({		\
	unsigned int __result = 0;			\
	__asm__ __volatile__("strex %0, %2, [%1]"	\
		: "=r"(__result)			\
		: "r"(addr), "r"(value)			\
		: "cc", "memory");			\
	__result;					\
})

#define __clrex()					\
	__asm__ __volatile__("clrex" ::: "cc", "memory")
#else /* defined(TEST) */
#define __ldrex(addr)		(*(addr))
#define __clrex()
static inline int __strex(uintptr_t value, uintptr_t *addr)
{
	*addr = value;
	return 0;
}
#endif

#endif /* __YAOS_ARMv7M_ATOMIC_H__ */
