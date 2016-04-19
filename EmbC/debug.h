#ifndef DEBUG_H

#define DEBUG_H

#include "common.h"

void debug_init(void);
void debug_shut(void);
void print_nl(void);
void print_str(char *str);
void print_str_nl(char *str);
void print_num(uint32_t n);
void print_hex(uint32_t n);
char scan_char(void);
void scan_line(char *line, int max_len);

#endif
