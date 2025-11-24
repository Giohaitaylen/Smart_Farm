#ifndef SSD1306_H
#define SSD1306_H

#include <string>
#include <vector>

class SSD1306 {
public:
    SSD1306();
    ~SSD1306();

    // Khởi tạo màn hình (mặc định address 0x3C)
    int init(std::string devicePath, int deviceAddress = 0x3C);
    void deinit();

    // Các hàm hiển thị
    void clear();
    void update(); // Đẩy buffer ra màn hình
    void drawPixel(int x, int y, bool color);
    void drawChar(int x, int y, char c);
    void drawString(int x, int y, const std::string &str);
    
    // Hàm Supervisor: Hiển thị tổng quan
    void showStatus(float temp, float hum, int lux, int pumpState, int lightState);

private:
    int mDeviceFd;
    std::string mDevicePath;
    int mDeviceAddress;
    
    // Buffer cho màn hình 128x64
    unsigned char mBuffer[1024]; 

    int sendCommand(unsigned char cmd);
    int sendData(unsigned char *data, int len);
};

#endif // SSD1306_H