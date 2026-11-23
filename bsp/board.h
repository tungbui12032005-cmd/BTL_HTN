#ifndef BOARD_H
#define BOARD_H
#include <stdbool.h>
#include <stdint.h>

// 1: module active-HIGH; 0: module active-LOW.
#ifndef BUZZER_ACTIVE_HIGH
#define BUZZER_ACTIVE_HIGH 1
#endif

#if BUZZER_ACTIVE_HIGH != 0 && BUZZER_ACTIVE_HIGH != 1
#error "BUZZER_ACTIVE_HIGH must be 0 or 1"
#endif

void board_init(void);
unsigned board_mq_adc(void);
bool board_flame(void);
bool board_button(unsigned index);
void board_outputs(bool alarm, bool fan);
void board_led(bool on);
int board_uart_get(void);
void board_uart_put(char ch);
void board_wait_us(uint16_t us);
#endif
