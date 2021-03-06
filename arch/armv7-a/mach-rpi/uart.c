#include <uart.h>
#include <stdlib.h>
#include <foundation.h>
#include <gpio.h>
#include <error.h>
#include "irq.h"

#define SYSTEM_CLOCK		250000000 /* 250MHz */

int __uart_open(unsigned int channel, unsigned int baudrate)
{
	if (channel)
		return ERANGE;

	/* Set GPIO first */
	SET_GPIO_FS(14, GPIO_ALT5);
	SET_GPIO_FS(15, GPIO_ALT5);
	gpio_pull(14, 0);
	gpio_pull(15, 0);

	struct aux *aux = (struct aux *)AUX_BASE;
	struct mini_uart *uart = (struct mini_uart *)UART_BASE;

	aux->enable = 1; /* enable mini-uart */
	uart->cntl  = 0;
	uart->lcr   = 3; /* 8-bit data */
	uart->iir   = 0xc6;
	uart->ier   = 5; /* enable rx interrupt */
	uart->baudrate = (SYSTEM_CLOCK / (baudrate * 8)) - 1;

	uart->cntl = 3; /* enable rx and tx */

	struct interrupt_controller *intcntl
		= (struct interrupt_controller *)INTCNTL_BASE;
	intcntl->irq1_enable = 1 << IRQ_AUX;

	return IRQ_AUX; /* aux int */
}

void __uart_close(unsigned int channel)
{
}

int __uart_putc(unsigned int channel, int c)
{
	struct mini_uart *uart = (struct mini_uart *)UART_BASE;

	if (!(uart->lsr & 0x20)) return 0;

	uart->dr = c;

	return 1;
}

int __uart_check_rx(unsigned int channel)
{
	struct mini_uart *uart = (struct mini_uart *)UART_BASE;

	if (uart->iir & 1)
		goto out;

	if (uart->iir & 4)
		return 1;

out:
	return 0;
}

int __uart_check_tx(unsigned int channel)
{
	return 0;
}

int __uart_getc(unsigned int channel)
{
	struct mini_uart *uart = (struct mini_uart *)UART_BASE;

	return uart->dr;
}

void __uart_tx_irq_reset(unsigned int channel)
{
}

void __uart_tx_irq_raise(unsigned int channel)
{
}

void __uart_flush(unsigned int channel)
{
}

unsigned int __uart_get_baudrate(unsigned int channel)
{
	return 0;
}

int __uart_set_baudrate(unsigned int channel, unsigned int baudrate)
{
	return 0;
}
