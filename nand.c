#include "nand.h"
#include "uart.h"
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
	nand->NFCSR = 0x01 << (2 * (bank - 1));
}

static inline void nand_fce_restore(void)
{
	nand->NFCSR = 0x03 << (2 * (bank - 1));
}

void nand_init(void)
{
	//*EMC_SMCR(bank) = ;
	//nand_fce_restore();
}

void nand_print_id(void)
{
	nand_fce_assert();
	*NAND_CMD_PORT(bank) = 0x90;
	*NAND_ADDR_PORT(bank) = 0x00;
	uart_puts("NAND id: ");
	for (int i = 0; i < 5; i++) {
		uint8_t id = *NAND_DATA_PORT_8(bank);
		uart_puthex(id, 2);
	}
	uart_puts("\r\n");
	nand_fce_restore();
}
