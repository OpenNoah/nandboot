#include "io.h"
#include "gpio.h"
#include "uart.h"
#include "config.h"
#include "sdram.h"

// Unit: 0.1ns
#define T_TO_CLK(t)	DIV_CEIL((uint64_t)(t) * SDRAM_CLK_RATE, 10000000000)

_IO uint32_t *BCR = MEMC_BASE;

static struct sdram_t {
	_IO uint32_t DMCR;
	_IO uint16_t RTCSR;
	uint16_t _RESERVED0;
	_IO uint16_t RTCNT;
	uint16_t _RESERVED1;
	_IO uint16_t RTCOR;
	uint16_t _RESERVED2;
	_IO uint32_t DMAR;
	uint32_t _RESERVED3[(0xa000 - 0x0094) / 4];
	_IO uint8_t SDMR[0];
} * const sdram = SDRAM_BASE;

struct sdram_config_t {
	// Parameters
	unsigned bw16;	// 0: 32-bit, 1: 16-bit
	unsigned bank4;	// 0: 2 banks per chip, 1: 4 banks per chip
	unsigned nrow;
	unsigned ncol;
	unsigned cas;
	unsigned burst;	// 0: burst read & write, 1: burst read only
	unsigned btype;	// 0: sequential, 1: interleave
	unsigned blen;	// burst length

	// Timing, unit: 0.1ns
	unsigned tref;	// Refresh cycle time
	unsigned trc;	// RAS cycle time
	unsigned trcd;	// RAS to CAS delay
	unsigned tras;	// RAS active time
	unsigned trp;	// RAS precharge time

	// Timing, unit: clocks
	unsigned dpl;	// Data-in to precharge
};

static const struct sdram_config_t sdram_config_HY57V561620FTP_6 = {
	.bw16 = 0,
	.bank4 = 1,
	.nrow = 13,
	.ncol = 9,
	.cas = 3,
	.burst = 0,
	.btype = 0,
	.blen = 0b010,	// burst length 4

	.tref = 64 * 1000 * 1000 * 10 / 8192,
	.trc = 630,
	.trcd = 200,
	.tras = 420,
	.trp = 200,

	.dpl = 2,
};

void sdram_init(void)
{
	static const struct sdram_config_t *config =
		&sdram_config_HY57V561620FTP_6;

	// Disable bus release, stop auto refresh
	*BCR = 0;
	sdram->RTCSR = 0;

	const unsigned ref = T_TO_CLK(config->tref);
	const unsigned ras = T_TO_CLK(config->tras);
	const unsigned rcd = T_TO_CLK(config->trcd);
	const unsigned rp = T_TO_CLK(config->trp);
	const unsigned rc = T_TO_CLK(config->trc);

	static const unsigned rc_tbl[] = {
		0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};

	const uint32_t dmcr =
		(config->bw16 << 31) | ((config->ncol - 8) << 26) |
		(0 << 25) | (0 << 24) | (0 << 23) |
		((config->nrow - 11) << 20) | (config->bank4 << 19) |
		(0 << 18) | (1 << 17) | ((ras - 4) << 13) | ((rcd - 1) << 11) |
		((rp - 1) << 8) | ((config->dpl - 1) << 5) |
		(rc_tbl[rc] << 2) | ((config->cas - 1) << 0);

	const uint16_t mode =
		(config->burst << 9) | (config->cas << 4) |
		(config->btype << 3) | (config->blen << 0);

	// Power up, wait for 200us with clock started
	sdram->DMCR = dmcr;
	for (int i = 0; i < (uint64_t)200 * SYS_CLK_RATE / 1000000; i++)
		asm("nop");

	// Precharge all banks
	sdram->SDMR[mode] = 0;

	// Enable auto-refresh, wait for 10 refresh cycles
	sdram->RTCOR = ref / 4 - 1;
	sdram->RTCNT = 0;
	sdram->RTCSR = (0 << 7) | (0b001 << 0);	// div by 4
	sdram->DMCR = dmcr | (1 << 24) | (1 << 23);
	for (int i = 0; i < 10; i++) {
		while (!(sdram->RTCSR & (1 << 7)));
		sdram->RTCSR = (0 << 7) | (0b001 << 0);
	}

	// Mode register set
	sdram->SDMR[mode] = 0;

	// Map to default SDRAM bank
	sdram->DMAR = 0x000020f8;
	*BCR = (1 << 1);
}

void sdram_print(void)
{
	uint32_t csr = sdram->RTCSR;
	uint32_t cor = sdram->RTCOR;
	uint32_t cnt = sdram->RTCNT;
	uart_puthex((uint32_t)&sdram->SDMR[0], 8);
	uart_puts(", ");
	uart_puthex(csr, 2);
	uart_puts(", ");
	uart_puthex(cor, 2);
	uart_puts(", ");
	uart_puthex(cnt, 2);
	uart_puts("\r\n");
}
