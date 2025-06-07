#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SMALL_STACK_SIZE 1800  // Small stack size for overflow demonstration
#define NORMAL_STACK_SIZE 2048 // Normal stack size

// void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
// {
// #define ERR_STR1 "***HEHEHEHEHE ERROR*** A stack overflow in task "

// 	printf(ERR_STR1);
//     abort();
// }
// void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
//     printf("🔥 Stack overflow in task: %s\n", pcTaskName);
//     abort();  // Optional: reset or halt
// }

// Function to monitor the remaining stack space for a task
void monitor_task_stack(TaskHandle_t taskHandle, const char *taskName)
{
    UBaseType_t remainingStack = uxTaskGetStackHighWaterMark(taskHandle);
    printf("Remaining stack for %s: %d bytes\n", taskName, remainingStack);
}

// Task with normal stack usage
void normal_task(void *pvParameter)
{
    int localVar = 0;
    while (1)
    {
        localVar++;
        printf("Normal Task - Running, LocalVar: %d\n", localVar);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay to simulate work
    }
}

// Task with intentionally small stack to trigger overflow
void small_stack_task(void *pvParameter)
{
    char *largeArray = (char *)malloc(1500); // Allocate a large array on the heap
    if (largeArray == NULL)
    {
        printf("Failed to allocate memory for largeArray\n");
        vTaskDelete(NULL);
    }
    for (int i = 0; i < 200; i++)
    {
        largeArray[i] = i; // Fill the array
    }
    while (1)
    {
        printf("Small Stack Task - Running\n");
        printf("Free heap size: %u bytes\n", xPortGetFreeHeapSize());

        vTaskDelay(500 / portTICK_PERIOD_MS); // Delay to simulate work
    }
}

void app_main(void)
{
    TaskHandle_t normalTaskHandle, smallStackTaskHandle;

    // Create a task with a normal stack size
    xTaskCreate(normal_task, "Normal Task", NORMAL_STACK_SIZE, NULL, 1, &normalTaskHandle);

    // Create a task with a small stack size to demonstrate stack overflow
    xTaskCreate(small_stack_task, "Small Stack Task", SMALL_STACK_SIZE, NULL, 1, &smallStackTaskHandle);

    // Monitor the stack usage of each task every 2 seconds
    while (1)
    {
        monitor_task_stack(normalTaskHandle, "Normal Task");
        monitor_task_stack(smallStackTaskHandle, "Small Stack Task");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}