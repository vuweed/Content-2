#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// // Stack overflow hook – đừng quên khai báo trong FreeRTOSConfig.h
// void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
//     printf("🔥 Stack overflow in task: %s\n", pcTaskName);
//     abort();  // Đảm bảo dừng hệ thống nếu cần
// }

// Task tiêu tốn stack tăng dần
/* Task that gradually increases stack usage */
void variable_stack_task(void *pvParameters)
{
    int i = 1;
    TaskHandle_t xTask = xTaskGetCurrentTaskHandle();
    /* Log remaining stack watermark */
    UBaseType_t watermark;
    while (1)
    {
        size_t size = i * 50; /* Gradually increase stack usage */
        volatile char buffer[size];

        printf("\n📦 Loop %d | Allocating %d bytes on stack\n", i, size);

        /* Fill the large array (volatile to prevent optimization) */
        for (int j = 0; j < size; j++)
        {
            buffer[j] = j % 256;
        }

        watermark = uxTaskGetStackHighWaterMark(NULL);
        printf("💧 Stack high watermark: %u bytes\n", watermark * sizeof(StackType_t));

        vTaskDelay(pdMS_TO_TICKS(1000));
        i++;
    }
}

void app_main()
{
    // Tạo task với stack khá nhỏ để dễ gây tràn
    xTaskCreate(variable_stack_task, "OverflowTask", 2024, NULL, 1, NULL);
}
