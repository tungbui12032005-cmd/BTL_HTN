#include "task_oled.h"
#include "app.h"
#include "ssd1306.h"
#include <stdio.h>

void task_oled(void *argument)
{
    (void)argument;
    bool ready = false;

    for (;;) {
        Status status;
        xQueueReceive(oled_queue, &status, portMAX_DELAY);
        if (!ready) ready = ssd1306_init();

        if (ready) {
            char text[22];
            ssd1306_clear();
            snprintf(text, sizeof text, "MQ-2: %u", status.sensors.adc);
            ssd1306_text(0, 0, text);
            snprintf(text, sizeof text, "NGUONG: %u", status.threshold);
            ssd1306_text(1, 0, text);
            snprintf(text, sizeof text, "TRE: %u S", status.delay_seconds);
            ssd1306_text(2, 0, text);
            ssd1306_text(3, 0, status.sensors.flame ? "LUA: CO" : "LUA: KHONG");
            ssd1306_text(4, 0, status.gas ? "MQ-2: CANH BAO" :
                                 status.pending ? "MQ-2: DANG DEM TRE" :
                                                  "MQ-2: BINH THUONG");
            ssd1306_text(5, 0, status.alarm_on ? "LED/COI: BAT" : "LED/COI: TAT");
            ssd1306_text(6, 0, status.fan_on ? "QUAT: BAT" : "QUAT: TAT");
            ready = ssd1306_show();
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
