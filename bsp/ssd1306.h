#ifndef SSD1306_H
#define SSD1306_H
#include <stdbool.h>
#include <stdint.h>

bool ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_text(unsigned row, unsigned column, const char *text);
bool ssd1306_show(void);
#endif
