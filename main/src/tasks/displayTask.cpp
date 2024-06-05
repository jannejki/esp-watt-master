#include "tasks/displayTask.h"
#include "main.h"
#include <Wire.h>
int sda_pin = 15; // GPIO16 as I2C SDA
int scl_pin = 7; // GPIO17 as I2C SCL
#define SSD1306_I2C_ADDRESS 0x3C

void sendCommand(uint8_t command) {
    Wire.beginTransmission(SSD1306_I2C_ADDRESS);
    Wire.write(0x00); // Co = 0, D/C# = 0
    Wire.write(command);
    Wire.endTransmission();
}

void sendData(uint8_t data) {
    Wire.beginTransmission(SSD1306_I2C_ADDRESS);
    Wire.write(0x40); // Co = 0, D/C# = 1
    Wire.write(data);
    Wire.endTransmission();
}

void clearDisplay() {
    for (uint16_t i = 0; i < 128 * 64 / 8; i++) {
        sendData(0x00); // Clear all pixels
    }
}

void displayPattern() {
    for (uint16_t i = 0; i < 128 * 64 / 8; i++) {
        sendData(0xFF); // Set all pixels
    }
}



void initializeSSD1306() {
    // Initialization sequence based on datasheet
    sendCommand(0xAE); // Display off
    sendCommand(0xD5); // Set display clock divide ratio/oscillator frequency
    sendCommand(0x80); // Suggested ratio
    sendCommand(0xA8); // Set multiplex ratio
    sendCommand(0x3F); // 1/64 duty
    sendCommand(0xD3); // Set display offset
    sendCommand(0x00); // No offset
    sendCommand(0x40); // Set start line address
    sendCommand(0x8D); // Enable charge pump regulator
    sendCommand(0x14); // Enable
    sendCommand(0x20); // Set memory addressing mode
    sendCommand(0x00); // Horizontal addressing mode
    sendCommand(0xA1); // Set segment re-map
    sendCommand(0xC8); // Set COM output scan direction
    sendCommand(0xDA); // Set COM pins hardware configuration
    sendCommand(0x12); // Alternative COM pin config
    sendCommand(0x81); // Set contrast control
    sendCommand(0xCF); // Maximum contrast
    sendCommand(0xD9); // Set pre-charge period
    sendCommand(0xF1); // Phase 1 and 2 periods
    sendCommand(0xDB); // Set VCOMH deselect level
    sendCommand(0x40); // ~0.77 x VCC
    sendCommand(0xA4); // Output follows RAM content
    sendCommand(0xA6); // Normal display (non-inverted)
    sendCommand(0xAF); // Display on
}

void displayTask(void* params) {

    Wire.setPins(sda_pin, scl_pin); // Set the I2C pins before begin
    Wire.begin(); // join i2c bus (address optional for master)

    initializeSSD1306();
    while (1) {
        ESP_LOGI("displayTask", "displayTask running");
        clearDisplay();
        vTaskDelay(1000);
        displayPattern();
        vTaskDelay(1000);
    }

}
