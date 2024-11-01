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

        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1-second delay
    }
}

void app_main() {
    printf("Program Starting...\n");

    // PSRAM initialization with error checking
    esp_err_t psram_init_result = esp_psram_init();
    if (psram_init_result == ESP_OK) {
        printf("PSRAM initialized successfully.\n");
        printf("PSRAM Size: %u MB\n", esp_psram_get_size() / (1024 * 1024));
    } else {
        printf("Failed to initialize PSRAM. Error code: 0x%x\n", psram_init_result);
        printf("Please check PSRAM settings in menuconfig and verify hardware setup.\n");
    }

    // Creating a task to print board information
    xTaskCreate(
        TaskPrintInfo,   // Function to be executed by the task
        "TaskPrintInfo", // Task name (for debugging)
        2048,            // Stack size in bytes
        NULL,            // Parameters to pass to the task (we have none)
        1,               // Task priority
        NULL             // Task handle (can be left NULL)
    );
}
