#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"

#define BUF_SIZE 5
#define MAX_DATA 5

int buf[BUF_SIZE];
int head = 0;
int tail = 0;

SemaphoreHandle_t mutex;
SemaphoreHandle_t full_sem;
SemaphoreHandle_t empty_sem;

void producer_task(void *param)
{
    static int data = 0;
    if (xSemaphoreTake(empty_sem, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        xSemaphoreTake(mutex, portMAX_DELAY); //lock the mutex
        buf[head] = data;
        head = (head + 1) % BUF_SIZE;
        ESP_LOGI("PRODUCER - ", "Produced: %d", data);
        data = (data + 1) % MAX_DATA;
        xSemaphoreGive(mutex);
        xSemaphoreGive(full_sem);
    }
    else
    {
        ESP_LOGW("PRODUCER - ", "Buffer full!");
    }

    printf("PRODUCER -  buffer: [");

    if ((tail == head) && (uxSemaphoreGetCount(empty_sem) == 0))
    {
        for (int i = 0; i < BUF_SIZE; i++)
        {
            int index = (tail + i) % BUF_SIZE;
            printf("%d ", buf[index]);
        }
    }
    else if ((uxSemaphoreGetCount(full_sem) < 5) && (uxSemaphoreGetCount(empty_sem) > 0))
    {
        for (int i = tail;; i = (i + 1) % (BUF_SIZE))
        {
            if ((0 == (i - head)))
                break; // the whole buffer is printed
            printf("%d ", buf[i]);
        }
    }
    else
    {
        // nothing
    }

    printf("]\n");

    vTaskDelete(NULL);
}

void consumer_task(void *param)
{
    int value;
    if (xSemaphoreTake(full_sem, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        value = buf[tail];
        tail = (tail + 1) % BUF_SIZE;
        xSemaphoreGive(mutex);
        xSemaphoreGive(empty_sem);
        ESP_LOGI("CONSUMER - ", "Consumed: %d", value);
    }
    else
    {
        ESP_LOGW("CONSUMER - ", "Buffer empty!");
    }


    printf("CONSUMER -  buffer: [");
    if ((tail == head) && (uxSemaphoreGetCount(empty_sem) == 0))
    {
        for (int i = 0; i < BUF_SIZE; i++)
        {
            int index = (tail + i) % BUF_SIZE;
            printf("%d ", buf[index]);
        }
    }
    else if ((uxSemaphoreGetCount(full_sem) < 5) && (uxSemaphoreGetCount(empty_sem) > 0))
    {
        for (int i = tail;; i = (i + 1) % (BUF_SIZE))
        {
            if ((0 == (i - head)))
                break; // the whole buffer is printed
            printf("%d ", buf[i]);
        }
    }
    else
    {
        // nothing
    }
    printf("]\n");

    vTaskDelete(NULL);
}

void input_task(void *param)
{
    uint8_t data[1];

    while (1)
    {
        int len = uart_read_bytes(UART_NUM_0, data, 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            if (data[0] == 'p')
            {
                xTaskCreate(producer_task, "producer", 2048, NULL, 1, NULL);
            }
            else if (data[0] == 'c')
            {
                xTaskCreate(consumer_task, "consumer", 2048, NULL, 1, NULL);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    mutex = xSemaphoreCreateMutex();
    full_sem = xSemaphoreCreateCounting(BUF_SIZE, 0);
    empty_sem = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);

    xTaskCreate(input_task, "input", 2048, NULL, 1, NULL);
}
