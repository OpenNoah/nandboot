#pragma once

#include <stdint.h>
#include "helper.h"

#define _I	volatile
#define _O	volatile
#define _IO	volatile

#define CGU_BASE	((void *)PA_TO_KSEG1(0x10000000))
#define LCD_BASE	((void *)PA_TO_KSEG1(0x13050000))
#define MEMC_BASE	((void *)PA_TO_KSEG1(0x13010000))
#define SDRAM_BASE	((void *)PA_TO_KSEG1(0x13010080))

#define UART0_BASE	((void *)PA_TO_KSEG1(0x10030000))
#define UART1_BASE	((void *)PA_TO_KSEG1(0x10031000))
#define UART2_BASE	((void *)PA_TO_KSEG1(0x10032000))
#define UART3_BASE	((void *)PA_TO_KSEG1(0x10033000))
