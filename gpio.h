#pragma once

void gpio_init(void);
void gpio_nand_busy_catch(void);
void gpio_nand_busy_wait(void);
void gpio_lcd_enable(int en);
