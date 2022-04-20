#include "nand.h"
#include "uart.h"
#include "gpio.h"
#include "io.h"
#include "config.h"

#define EMC_SMCR(n)		((_IO uint32_t *)(SRAM_BASE + ((n) - 1) * 4))

#define NAND_BANK_BASE(n)	((n) == 1 ? PA_TO_KSEG1(0x18000000) : \
				 (n) == 2 ? PA_TO_KSEG1(0x14000000) : \
				 (n) == 3 ? PA_TO_KSEG1(0x0c000000) : \
				            PA_TO_KSEG1(0x08000000))
#define NAND_ADDR_PORT(n)	((_IO uint8_t *)(NAND_BANK_BASE(n) + 0x00010000))	// A16
#define NAND_CMD_PORT(n)	((_IO uint8_t *)(NAND_BANK_BASE(n) + 0x00008000))	// A15
#define NAND_DATA_PORT_8(n)	((_IO uint8_t *)(NAND_BANK_BASE(n) + 0))
#define NAND_DATA_PORT_16(n)	((_IO uint16_t *)(NAND_BANK_BASE(n) + 0))
#define NAND_DATA_PORT_32(n)	((_IO uint32_t *)(NAND_BANK_BASE(n) + 0))

static struct nand_t {
	_IO uint32_t NFCSR;
	uint8_t _RESERVED0[0x100 - 0x54];
	_IO uint32_t NFECCR;
	_IO uint32_t NFECC;
	_IO uint32_t NFPAR[3];
	_IO uint32_t NFINTS;
	_IO uint32_t NFINTE;
	_IO uint32_t NFERR[4];
} * const nand = NAND_BASE;

static const unsigned bank = config.nand.bank;

static inline void nand_fce_assert(void)
{
	nand->NFCSR = 0x03 << (2 * (bank - 1));
}

static inline void nand_fce_restore(void)
{
	nand->NFCSR = 0x01 << (2 * (bank - 1));
}

void nand_init(void)
{
	//*EMC_SMCR(bank) = ;
	nand_fce_restore();
	*NAND_CMD_PORT(bank) = 0xff;
}

void nand_print_id(void)
{
	//nand_fce_assert();
	*NAND_CMD_PORT(bank) = 0x90;
	*NAND_ADDR_PORT(bank) = 0x00;
	uart_puts("NAND id: ");
	for (int i = 0; i < 5; i++) {
		uint8_t id = *NAND_DATA_PORT_8(bank);
		uart_puthex(id, 2);
	}
	uart_puts("\r\n");
	//nand_fce_restore();
}

void nand_dump(void *buf, uint32_t addr, uint32_t len)
{
	const unsigned page = config.nand.page + config.nand.oob;
	addr = addr & ~(16 - 1);
	addr = addr / config.nand.page * page;
	len = len & ~(4 - 1);
	len = len / config.nand.page * page;
	uint32_t *buf32 = buf;
	uint32_t a = addr, v = len;
	while (v) {
		uint32_t col = a % page;
		uint32_t row = a / page;
		//nand_fce_assert();
		gpio_nand_busy_catch();
		*NAND_CMD_PORT(bank) = 0x00;
		*NAND_ADDR_PORT(bank) = col & 0xff;
		*NAND_ADDR_PORT(bank) = (col >> 8) & 0xff;
		*NAND_ADDR_PORT(bank) = row & 0xff;
		*NAND_ADDR_PORT(bank) = (row >> 8) & 0xff;
		*NAND_ADDR_PORT(bank) = (row >> 16) & 0xff;
		*NAND_CMD_PORT(bank) = 0x30;
		uint32_t s = ((a + page) & ~(page - 1)) - a;
		s = s >= v ? v : s;
		a += s;
		v -= s;
		gpio_nand_busy_wait();
		for (unsigned i = 0; i < s / 4; i++)
			buf32[i] = *NAND_DATA_PORT_32(bank);
		//nand_fce_restore();
		buf32 += s / 4;
	}

	uint8_t *buf8 = buf;
	uint8_t prev[16] = {~buf8[0]};
	uint8_t pnt = 0;
	a = addr;
	for (v = 0; v < len; v += 16) {
		unsigned i;
		for (i = 0; i < 16; i++)
			if (prev[i] != buf8[v + i])
				break;
		if (i == 16) {
			if (!pnt) {
				pnt = 1;
				uart_puts("*\r\n");
			}
			a += 16;
			continue;
		}
		for (i = 0; i < 16; i++)
			prev[i] = buf8[v + i];
		pnt = 0;

		uint32_t col = a % page;
		uint32_t row = a / page;
		uint32_t pa = row * config.nand.page;
		if (col < config.nand.page) {
			uart_puthex(pa + col, 8);
			uart_puts("   :");
		} else {
			uart_puthex(pa, 8);
			uart_putc('+');
			uart_puthex(col - config.nand.page, 2);
			uart_putc(':');
		}
		a += 16;
		for (i = 0; i < 16; i++) {
			uart_putc(' ');
			if (i == 8)
				uart_putc(' ');
			uart_puthex(buf8[v + i], 2);
		}
		uart_puts("\r\n");
	}
}
