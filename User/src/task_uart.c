#include "task_uart.h"
#include "app.h"
#include "board.h"
#include <stdio.h>

static void parse_line(const char *line, unsigned length)
{
    if (length < 4U || line[2] != '=') return;

    unsigned value = 0;
    for (unsigned i = 3; i < length; ++i) {
        if (line[i] < '0' || line[i] > '9') return;
        value = 10U * value + (unsigned)(line[i] - '0');
        if (value > 4095U) return;
    }

    Command command = { .value = value };
    if (line[0] == 'T' && line[1] == 'H' && value >= 1U) {
        command.type = SET_THRESHOLD;
    } else if (line[0] == 'D' && line[1] == 'E' && value <= 60U) {
        command.type = SET_DELAY;
    } else if (line[0] == 'A' && line[1] == 'L' && value <= 1U) {
        command.type = SET_ALARM;
    } else if (line[0] == 'F' && line[1] == 'A' && value <= 1U) {
        command.type = SET_FAN;
    } else {
        return;
    }
    xQueueSend(command_queue, &command, portMAX_DELAY);
}

static void send_status(const Status *status)
{
    char report[64];
    snprintf(report, sizeof report, "S,%u,%d,%d,%d,%d,%d,%u,%u\n",
             status->sensors.adc, status->sensors.flame, status->alarm_on,
             status->fan_on, status->alarm_manual, status->fan_manual,
             status->threshold, status->delay_seconds);
    for (const char *ch = report; *ch; ++ch) board_uart_put(*ch);
}

void task_uart(void *argument)
{
    (void)argument;
    char line[24];
    unsigned length = 0;
    bool discard = false;
    TickType_t last_report = xTaskGetTickCount();

    for (;;) {
        for (unsigned count = 0; count < 96U; ++count) {
            int ch = board_uart_get();
            if (ch == -1) break;
            if (ch == -2) {
                discard = true;
                length = 0;
                continue;
            }
            if (ch == '\n') {
                if (!discard) parse_line(line, length);
                length = 0;
                discard = false;
            } else if (ch != '\r') {
                if (length < sizeof line) line[length++] = (char)ch;
                else discard = true;
            }
        }

        TickType_t now = xTaskGetTickCount();
        if (now - last_report >= pdMS_TO_TICKS(1000)) {
            Status status;
            if (xQueueReceive(uart_queue, &status, 0) == pdPASS) {
                send_status(&status);
                last_report = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
