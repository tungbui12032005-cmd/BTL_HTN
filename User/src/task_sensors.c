#include "task_sensors.h"
#include "app.h"
#include "board.h"

void task_sensors(void *argument)
{
    (void)argument;
    TickType_t wake = xTaskGetTickCount();
    Sensors sensors = { .adc = board_mq_adc() };
    unsigned cycle = 0;

    for (;;) {
        if (++cycle == 5U) {
            cycle = 0;
            sensors.adc = (3U * sensors.adc + board_mq_adc() + 2U) / 4U;
        }
        sensors.flame = board_flame();
        xQueueOverwrite(sensor_queue, &sensors);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}
