#include <stdint.h>
#include "gpio.h"
#include "uart.h"
#include "wdt.h"
#include "pll.h"
#include "lcd.h"
#include "sdram.h"
#include "nand.h"
#include "keypad.h"
#include "helper.h"
#include "config.h"

#define BUFFER_SIZE	(8 * 1024 * 1024)

extern struct {
	uint8_t nandtype;
	uint8_t version;
	uint8_t variant_h, variant_l;
} _header;

static void *buf;

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

static void mem_dump_nand(const char *line, void *buf)
{
	uint32_t addr = 0;
	line = get_hex_u32(&line[2], &addr);
	if (*line++ == '\0')
		return;
	uint32_t len = 0;
	get_hex_u32(line, &len);
	if (len > BUFFER_SIZE)
		len = BUFFER_SIZE;
	nand_dump(buf, addr, len);
}

static void boot(void)
{
	// Load 2MB from next NAND block to SDRAM_LOAD_BASE
	nand_load(config.nand.block, SDRAM_LOAD_BASE, 2 * 1024 * 1024);
	uart_puts("Jumping to ");
	uart_puthex((uint32_t)SDRAM_LOAD_BASE, 8);
	uart_puts("...\r\n");
	// Jump to SDRAM_LOAD_BASE
	((void (*)(void))(SDRAM_LOAD_BASE))();
}

int main()
{
	pll_init();
	gpio_init();
	uart_init();
	uart_puts("\r\n*** nandboot start np");
	uart_puthex((_header.variant_h << 8) | _header.variant_l, 4);
	uart_puts(" ***\r\n");
	pll_switch();
	sdram_init();
	nand_init();
	lcd_init();

	uart_puts("Ready.\r\n");
	nand_print_id();
	boot();
	buf = alloc(BUFFER_SIZE);

	for (;;) {
		uart_puts("> ");
		char *line = uart_get_line();
		if (line[0] == 0)
			continue;

		switch (line[0]) {
		case 'n':
			mem_dump_nand(line, buf);
			break;
		case 'r':
			mem_read_line(line);
			break;
		case 'w':
			mem_write_line(line);
			break;
		case 'f':
			mem_fill_line(line);
			break;
		case 'b':
			boot();
			break;
		case '*':
			wdt_reset();
			break;
		}
	}

	wdt_reset();
	return 0;
}
