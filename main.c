#include <stdint.h>
#include "gpio.h"
#include "uart.h"
#include "wdt.h"
#include "pll.h"
#include "sdram.h"
#include "keypad.h"
#include "helper.h"

static void mem_read_line(const char *line)
{
	uint32_t addr = 0;
	get_hex_u32(&line[2], &addr);
	if (addr == 0)
		return;
	uint32_t v = *(uint32_t *)addr;
	uart_puts("0x");
	uart_puthex(addr, 8);
	uart_puts(" = 0x");
	uart_puthex(v, 8);
	uart_puts("\r\n");
}

static void mem_write_line(const char *line)
{
	uint32_t addr = 0;
	line = get_hex_u32(&line[2], &addr);
	if (addr == 0 || *line++ == '\0')
		return;
	uint32_t v = 0;
	get_hex_u32(line, &v);
	*(uint32_t *)addr = v;

	v = *(uint32_t *)addr;
	uart_puts("0x");
	uart_puthex(addr, 8);
	uart_puts(" = 0x");
	uart_puthex(v, 8);
	uart_puts("\r\n");
}

static void mem_fill_line(const char *line)
{
	uint32_t addr = 0;
	line = get_hex_u32(&line[2], &addr);
	if (addr == 0 || *line++ == '\0')
		return;
	uint32_t size = 0;
	line = get_hex_u32(line, &size);
	if (size == 0 || *line++ == '\0')
		return;
	uint32_t v = 0;
	get_hex_u32(line, &v);
	for (uint32_t i = 0; i < size / 4; i++)
		*((uint32_t *)addr + i) = v;

	v = *(uint32_t *)addr;
	uart_puts("0x");
	uart_puthex(addr, 8);
	uart_puts(" = 0x");
	uart_puthex(v, 8);
	uart_puts("\r\n");
}

extern unsigned short quot1[3];

int main()
{
	pll_init();
	gpio_init();
	uart_init();
	uart_puts("\r\n*** nandboot start ***\r\n");
	pll_switch();
	sdram_init();

	uart_puts("Ready.\r\n");

	for (;;) {
		uart_puts("> ");
		char *line = uart_get_line();
		if (line[0] == 0)
			continue;

		switch (line[0]) {
		case 'r':
			mem_read_line(line);
			break;
		case 'w':
			mem_write_line(line);
			break;
		case 'f':
			mem_fill_line(line);
			break;
		case '*':
			wdt_reset();
			break;
		}
	}

	wdt_reset();
	return 0;
}
