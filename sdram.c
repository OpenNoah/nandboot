#include "io.h"
#include "uart.h"
#include "config.h"
#include "sdram.h"

// Unit: 0.1ns
#define T_TO_CLK(t)	DIV_CEIL((uint64_t)(t) * SDRAM_CLK_RATE, 10000000000)

static struct sdram_t {
	_IO uint32_t DMCR;
	_IO uint16_t RTCSR;
	uint16_t _RESERVED0;
	_IO uint16_t RTCNT;
	uint16_t _RESERVED1;
	_IO uint16_t RTCOR;
	uint16_t _RESERVED2;
	_IO uint32_t DMAR;
	uint8_t _RESERVED3[0xa000 - 0x0094];
	_IO uint8_t SDMR[0];
} * const sdram = SDRAM_BASE;

void sdram_init(void)
{
	static const struct sdram_config_t *cfg = &config.sdram;

	// Disable bus release, stop auto refresh
	*BCR = 0;
	sdram->RTCSR = 0;

	const unsigned ref = T_TO_CLK(cfg->tref);
	const unsigned ras = T_TO_CLK(cfg->tras);
	const unsigned rcd = T_TO_CLK(cfg->trcd);
	const unsigned rp = T_TO_CLK(cfg->trp);
	const unsigned rc = T_TO_CLK(cfg->trc);

	static const unsigned rc_tbl[] = {
		0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};

	const uint32_t dmcr =
		(cfg->bw16 << 31) | ((cfg->ncol - 8) << 26) |
		(0 << 25) | (0 << 24) | (0 << 23) |
		((cfg->nrow - 11) << 20) | (cfg->bank4 << 19) |
		(0 << 18) | (1 << 17) | ((ras - 4) << 13) | ((rcd - 1) << 11) |
		((rp - 1) << 8) | ((cfg->dpl - 1) << 5) |
		(rc_tbl[rc] << 2) | ((cfg->cas - 1) << 0);

	const uint16_t mode =
		(cfg->burst << 9) | (cfg->cas << 4) |
		(cfg->btype << 3) | (cfg->blen << 0);

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
