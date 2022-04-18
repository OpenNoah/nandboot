#include "io.h"
#include "config.h"
#include "pll.h"

#define DIV_CEIL(a, b)	(((a) + (b) - 1) / (b))

#pragma packed(push, 1)
struct cgu_t {
	_IO uint32_t CPCCR;
	uint32_t _RESERVED0[3];
	_IO uint32_t CPPCR;
	uint32_t _RESERVED1[19];
	_IO uint32_t I2SCDR;
	_IO uint32_t LPCDR;
	_IO uint32_t MSCCDR;
	_IO uint32_t UHCCDR;
	uint32_t _RESERVED2[1];
	_IO uint32_t SSICDR;
};

static struct cgu_t * const cgu = CGU_BASE;

void pll_init(void)
{
	// Configure PLL
	static const unsigned long n = 2;
	static const unsigned long m = DIV_CEIL(SYS_CLK_RATE, EXT_CLK_RATE / n);
	// M, N, OD, bypass, enabled, stablise time = 3
	cgu->CPPCR = ((m - 2) << 23) | ((n - 2) << 18) | (0 << 16) |
		     (1 << 9) | (1 << 8) | (3 << 0);
	// System clock dividers
	static const unsigned long cdiv = 1;
	static const unsigned long mdiv = DIV_CEIL(SYS_CLK_RATE, SDRAM_CLK_RATE);
	static const unsigned long hdiv = mdiv;
	static const unsigned long pdiv = mdiv;
	static const unsigned long ldiv = 32;
	cgu->CPCCR = (1 << 30) | (1 << 22) | (1 << 21) | ((ldiv - 1) << 16) |
		     ((mdiv - 1) << 12) | ((pdiv - 1) << 8) |
		     ((hdiv - 1) << 4) | ((cdiv - 1) << 0);
	// UHC clock at 48MHz
	cgu->UHCCDR = (SYS_CLK_RATE / 48) - 1;
}

void pll_switch(void)
{
	// Wait for PLL stable
	while (!(cgu->CPPCR & (1 << 10)));
	// Now, switch system clock to PLL
	cgu->CPPCR &= ~(1 << 9);
}
