#include "ssd1306.h"
#include "font.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cstdio>

SSD1306::SSD1306() : mDeviceFd(-1) {}
SSD1306::~SSD1306() { deinit(); }

int SSD1306::init(std::string devicePath, int deviceAddress) {
    mDeviceFd = open(devicePath.c_str(), O_RDWR);
    if (mDeviceFd < 0) {
        perror("OLED: Open failed");
        return -1;
    }
    if (ioctl(mDeviceFd, I2C_SLAVE, deviceAddress) < 0) {
        perror("OLED: Set address failed");
        close(mDeviceFd);
        return -1;
    }
    mDeviceAddress = deviceAddress;

    // Init sequence cho SSD1306 128x64
    unsigned char cmds[] = {
        0xAE,       // Display OFF
        0x20, 0x00, // Memory addressing mode: Horizontal
        0xB0,       // Page start address
        0xC8,       // COM output scan direction
        0x00,       // Low column address
        0x10,       // High column address
        0x40,       // Start line address
        0x81, 0xFF, // Contrast
        0xA1,       // Segment re-map
        0xA6,       // Normal display
        0xA8, 0x3F, // Multiplex ratio
        0xA4,       // Output follows RAM content
        0xD3, 0x00, // Display offset
        0xD5, 0x80, // Clock divide ratio
        0xD9, 0xF1, // Pre-charge period
        0xDA, 0x12, // COM pins hardware config
        0xDB, 0x40, // VCOMH deselect level
        0x8D, 0x14, // Charge pump on
        0xAF        // Display ON
    };

    for (unsigned char cmd : cmds) sendCommand(cmd);
    clear();
    update();
    return 0;
}

void SSD1306::deinit() {
    if (mDeviceFd >= 0) close(mDeviceFd);
}

int SSD1306::sendCommand(unsigned char cmd) {
    unsigned char buf[2] = {0x00, cmd}; // 0x00 là Co byte cho Command
    return write(mDeviceFd, buf, 2);
}

int SSD1306::sendData(unsigned char *data, int len) {
    // I2C write block data. Byte đầu là 0x40 (Data)
    unsigned char buf[1025];
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    return write(mDeviceFd, buf, len + 1);
}

void SSD1306::clear() {
    memset(mBuffer, 0, sizeof(mBuffer));
}

void SSD1306::drawPixel(int x, int y, bool color) {
    if(x < 0 || x >= 128 || y < 0 || y >= 64) return;
    if(color)
        mBuffer[x + (y / 8) * 128] |= (1 << (y % 8));
    else
        mBuffer[x + (y / 8) * 128] &= ~(1 << (y % 8));
}

void SSD1306::drawChar(int x, int y, char c) {
    int index = c - 32;
    if(index < 0 || index > 95) return; // Ngoài phạm vi hiển thị
    const unsigned char* bitmap =  font5x7[index];
    for(int i=0; i<5; i++) {
        unsigned char line = bitmap[i];
        for(int j=0; j<8; j++) {
            if(line & (1<<j)) drawPixel(x+i, y+j, true);
        }
    }
}

void SSD1306::drawString(int x, int y, const std::string &str) {
    int cursorX = x;
    for(char c : str) {
        drawChar(cursorX, y, c);
        cursorX += 6; // 5 width + 1 space
    }
}

void SSD1306::update() {
    sendData(mBuffer, 1024);
}

void SSD1306::showStatus(float temp, float hum, int lux, int pumpState, int lightState) {
    clear();
    
    // Format chuỗi
    char buf[32];
    
    sprintf(buf, "Temp: %d C", (int)temp);
    drawString(0, 0, buf);
    
    sprintf(buf, "Hum : %d %%", (int)hum);
    drawString(0, 10, buf);
    
    sprintf(buf, "Lux : %d", lux);
    drawString(0, 20, buf);
    
    sprintf(buf, "Lamp: %s", lightState ? "ON" : "OFF");
    drawString(0, 40, buf);
    
    sprintf(buf, "Pump: %s", pumpState ? "ON" : "OFF");
    drawString(0, 50, buf);
    
    update();
}