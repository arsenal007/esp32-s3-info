#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_psram.h>
#include <esp_idf_version.h>
#include <esp_mac.h>
#include <esp_clk_tree.h>
#include <esp_littlefs.h>
#include <driver/rmt.h>
#include <led_strip.h>


void TaskPrintInfo(void *parameter) {
    for (;;) { // Infinite loop for the task
        printf("\nESP32-S3 Board Information:\n");

        printf("Processor Architecture: Xtensa Dual-core 32-bit LX7\n");

        printf("ESP32 Board Identification\n");

        // Chip model
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        printf("Chip Model: ESP32-S3\n");

        // Chip revision
        printf("Chip Revision: %d\n", chip_info.revision);

        // Cores count
        printf("Number of Cores: %d\n", chip_info.cores);

        // Flash size
        uint32_t flash_size = 0;
        esp_flash_get_size(NULL, &flash_size);
        printf("Flash Size: %ld MB\n", flash_size / (1024 * 1024));

        // Get CPU frequency using esp_clk_tree_src_get_freq_hz
        uint32_t cpu_freq_hz = 0;
        esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &cpu_freq_hz);
        printf("CPU Frequency: %ld MHz\n", cpu_freq_hz / 1000000);

        printf("RAM (SRAM): %u KB\n", heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024);

        printf("Free Heap Memory: %u KB\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024);

        // PSRAM information
        size_t psramSize = esp_psram_get_size();
        printf("PSRAM Memory: ");
        if (psramSize > 0) {
            printf("%u KB\n", psramSize / 1024);
        } else {
            printf("Not Available\n");
        }

        // MAC address as Chip ID
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        printf("Board ID (Chip ID): %02X%02X%02X\n", mac[3], mac[4], mac[5]);

        printf("SDK Version: %s\n", esp_get_idf_version());

        // LittleFS Information
        size_t total_bytes, used_bytes;
        esp_err_t ret = esp_littlefs_info("storage", &total_bytes, &used_bytes);
        if (ret == ESP_OK) {
            printf("LittleFS Total Size: %d KB\n", total_bytes / 1024);
            printf("LittleFS Used Size: %d KB\n", used_bytes / 1024);
        } else {
            printf("Failed to retrieve LittleFS info (%s)\n", esp_err_to_name(ret));
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS); // 5-second delay
    }
}

#define LED_PIN 38           // Пін підключення WS2812
#define LED_RMT_CHANNEL RMT_CHANNEL_0  // Канал RMT
#define LED_NUM 1            // Кількість світлодіодів у стрічці

static led_strip_t *strip;

void TaskLedControl(void *parameter) {
    // Ініціалізація об'єкта для керування світлодіодами
    strip = led_strip_init(LED_RMT_CHANNEL, LED_PIN, LED_NUM);
    strip->clear(strip, 100);  // Очищення (вимкнення) світлодіодів

    while (1) {
        // Встановлення червоного кольору
        strip->set_pixel(strip, 0, 255, 0, 0);
        strip->refresh(strip, 100);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        // Встановлення зеленого кольору
        strip->set_pixel(strip, 0, 0, 255, 0);
        strip->refresh(strip, 100);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        // Встановлення синього кольору
        strip->set_pixel(strip, 0, 0, 0, 255);
        strip->refresh(strip, 100);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// Розмір стека для кожного завдання
#define STACK_SIZE 4096

// Оголошення масивів для статичної алокації пам'яті для TaskLedControl
StaticTask_t ledControlTaskBuffer;
StackType_t ledControlStack[STACK_SIZE];

// Оголошення масивів для статичної алокації пам'яті для TaskPrintInfo
StaticTask_t printInfoTaskBuffer;
StackType_t printInfoStack[STACK_SIZE];

void app_main() {
    printf("Program Starting...\n");

    // PSRAM initialization with error checking
    esp_err_t psram_init_result = esp_psram_init();
    if (psram_init_result == ESP_OK) {
        printf("PSRAM initialized successfully.\n");
        printf("PSRAM Size: %u MB\n", esp_psram_get_size() / (1024 * 1024));
    } else {
        printf("Failed to initialize PSRAM. Error code: (%s)\n", esp_err_to_name(psram_init_result));
        printf("Please check PSRAM settings in menuconfig and verify hardware setup.\n");
    }

    printf("Initializing LittleFS\n");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        printf("Failed to mount LittleFS (%s)\n", esp_err_to_name(ret));
        return;
    }
    printf("LittleFS mounted successfully\n");

    // Створення статичного завдання для контролю LED на ядрі 1
 /*   xTaskCreateStaticPinnedToCore(
        TaskLedControl,                // Функція завдання
        "TaskLedControl",              // Назва завдання
        STACK_SIZE,                    // Розмір стека
        NULL,                          // Параметри для завдання
        1,                             // Пріоритет завдання
        ledControlStack,               // Стек пам'яті
        &ledControlTaskBuffer,         // Обробник пам'яті для TCSB
        0                              // Ядро для виконання завдання
    );*/

    // Створення статичного завдання для виведення інформації на будь-якому доступному ядрі
    xTaskCreateStaticPinnedToCore(
        TaskPrintInfo,                 // Функція завдання
        "TaskPrintInfo",               // Назва завдання
        STACK_SIZE,                    // Розмір стека
        NULL,                          // Параметри для завдання
        1,                             // Пріоритет завдання
        printInfoStack,                // Стек пам'яті
        &printInfoTaskBuffer,          // Обробник пам'яті для TCSB
        0                              // Ядро для виконання завдання
    );
}
