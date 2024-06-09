#include "tasks/displayTask.h"
#include "main.h"
#include <Wire.h>
#include <U8g2lib.h>

#define SSD1306_I2C_ADDRESS 0x3C
#define cloud_width 17
#define cloud_height 17
static unsigned char cloud[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00,
   0x60, 0x0c, 0x00, 0x30, 0x18, 0x00, 0x1c, 0x30, 0x00, 0x06, 0x70, 0x00,
   0x03, 0xe0, 0x00, 0x03, 0x80, 0x01, 0x03, 0x80, 0x01, 0x03, 0x80, 0x01,
   0x06, 0xc0, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00 };

#define wifi_full_width 17
#define wifi_full_height 17
static unsigned char wifi_full_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x0f, 0x00,
   0x18, 0x30, 0x00, 0x06, 0xc0, 0x00, 0xe0, 0x0f, 0x00, 0x30, 0x18, 0x00,
   0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00 };


#define wifi_3_width 17
#define wifi_3_height 17
static unsigned char wifi_3_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x30, 0x18, 0x00,
   0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00 };


#define wifi_2_width 17
#define wifi_2_height 17
static unsigned char wifi_2_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00 };


#define wifi_1_width 17
#define wifi_1_height 17
static unsigned char wifi_1_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00 };


void welcomeScreen();
void accessPointInfoScreen();
void initializeDisplay();
bool pingDisplay();

struct ScreenLayout {
    // Top section for connection info
    static const uint8_t TOP_SECTION_HEIGHT = 20;
    static const uint8_t TOP_SECTION_WIDTH = 128;

    // Bottom section for relay statuses
    static const uint8_t BOTTOM_SECTION_HEIGHT = 44;
    static const uint8_t BOTTOM_SECTION_WIDTH = 128;
};

int sda_pin = 16;
int scl_pin = 15;

/* Constructor */
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, scl_pin, sda_pin);
void displayTask(void* params) {
    Wire.begin(sda_pin, scl_pin);   // Initialize the I2C bus
    initializeDisplay();
    DisplayMessage currentDisplayData = {
        .IPaddress = "Ei wifi-yhteyttä",
        .internetConnection = false,
        .mqttConnection = false,
    };
    DisplayMessage newDisplayMessage;
    bool displayRefresh = true;
    bool showCloud = true;
 //   welcomeScreen();

    display.clearDisplay();
    RelaySettings relays[2];
    bool relaysInitialized[2] = { false, false };

    int wifiIconFrame = 0; // To keep track of the current WiFi icon frame
    const unsigned char* wifiIcons[] = { wifi_1_bits, wifi_2_bits, wifi_3_bits, wifi_full_bits };
    const int wifiIconCount = sizeof(wifiIcons) / sizeof(wifiIcons[0]);

    while (1) {
        if (xQueueReceive(displayQueue, &newDisplayMessage, 0) == pdTRUE || displayRefresh) {
            displayRefresh = false;
            switch (newDisplayMessage.updateType) {
            case INTERNET_UPDATE:
                currentDisplayData.internetConnection = newDisplayMessage.internetConnection;
                currentDisplayData.internetMode = newDisplayMessage.internetMode;
                strcpy(currentDisplayData.IPaddress, newDisplayMessage.IPaddress);
                break;
            case RELAY_UPDATE:
                relays[newDisplayMessage.relay.relayNumber] = newDisplayMessage.relay;
                relaysInitialized[newDisplayMessage.relay.relayNumber] = true;
                break;
            case ALL_UPDATES:
                break;

            case MQTT_UPDATE:
                currentDisplayData.mqttConnection = newDisplayMessage.mqttConnection;
                break;
            }

            display.firstPage();
            do {
                display.setDrawColor(1);
                display.setFont(u8g2_font_ncenB08_tr);
                display.drawHLine(0, ScreenLayout::TOP_SECTION_HEIGHT, ScreenLayout::TOP_SECTION_WIDTH); // Draw top line

                if (currentDisplayData.internetConnection) {
                    display.drawXBM(85, 0, wifi_full_width, wifi_full_height, wifi_full_bits);
                }
                else {
                    // Draw animated WiFi icon
                    display.drawXBM(85, 0, wifi_full_width, wifi_full_height, wifiIcons[wifiIconFrame]);
                }

                // make this flash if no connection to mqtt
                if (currentDisplayData.mqttConnection) {
                    display.drawXBM(105, 0, cloud_width, cloud_height, cloud);
                }
                else if (currentDisplayData.internetConnection && !currentDisplayData.mqttConnection && showCloud) {
                    display.drawXBM(105, 0, cloud_width, cloud_height, cloud);
                }

                // Draw the relay statuses
                display.setFont(u8g2_font_inr21_mf); // Set the font
                if (relaysInitialized[0]) {
                    if (relays[0].state == on) {
                        display.drawRBox(17, 23, 37, 39, 4); // Draw the box for the relay statuses
                        display.setDrawColor(0); // Set color to inverse
                        if (relays[0].mode == automatic)
                            display.drawStr(17 + (37 - display.getStrWidth("A")) / 2, 33 + 23 - 3, "A");
                        else
                            display.drawStr(17 + (37 - display.getStrWidth("M")) / 2, 33 + 23 - 3, "M");
                    }
                    else {
                        display.drawRFrame(17, 23, 37, 39, 4); // Draw the box for the relay statuses
                        display.setDrawColor(1); // Set color to inverse
                        if (relays[0].mode == automatic)
                            display.drawStr(17 + (37 - display.getStrWidth("A")) / 2, 33 + 23 - 3, "A");
                        else
                            display.drawStr(17 + (37 - display.getStrWidth("M")) / 2, 33 + 23 - 3, "M");
                    }
                }

                // Reset draw color to default before drawing the second relay
                display.setDrawColor(1);

                if (relaysInitialized[1]) {
                    if (relays[1].state == on) {
                        display.drawRBox(73, 23, 37, 39, 4); // Draw the box for the relay statuses
                        display.setDrawColor(0); // Set color to inverse
                        if (relays[1].mode == automatic)
                            display.drawStr(73 + (37 - display.getStrWidth("A")) / 2, 33 + 23 - 3, "A");
                        else
                            display.drawStr(73 + (37 - display.getStrWidth("M")) / 2, 33 + 23 - 3, "M");
                    }
                    else {
                        display.drawRFrame(73, 23, 37, 39, 4); // Draw the box for the relay statuses
                        display.setDrawColor(1); // Set color to inverse
                        if (relays[1].mode == automatic)
                            display.drawStr(73 + (37 - display.getStrWidth("A")) / 2, 33 + 23 - 3, "A");
                        else
                            display.drawStr(73 + (37 - display.getStrWidth("M")) / 2, 33 + 23 - 3, "M");
                    }
                }

            } while (display.nextPage());

        }
        else {
            bool displayOn = pingDisplay();
            if (!displayOn) {
                ESP_LOGE("DISPLAY", "Display not responding. Restarting display...");
                initializeDisplay();
                displayRefresh = true;
            }
        }

        if (!currentDisplayData.internetConnection) {
            wifiIconFrame = (wifiIconFrame + 1) % wifiIconCount; // Update WiFi icon frame
            vTaskDelay(pdMS_TO_TICKS(500)); // Delay to control animation speed
            displayRefresh = true;
        }
        else if (!currentDisplayData.mqttConnection) {
            showCloud = !showCloud;
            vTaskDelay(pdMS_TO_TICKS(500)); // Delay to control animation speed
            displayRefresh = true;
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(200)); // Delay to control animation speed
        }
    }
}



//==============================================================================
//  Functions
//==============================================================================


void initializeDisplay() {
    Wire.end(); // End the I2C bus to prevent any issues
    Wire.begin(sda_pin, scl_pin);   // Initialize the I2C bus
    display.begin();
    display.enableUTF8Print();
}

bool pingDisplay() {
    bool success = false;

    Wire.beginTransmission(SSD1306_I2C_ADDRESS);
    Wire.write(0x00); // Control byte: Co=0, D/C#=0
    Wire.endTransmission();

    // Request 1 byte from the SSD1306
    Wire.requestFrom(SSD1306_I2C_ADDRESS, 1);

    if (Wire.available()) {
        uint8_t status = Wire.read();
        success = !(status & 0x40);
    }
    return success;
}

void accessPointInfoScreen() {
    display.clearDisplay();
    display.firstPage();

    do {
        display.setFont(u8g2_font_ncenB08_tf);
        display.drawUTF8(0, 10, "Ei internet-yhteyttä.");
        display.drawUTF8(0, 20, "Käynnistetty tukiasema-tilassa.");
        display.drawUTF8(0, 30, "SSID:");
        display.drawUTF8(display.getStrWidth("SSID:") + 5, 30, CONFIG_ESP_WIFI_SSID);

    } while (display.nextPage());
}

void welcomeScreen() {
    display.setFont(u8g2_font_lubBI24_tf); // Set the font to use
    display.setFontDirection(0); // Set the text direction
    display.firstPage();

    int textX = 0;
    const char* top = "WATT";
    const char* bottom = "MASTER";

    int textWidth = display.getStrWidth(bottom) > display.getStrWidth(top) ? display.getStrWidth(bottom) : display.getStrWidth(top);

    int secondLineY = 32 + 24;
    bool printingStarted = false;

    while (textX + textWidth > 0) {
        display.firstPage();
        display.clearBuffer(); // Clear the display buffer

        do {
            display.drawStr(textX, 32, top); // Draw the top line
            display.drawStr(textX, secondLineY, bottom); // Draw the bottom line
        } while (display.nextPage());

        if (!printingStarted) {
            vTaskDelay(1000);
            printingStarted = true;
        }

        // Adjust the text position for the next iteration
        textX -= 3;
    }
}
