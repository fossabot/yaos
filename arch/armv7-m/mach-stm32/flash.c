#include <io.h>
#include <kernel/lock.h>
#include <kernel/page.h>
#include <kernel/module.h>
#include <error.h>

#include "flash.h"

#ifndef stm32f1
#define stm32f1	1
#define stm32f4	2
#endif

#define BLOCK_LEN		(BLOCK_SIZE / WORD_SIZE)

#if (SOC == stm32f4)
static inline unsigned int get_sector(unsigned int addr)
{
	unsigned int sector = ((addr >> 12) & 0xff) >> 2;

	if (sector > 4)
		sector = (sector >> 3) + 4; /* sector / 8 + 4 */

	return sector;
}
#endif

static DEFINE_SPINLOCK(wlock);

static inline int __attribute__((section(".iap"))) flash_erase(void *addr)
{
	/* FIXME:
	 * Flush I/D cache before erase, noting that I/D cache should be
	 * flushed only when it is disabled. */
	FLASH_CR |= (1 << PER);
#if (SOC == stm32f1 || SOC == stm32f3)
	FLASH_AR = (unsigned int)addr;
#elif (SOC == stm32f4)
	FLASH_CR = (FLASH_CR & ~0x78) | (get_sector((unsigned int)addr) << SNB);
	FLASH_CR = (FLASH_CR & ~(3 << PSIZE)) | (2 << PSIZE);
#endif
	FLASH_CR |= (1 << STRT);
	while (FLASH_SR & (1 << BSY)) ;
	FLASH_CR &= ~(1 << PER);

	return FLASH_SR;
}

static unsigned int __flash_read(void *addr, void *buf, size_t len)
{
	unsigned int *s = (unsigned int *)addr;
	unsigned int *d = (unsigned int *)buf;

	len /= WORD_SIZE;

	while (len--)
		*d++ = *s++;

	return (unsigned int)s - (unsigned int)addr;
}

static size_t flash_read(struct file *file, void *buf, size_t len)
{
	/* check boundary */

	unsigned int nblock, diff;

	nblock = file->offset;
	diff = __flash_read((void *)nblock, buf, len);

	return diff;
}

static int flash_seek(struct file *file, unsigned int offset, int whence)
{
	unsigned int end;
	struct device *dev;

	switch (whence) {
	case SEEK_SET:
		file->offset = offset;
		break;
	case SEEK_CUR:
		if (file->offset + offset < file->offset)
			return -ERR_RANGE;

		file->offset += offset;
		break;
	case SEEK_END:
		dev = getdev(file->inode->dev);
		end = dev->block_size * dev->nr_blocks - 1;
		end = BASE_WORD(end + dev->base_addr);

		if (end - offset < dev->base_addr)
			return -ERR_RANGE;

		file->offset = end - offset;
		break;
	}

	return 0;
}

/* TODO: Support variable block sizes
 * It only works for fixed block size. Support variable block sizes. And
 * a block is too big to hold in memory. Find alternative. Do not allocate
 * memory too much of `BLOCK_SIZE`. */
static size_t __attribute__((section(".iap")))
__flash_write(void *addr, void *buf, size_t len)
{
	unsigned int *tmp, *data, *to;
	unsigned int index, sentinel;

	if ((tmp = kmalloc(BLOCK_SIZE)) == NULL)
		return -ERR_ALLOC;

	len  = BASE_WORD(len); /* to prevent underflow */
	data = (unsigned int *)buf;
	to   = (unsigned int *)addr;
	sentinel = 0;

	spin_lock(&wlock);
	FLASH_UNLOCK();
	FLASH_WRITE_START();

	for (index = BLOCK_LEN; len; len -= WORD_SIZE) {
		if (index >= BLOCK_LEN) {
			index = 0;
			sentinel = (unsigned int)
				to - BLOCK_BASE(to, BLOCK_SIZE);
			sentinel /= WORD_SIZE;

			to = (unsigned int *)BLOCK_BASE(to, BLOCK_SIZE);
			__flash_read(to, tmp, BLOCK_SIZE);
			FLASH_WRITE_END();
			flash_erase(to);
			FLASH_WRITE_START();
		}

		while (sentinel) {
			WRITE_WORD(to, tmp[index]);
#if (SOC == stm32f1 || SOC == stm32f3)
			if (FLASH_SR & 0x14) goto out;
#elif (SOC == stm32f4)
			if (FLASH_SR & 0xf0) goto out;
#endif

			to++;
			index++;
			sentinel--;
		}

		WRITE_WORD(to, *data);
#if (SOC == stm32f1 || SOC == stm32f3)
		if (FLASH_SR & 0x14) goto out;
#elif (SOC == stm32f4)
		if (FLASH_SR & 0xf0) goto out;
#endif

		to++;
		data++;
		index++;
	}

	while (index < BLOCK_LEN) {
		WRITE_WORD(to, tmp[index]);
#if (SOC == stm32f1 || SOC == stm32f3)
		if (FLASH_SR & 0x14) goto out;
#elif (SOC == stm32f4)
		if (FLASH_SR & 0xf0) goto out;
#endif

		to++;
		index++;
	}

out:
	FLASH_WRITE_END();
	FLASH_LOCK();
	spin_unlock(&wlock);

	kfree(tmp);

	if (len || (index != BLOCK_LEN)) {
		unsigned int errcode;

		errcode = FLASH_SR;
#if (SOC == stm32f1 || SOC == stm32f3)
		FLASH_SR |= 0x34; /* clear flags */
#elif (SOC == stm32f4)
		FLASH_SR |= 0xf1; /* clear flags */
#endif
		error("embedfs: can not write %x", errcode);

		return errcode;
	}

	return (unsigned int)to - (unsigned int)addr;
}

static size_t __attribute__((section(".iap")))
flash_write(struct file *file, void *buf, size_t len)
{
	unsigned int nblock, diff;

	nblock = file->offset;
	diff = __flash_write((void *)nblock, buf, len);

	return diff;
}

static struct file_operations ops = {
	.open  = NULL,
	.read  = flash_read,
	.write = flash_write,
	.close = NULL,
	.seek  = flash_seek,
	.ioctl = NULL,
};

#include <kernel/buffer.h>

static int flash_init()
{
#if (SOC == stm32f4)
	FLASH_CR = (FLASH_CR & ~(3 << PSIZE)) | (2 << PSIZE);
#endif

	extern char _rom_size, _rom_start, _etext, _data, _ebss;
	struct device *dev;
	unsigned int end;

	/* whole disk of embedded flash memory */
	if (!(dev = mkdev(0, 0, &ops, "efm")))
		return -ERR_RANGE;

	dev->block_size = BLOCK_SIZE;
	dev->nr_blocks = (unsigned int)&_rom_size / BLOCK_SIZE;

	/* partition for file system */
	if (!(dev = mkdev(MAJOR(dev->id), 1, &ops, "efm")))
		return -ERR_RANGE;

	dev->block_size = BLOCK_SIZE;
	dev->base_addr = (unsigned int)&_etext +
		((unsigned int)&_ebss - (unsigned int)&_data);
	dev->base_addr = ALIGN_BLOCK(dev->base_addr, BLOCK_SIZE);

#if (SOC == stm32f4)
	/* 16KiB sectors only and part of 64KiB block,
	 * total 80KiB only for stm32f4xx family */
	end = (unsigned int)&_rom_start + (BLOCK_SIZE * 5);
#else
	end = (unsigned int)&_rom_start + (unsigned int)&_rom_size;
#endif
	dev->nr_blocks = (end - dev->base_addr) / BLOCK_SIZE;

	mount(dev, "/", "embedfs");

	return 0;
}
MODULE_INIT(flash_init);

#include <asm/power.h>

void flash_protect()
{
#if (SOC == stm32f1 || SOC == stm32f3)
	if (FLASH_OPT_RDP != 0x5aa5)
		return;
#else
	if (((FLASH_OPTCR >> 8) & 0xff) != 0xaa)
		return;
#endif

	warn("Protect flash memory from externel accesses");

	while (FLASH_SR & (1 << BSY));

	FLASH_UNLOCK();
	FLASH_UNLOCK_OPTPG();

#if (SOC == stm32f1 || SOC == stm32f3)
	FLASH_CR |= 0x20; /* OPTER */
	FLASH_CR |= 1 << STRT;
#else
	FLASH_OPTCR &= ~(0xff << 8);
	FLASH_OPTCR |= 2; /* set start bit */
#endif

	while (FLASH_SR & (1 << BSY));

#if (SOC == stm32f1 || SOC == stm32f3)
	FLASH_CR &= ~0x20; /* OPTER */
#else
	FLASH_OPTCR &= ~2;
#endif

	FLASH_LOCK_OPTPG();
	FLASH_LOCK();

	warn("Rebooting...");

	__reboot();
}
