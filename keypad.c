#include "gpio.h"
#include "keypad.h"

#ifdef np1380
// LT7		PD2
// LT6		PD3
// RT7		PD7
//#define ROW_MASK	0x0000008C

// LT3/RT4	PD1
// LT4/RT5	PD17
// LT5/RT6	PD15
// RT3		PD0
//#define COL_MASK	0x00028003

#define COL_MASK	0x0000008C
#define ROW_MASK	0x00028003
#endif

void keypad_init(void)
{
#if 0
#ifdef np1380
	gpio_t *gpiod = GPIOD_BASE;
	gpiod->FUN.C = (ROW_MASK | COL_MASK);
	gpiod->SEL.C = (ROW_MASK | COL_MASK);
	gpiod->DIR.C = (ROW_MASK | COL_MASK);
	gpiod->DIR.S = ROW_MASK;
	gpiod->DAT.C = ROW_MASK;
	gpiod->PE.S  = COL_MASK;

	uart_puts(uart, "Keypad init!\r\n");
	uint32_t pv = COL_MASK;
	for (;;) {
		uint32_t v = gpiod->PIN.D & (ROW_MASK | COL_MASK);
		if (v != pv) {
			pv = v;
			uart_puts(uart, "Keypad: ");
			uart_puthex(uart, v, 8);
			uart_puts(uart, "\r\n");
		}
		if (v == 0)
			break;
	}
#endif
#endif
}
