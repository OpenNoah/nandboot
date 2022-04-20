#pragma once

#include <stdint.h>

void nand_init(void);
void nand_print_id(void);
void nand_dump(void *buf, uint32_t addr, uint32_t len);
