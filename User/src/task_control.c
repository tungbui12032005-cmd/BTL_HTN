#include "task_control.h"
#include "app.h"
#include "board.h"

static void apply_command(Status *status, Command command)
{
    switch (command.type) {
    case TOGGLE_ALARM: status->alarm_manual = !status->alarm_manual; break;
    case TOGGLE_FAN: status->fan_manual = !status->fan_manual; break;
    case SET_ALARM: status->alarm_manual = command.value != 0; break;
    case SET_FAN: status->fan_manual = command.value != 0; break;
    case SET_THRESHOLD:
        if (status->threshold != command.value) {
            status->threshold = command.value;
            status->pending = false;
            status->gas = false;
        }
        break;
    case SET_DELAY: status->delay_seconds = command.value; break;
    }
}

static void update_gas(Status *status, TickType_t now, TickType_t *started)
{
    if (status->gas) {
        unsigned clear = status->threshold > 80U ? status->threshold - 80U : 0U;
        if (status->sensors.adc <= clear) status->gas = false;
    } else if (status->sensors.adc < status->threshold) {
        status->pending = false;
    } else {
        if (!status->pending) {
            *started = now;
            status->pending = true;
        }
        if (now - *started >= pdMS_TO_TICKS(status->delay_seconds * 1000U)) {
            status->pending = false;
            status->gas = true;
        }
    }
}

void task_control(void *argument)
{
    (void)argument;
    Status status = { .threshold = 1800, .delay_seconds = 3 };
    xQueueReceive(sensor_queue, &status.sensors, portMAX_DELAY);
    TickType_t wake = xTaskGetTickCount();
    TickType_t gas_started = wake;

    for (;;) {
        xQueueReceive(sensor_queue, &status.sensors, 0);
        Command command;
        for (unsigned i = 0; i < COMMAND_CAPACITY; ++i) {
            if (xQueueReceive(command_queue, &command, 0) != pdPASS) break;
            apply_command(&status, command);
        }
        update_gas(&status, xTaskGetTickCount(), &gas_started);
        status.alarm_on = status.sensors.flame || status.gas || status.alarm_manual;
        status.fan_on = !status.sensors.flame && (status.gas || status.fan_manual);
        board_outputs(status.alarm_on, status.fan_on);
        xQueueOverwrite(oled_queue, &status);
        xQueueOverwrite(uart_queue, &status);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}
