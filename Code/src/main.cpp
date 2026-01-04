#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h> // Thư viện JSON

// Bao gồm các file Header của các lớp đã tạo
#include "Constants.h"
#include "FileManager.h"
#include "PowerManager.h"
#include "FMRadio.h"
#include "AppWebServer.h"
#include "ConnectivityManager.h"

// =========================================================
// Khai báo các Đối tượng Toàn cục (Global Managers)
// =========================================================

FileManager fileManager;
PowerManager powerManager;
FMRadio fmRadio(&fileManager);
ConnectivityManager connectivityManager(&fileManager);
AppWebServer appWebServer(&fmRadio, &powerManager, &fileManager, &connectivityManager);

// =========================================================
// Setup() - Khởi tạo Hệ thống
// =========================================================

void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("\n--- Bắt đầu Hệ thống Famio FM Radio ESP32 ---");

    // Khởi tạo PowerManager và SD Card trước
    powerManager.begin();
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
    if (!fileManager.begin())
    {
        Serial.println("Lỗi nghiêm trọng: Không thể khởi tạo SD Card.");
        return;
    }

    // 1. TẢI CẤU HÌNH (Sử dụng JsonDocument, phù hợp với v7)
    JsonDocument commonConfig;

    // Đường dẫn được lấy từ Constants.h (PROJECT_ROOT_DIR)
    if (!fileManager.loadJsonFile(CONFIG_FILE_PATH COMMON_CONFIG_FILE, &commonConfig))
    {
        Serial.println("Không tải được /config/common.json. Sử dụng cấu hình mặc định.");
    }

    int initialVolume = commonConfig["volume"] | 50;
    float initialFreq = commonConfig["freq"] | 99.5f;

    // QUẢN LÝ KẾT NỐI WI-FI
    if (!connectivityManager.begin())
    {
        // Nếu kết nối/cấu hình thất bại, khởi động lại để thử lại
        Serial.println("Hệ thống không thể kết nối");
        while (1)
            ;
    }

    // KHỞI TẠO WEB SERVER
    appWebServer.begin();
    Wire.begin();
    // HOẶC: Wire.begin(SDA_PIN, SCL_PIN); nếu bạn dùng chân tùy chỉnh
    Serial.println("SETUP: Khởi tạo I2C Bus thành công.");
}

// =========================================================
// Loop() - Vòng lặp Chính
// =========================================================

void loop()
{
    appWebServer.handleClient();
    delay(10);
}