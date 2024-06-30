#include "config.h"
#include "nand.h"
#include "uart.h"
#include "gpio.h"
#include "io.h"

#define EMC_SMCR(n)         ((_IO uint32_t *)(SRAM_BASE + ((n) - 1) * 4))

#define NAND_BANK_BASE(n)   ((n) == 1 ? PA_TO_KSEG1(0x18000000) : \
                             (n) == 2 ? PA_TO_KSEG1(0x14000000) : \
                             (n) == 3 ? PA_TO_KSEG1(0x0c000000) : \
                                        PA_TO_KSEG1(0x08000000))
#if JZ4740
#define NAND_ADDR_PORT(n)       ((_IO uint8_t *)(NAND_BANK_BASE(n) + 0x00010000))   // A16
#define NAND_CMD_PORT(n)        ((_IO uint8_t *)(NAND_BANK_BASE(n) + 0x00008000))   // A15
#elif JZ4755
#define NAND_ADDR_PORT(n)       ((_IO uint8_t *)(NAND_BANK_BASE(n) + \
                                 (is_busshare ? 0x00010000 : 0x10)))       // A16
#define NAND_CMD_PORT(n)        ((_IO uint8_t *)(NAND_BANK_BASE(n) + \
                                 (is_busshare ? 0x00008000 : 0x08)))       // A15
#endif
#define NAND_DATA_PORT_8(n)     ((_IO uint8_t *)(NAND_BANK_BASE(n) + 0))
#define NAND_DATA_PORT_16(n)    ((_IO uint16_t *)(NAND_BANK_BASE(n) + 0))
#define NAND_DATA_PORT_32(n)    ((_IO uint32_t *)(NAND_BANK_BASE(n) + 0))

static struct nand_t {
    _IO uint32_t NFCSR;
#if JZ4740
    uint8_t _RESERVED0[0x100 - 0x54];
    _IO uint32_t NFECCR;
    _IO uint32_t NFECC;
    _IO uint32_t NFPAR[3];
    _IO uint32_t NFINTS;
    _IO uint32_t NFINTE;
    _IO uint32_t NFERR[4];
#endif
} * const nand = NAND_BASE;

#if JZ4755
static struct bch_t {
    _I  uint32_t BHCR;
    _O  uint32_t BHCSR;
    _O  uint32_t BHCCR;
    _IO uint32_t BHCNT;
    _IO uint8_t  BHDR;
        uint8_t  RESERVED_VAR[3];
    _IO uint32_t BHPAR[4];
    _I  uint32_t BHINT;
    _I  uint32_t BHERR[4];
    _IO uint32_t BHINTE;
    _O  uint32_t BHINTES;
    _O  uint32_t BHINTEC;
} * const bch = BCH_BASE;
#endif

static uint32_t is_busshare = 1;
static uint32_t page_size   = 0;
static uint32_t oob_size    = 0;
static uint32_t row_cycles  = 0;

static inline void nand_fce_assert(const unsigned bank)
{
    uint32_t mask = 0x03 << (2 * (bank - 1));
    nand->NFCSR = (nand->NFCSR & ~mask) | (0x03 << (2 * (bank - 1)));
}

static inline void nand_fce_restore(const unsigned bank)
{
    uint32_t mask = 0x03 << (2 * (bank - 1));
    nand->NFCSR = (nand->NFCSR & ~mask) | (0x01 << (2 * (bank - 1)));
}

void nand_init()
{
    const unsigned bank = 1;
    nand_fce_restore(bank);

    // Reset and wait
    gpio_nand_busy_catch();
    *NAND_CMD_PORT(bank) = 0xff;
    gpio_nand_busy_wait();

#if JZ4740
    row_cycles = 3;
    page_size = 4096;
    oob_size = 128;
#else
    // Read the first 12 bytes
    uint8_t header[12];
    gpio_nand_busy_catch();
    *NAND_CMD_PORT(bank) = 0x00;
    *NAND_ADDR_PORT(bank) = 0;
    *NAND_ADDR_PORT(bank) = 0;
    *NAND_ADDR_PORT(bank) = 0;
    *NAND_ADDR_PORT(bank) = 0;
    *NAND_ADDR_PORT(bank) = 0;
    *NAND_CMD_PORT(bank) = 0x30;
    gpio_nand_busy_wait();
    for (int i = 0; i < 12; i++)
        header[i] = *NAND_DATA_PORT_8(bank);

    // Parse NAND boot info
    uint32_t bus_width = header[0] == 0 ? 16 : 8;
    row_cycles = header[8] == 0 ? 2 : 3;
    page_size = header[9] == 0 ? 512 : header[10] == 0 ? 4096 : 2048;
    oob_size = 16 * page_size / 512;
#endif
}

void nand_print_id()
{
    const unsigned bank0 = 1;
    *NAND_CMD_PORT(bank0) = 0x90;
    *NAND_ADDR_PORT(bank0) = 0x00;

    uart_puts("NAND bank0 id: ");
    for (int i = 0; i < 6; i++) {
        uint8_t id = *NAND_DATA_PORT_8(bank0);
        uart_puthex(id, 2);
    }
    uart_puts("\r\n");
}

#define CACHE_SIZE          16*1024
#define CACHE_LINE_SIZE     32
#define KSEG0               0x80000000

#define SYNC_WB() __asm__ __volatile__ ("sync")

#define Index_Writeback_Inv_D   0x01

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	noreorder		\n"			\
	"	.set	mips32\n\t		\n"			\
	"	cache	%0, %1			\n"			\
	"	.set	mips0			\n"			\
	"	.set	reorder"					\
	:										\
	: "i" (op), "m" (*(unsigned char *)(addr)))

void __dcache_writeback_all()
{
	uint32_t i;
	for (i=KSEG0;i<KSEG0+CACHE_SIZE;i+=CACHE_LINE_SIZE)
		cache_op(Index_Writeback_Inv_D, i);
	SYNC_WB();
}

void nand_boot()
{
    // Read 8k from NAND to 0x80000000
    nand_read_pages((void *)0x80000000, 0, 8192 / page_size, 0);
    // Logging
    uart_puts(__func__);
    uart_puts(": now booting\r\n");
    // Flush dcache
    __dcache_writeback_all();
    // Jump to offset 4
    ((void(*)())0x80000004)();
}

void nand_read_pages(void *dst, uint32_t start, uint32_t count, int oob)
{
    uart_puts(__func__);
    uart_puts(": 0x");
    uart_puthex(start, 8);
    uart_puts(" + 0x");
    uart_puthex(count, 8);
    uart_puts("\r\n");

    const unsigned bank = 1;
    uint32_t *buf32 = dst;
    while (count--) {
        gpio_nand_busy_catch();
        *NAND_CMD_PORT(bank) = 0x00;
        *NAND_ADDR_PORT(bank) = (0 >> 0) & 0xff;
        *NAND_ADDR_PORT(bank) = (0 >> 8) & 0xff;
        *NAND_ADDR_PORT(bank) = (start >>  0) & 0xff;
        *NAND_ADDR_PORT(bank) = (start >>  8) & 0xff;
        *NAND_ADDR_PORT(bank) = (start >> 16) & 0xff;
        *NAND_CMD_PORT(bank) = 0x30;
        gpio_nand_busy_wait();

        uint32_t page_data[page_size / 4];
        for (unsigned i = 0; i < page_size / 4; i++)
            page_data[i] = *NAND_DATA_PORT_32(bank);

        uint32_t oob_data[(oob_size + 3) / 4];
        for (unsigned i = 0; i < oob_size / 4; i++)
            oob_data[i] = *NAND_DATA_PORT_32(bank);

        for (unsigned i = 0; i < page_size / 4; i++)
            *buf32++ = page_data[i];
        if (oob)
            for (unsigned i = 0; i < oob_size / 4; i++)
                *buf32++ = oob_data[i];
        start++;
    }
}
