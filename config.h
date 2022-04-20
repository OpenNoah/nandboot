#pragma once

#if VARIANT == 0x1500
#define SDRAM_HY57V561620FTP_H
#define NAND_HY27UU08AG5M
#elif VARIANT == 0x1501
#define SDRAM_HY57V561620FTP_H
#define NAND_K9GAG08U0M
#else
#error Unknown board variant
#endif

#define MHZ(n)		((n) * 1000000)

#define SYS_CLK_RATE	MHZ(336)
#define SDRAM_CLK_RATE	MHZ(112)
#define EXT_CLK_RATE	MHZ(12)
#define BAUDRATE	115200

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

struct nand_config_t {
	unsigned bank;	// Chip select #
	unsigned block;	// Block size
	unsigned page;	// Page size
	unsigned oob;	// OOB bytes per page
};

struct config_t {
	struct sdram_config_t sdram;
	struct nand_config_t nand;
};

static const struct config_t config = {
	.sdram = {
#ifdef SDRAM_HY57V561620FTP_H
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
#endif
	},
	.nand = {
#ifdef NAND_K9GAG08U0M
		.bank = 1,
		.block = 512 * 1024,
		.page = 4 * 1024,
		.oob = 16 * 2 * 4,
#elif defined(NAND_HY27UU08AG5M)
		.bank = 1,
		.block = 256 * 1024,
		.page = 2 * 1024,
		.oob = 64,
#endif
	}
};

static const void *sdram_base = (void *)PA_TO_KSEG0(0);
static const uint32_t sdram_size = (4 / (1 + config.sdram.bw16)) *
	(1 << (config.sdram.nrow + config.sdram.ncol)) *
	(2 * (1 + config.sdram.bank4));
