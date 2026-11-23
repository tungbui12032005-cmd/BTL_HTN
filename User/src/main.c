#include "app.h"
#include "board.h"
#include "task_sensors.h"
#include "task_buttons.h"
#include "task_control.h"
#include "task_oled.h"
#include "task_uart.h"
#include "stm32f1xx.h"

QueueHandle_t sensor_queue, command_queue, oled_queue, uart_queue;

void app_fault(void)
{
    __disable_irq();
    board_outputs(false, false);
    for (;;) {
        board_led(true);
        for (unsigned i = 0; i < 10U; ++i) board_wait_us(20000);
        board_led(false);
        for (unsigned i = 0; i < 10U; ++i) board_wait_us(20000);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    app_fault();
}

int main(void)
{
    board_init();
    sensor_queue = xQueueCreate(1, sizeof(Sensors));
    command_queue = xQueueCreate(COMMAND_CAPACITY, sizeof(Command));
    oled_queue = xQueueCreate(1, sizeof(Status));
    uart_queue = xQueueCreate(1, sizeof(Status));

    if (!sensor_queue || !command_queue || !oled_queue || !uart_queue) app_fault();

    // Stack sizes in 32-bit words, not bytes.
    if (xTaskCreate(task_sensors, "Sensors", 160, NULL, 3, NULL) != pdPASS ||
        xTaskCreate(task_buttons, "Buttons", 160, NULL, 1, NULL) != pdPASS ||
        xTaskCreate(task_control, "Control", 192, NULL, 2, NULL) != pdPASS ||
        xTaskCreate(task_oled, "OLED", 448, NULL, 1, NULL) != pdPASS ||
        xTaskCreate(task_uart, "UART", 448, NULL, 1, NULL) != pdPASS) app_fault();

    vTaskStartScheduler();
    app_fault();
    return 1;
}
