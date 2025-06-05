#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"

/* Define buffer size and maximum data value */
#define BUF_SIZE 5 /* Size of the circular buffer */
#define MAX_DATA 5 /* Maximum data value to produce */

/* Circular buffer and its pointers */
int buf[BUF_SIZE];
int head = 0; /* Points to the next position to write */
int tail = 0; /* Points to the next position to read */

/* Semaphores for synchronization */
SemaphoreHandle_t mutex;      /* Mutex for protecting shared resources */
SemaphoreHandle_t full_sem;   /* Semaphore to track full slots in the buffer */
SemaphoreHandle_t empty_sem;  /* Semaphore to track empty slots in the buffer */

/* Task for producing data */
void producer_task(void *param)
{
    static int data = 0; /* Data to produce */
    /* Wait for an empty slot in the buffer */
    if (xSemaphoreTake(empty_sem, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        /* Lock the mutex to safely access the buffer */
        xSemaphoreTake(mutex, portMAX_DELAY);
        buf[head] = data; /* Write data to the buffer */
        head = (head + 1) % BUF_SIZE; /* Update head pointer */
        ESP_LOGI("PRODUCER - ", "Produced: %d", data);
        data = (data + 1) % MAX_DATA; /* Increment data for next production */
        xSemaphoreGive(mutex); /* Unlock the mutex */
        xSemaphoreGive(full_sem); /* Signal that a slot is now full */
    }
    else
    {
        ESP_LOGW("PRODUCER - ", "Buffer full!"); /* Log if buffer is full */
    }

    /* Print the current state of the buffer */
    printf("PRODUCER -  buffer: [");
    if ((tail == head) && (uxSemaphoreGetCount(empty_sem) == 0))
    {
        /* Buffer is full, print all elements */
        for (int i = 0; i < BUF_SIZE; i++)
        {
            int index = (tail + i) % BUF_SIZE;
            printf("%d ", buf[index]);
        }
    }
    else if ((uxSemaphoreGetCount(full_sem) < 5) && (uxSemaphoreGetCount(empty_sem) > 0))
    {
        /* Buffer is partially filled, print elements from tail to head */
        for (int i = tail;; i = (i + 1) % (BUF_SIZE))
        {
            if ((0 == (i - head)))
                break; /* Stop when reaching head */
            printf("%d ", buf[i]);
        }
    }
    printf("]\n");

    vTaskDelete(NULL); /* Delete the task after execution */
}

/* Task for consuming data */
void consumer_task(void *param)
{
    int value;
    /* Wait for a full slot in the buffer */
    if (xSemaphoreTake(full_sem, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        /* Lock the mutex to safely access the buffer */
        xSemaphoreTake(mutex, portMAX_DELAY);
        value = buf[tail]; /* Read data from the buffer */
        tail = (tail + 1) % BUF_SIZE; /* Update tail pointer */
        xSemaphoreGive(mutex); /* Unlock the mutex */
        xSemaphoreGive(empty_sem); /* Signal that a slot is now empty */
        ESP_LOGI("CONSUMER - ", "Consumed: %d", value);
    }
    else
    {
        ESP_LOGW("CONSUMER - ", "Buffer empty!"); /* Log if buffer is empty */
    }

    /* Print the current state of the buffer */
    printf("CONSUMER -  buffer: [");
    if ((tail == head) && (uxSemaphoreGetCount(empty_sem) == 0))
    {
        /* Buffer is full, print all elements */
        for (int i = 0; i < BUF_SIZE; i++)
        {
            int index = (tail + i) % BUF_SIZE;
            printf("%d ", buf[index]);
        }
    }
    else if ((uxSemaphoreGetCount(full_sem) < 5) && (uxSemaphoreGetCount(empty_sem) > 0))
    {
        /* Buffer is partially filled, print elements from tail to head */
        for (int i = tail;; i = (i + 1) % (BUF_SIZE))
        {
            if ((0 == (i - head)))
                break; /* Stop when reaching head */
            printf("%d ", buf[i]);
        }
    }
    printf("]\n");

    vTaskDelete(NULL); /* Delete the task after execution */
}

/* Task for handling user input */
void input_task(void *param)
{
    uint8_t data[1]; /* Buffer to store input data */

    while (1)
    {
        /* Read a byte from UART */
        int len = uart_read_bytes(UART_NUM_0, data, 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            /* Create producer or consumer task based on input */
            if (data[0] == 'p')
            {
                xTaskCreate(producer_task, "producer", 2048, NULL, 1, NULL);
            }
            else if (data[0] == 'c')
            {
                xTaskCreate(consumer_task, "consumer", 2048, NULL, 1, NULL);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); /* Delay to avoid busy looping */
    }
}


void TaskFunction1(void *pvParameters) {
    while (1) {
        // Get remaining stack space
        UBaseType_t stackWatermark = uxTaskGetStackHighWaterMark(NULL);
        printf("Task1 Stack Remaining: %u bytes\n", stackWatermark * sizeof(StackType_t));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskFunction2(void *pvParameters) {
    // char largeArray[5]; // Large local array, consuming stack space
    while (1) {
        // Some processing
        UBaseType_t stackWatermark = uxTaskGetStackHighWaterMark(NULL);
        printf("Task2 Stack Remaining: %u bytes\n", stackWatermark * sizeof(StackType_t));
    vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    xTaskCreate(TaskFunction1, "Task1",2048, NULL, 1, NULL); 
    xTaskCreate(TaskFunction2, "Task2", 2048, NULL, 1, NULL); 
}

/* Main application entry point */
void app_main(void)
{
    setup(); /* Initialize tasks */
}
