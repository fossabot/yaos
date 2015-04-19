#ifndef __TYPES_H__
#define __TYPES_H__

#define NULL			((void *)0)
#define EOF			(-1)

#define HIGH			1
#define LOW			0
#define ON			1
#define OFF			0
#define ENABLE			1
#define DISABLE			0

typedef enum {FALSE = 0, TRUE = 1} bool;
typedef unsigned long size_t;
typedef unsigned long long uint64_t;

#define get_container_of(ptr, type, member) \
		((type *)((char *)ptr - (char *)&((type *)0)->member))

/* double linked list */
struct list_t {
	struct list_t *next;
	struct list_t *prev;
};

#define LIST_HEAD_INIT(name) 	{ &(name), &(name) }
#define LIST_HEAD(name) \
		struct list_t name = LIST_HEAD_INIT(name)

extern inline void LIST_LINK_INIT(struct list_t *list);
extern inline void list_add(struct list_t *new, struct list_t *ref);
extern inline void list_del(struct list_t *item);

/* fifo */
struct fifo_t {
	unsigned size;
	unsigned front, rear;
	void     *buf;
};

extern inline void fifo_init(struct fifo_t *q, void *queue, unsigned size);
extern inline int  fifo_get(struct fifo_t *q, int type_size);
extern inline int  fifo_put(struct fifo_t *q, int value, int type_size);
extern inline void fifo_flush(struct fifo_t *q);

#define SWAP_WORD(word)	\
		((word >> 24) | (word << 24) | ((word >> 8) & 0xff00) | ((word << 8) & 0xff0000))

#endif /* __TYPES_H__ */
