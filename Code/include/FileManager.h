#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "FS.h"
#include "SD.h"
#include <ArduinoJson.h>

// Định nghĩa Pin CS cho SD Card (Điều chỉnh theo mạch của bạn)
#define SD_CS_PIN 5 

class FileManager {
public:
    // Hàm khởi tạo và kiểm tra SD Card
    bool begin();
    
    // Hàm đọc JSON, sử dụng cú pháp tối ưu mà bạn đề xuất
    bool loadJsonFile(const char* path, JsonDocument* doc);

    // Hàm lưu JSON (cần thiết để lưu cấu hình Wi-Fi, Preset)
    bool saveJsonFile(const char* path, const JsonDocument& doc);

    // Hàm phục vụ file tĩnh (cho Web Server)
    File openFile(const char* path);

private:
    // Biến lưu trữ trạng thái khởi tạo
    bool sd_initialized = false;
};

#endif // FILEMANAGER_H