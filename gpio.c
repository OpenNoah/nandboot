#include "gpio.h"

#include <stdint.h>
#include "helper.h"
#include "io.h"

#define GPIOA_BASE	((gpio_t *)PA_TO_KSEG1(0x10010000))
#define GPIOB_BASE	((gpio_t *)PA_TO_KSEG1(0x10010100))
#define GPIOC_BASE	((gpio_t *)PA_TO_KSEG1(0x10010200))
#define GPIOD_BASE	((gpio_t *)PA_TO_KSEG1(0x10010300))

typedef struct gpio_t {
	struct gpio_port_t {
		_I uint32_t D;
		_O uint32_t S;
		_O uint32_t C;
		   uint32_t _RESV;
	} PIN, DAT, IM, PE, FUN, SEL, DIR, TRG, FLG;
} gpio_t;

static gpio_t * const gpa = GPIOA_BASE;
static gpio_t * const gpb = GPIOB_BASE;
static gpio_t * const gpc = GPIOC_BASE;
static gpio_t * const gpd = GPIOD_BASE;

// UART 0: PD25 AF1, PD26 AF1
#define GPIO_PINS_UART0	((1 << 25) | (1 << 26))

// MEMC: PA0-31: AF0
#define GPIOA_PINS_MEMC	(0xffffffff)
// MEMC: PB0-16, PB19-PB26, PB31: AF0
#define GPIOB_PINS_MEMC	(0x87f9ffff)
// MEMC: PC24-PC26, PC28-PC29: AF0
#define GPIOC_PINS_MEMC	(0x37000000)

void gpio_init(void)
{
	gpa->FUN.S = GPIOA_PINS_MEMC;
	gpa->SEL.C = GPIOA_PINS_MEMC;
	gpa->PE.S  = GPIOA_PINS_MEMC;

	gpb->FUN.S = GPIOB_PINS_MEMC;
	gpb->SEL.C = GPIOB_PINS_MEMC;
	gpb->PE.S  = GPIOB_PINS_MEMC;

	gpc->FUN.S = GPIOC_PINS_MEMC;
	gpc->SEL.C = GPIOC_PINS_MEMC;
	gpc->PE.S  = GPIOC_PINS_MEMC;

	gpd->FUN.S = GPIO_PINS_UART0;
	gpd->SEL.S = GPIO_PINS_UART0;
	gpd->PE.C  = GPIO_PINS_UART0;
}
