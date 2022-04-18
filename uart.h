#pragma once

#include "io.h"

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puthex(uint32_t v, int w);
void uart_putdec(uint32_t v);
char uart_getc(void);
char *uart_get_line(void);
