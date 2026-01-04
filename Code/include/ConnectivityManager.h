#ifndef CONNECTIVITYMANAGER_H
#define CONNECTIVITYMANAGER_H

#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "FileManager.h"
#include "Constants.h" // Cần để truy cập FileManager


class ConnectivityManager {
public:
    // Cấu trúc để lưu kết quả quét
    struct ScanResult {
        String ssid;
        int rssi;
    };
    
    // Constructor nhận FileManager
    ConnectivityManager(FileManager* fileManager);

    // Hàm chính khởi tạo và thiết lập chế độ Wi-Fi
    // Trả về TRUE nếu ở chế độ Operational (STA), FALSE nếu ở chế độ Provisioning (AP+STA)
    bool begin();

    // Lấy trạng thái hoạt động hiện tại
    bool isOperational() const { return operational_mode; }

    // --- Hàm phục vụ API ---
    
    // API: Thực hiện Quét mạng Wi-Fi (Non-blocking)
    // Trả về -1: đang quét, -2: chưa quét, >=0: số mạng tìm thấy
    int startScanNetworks();
    
    // API: Lấy kết quả quét sau khi scanNetworks() hoàn tất
    // Trả về một JsonArray chứa danh sách mạng
    void getScanResults(JsonArray& array);

    // API: Kiểm tra Credentials và lưu nếu thành công
    // Trả về true nếu thành công và đã gọi reset, false nếu thất bại/timeout.
    bool checkAndSaveCredentials(const String& ssid, const String& pass);

    // API: Buộc đưa thiết bị về chế độ cấu hình (Change Network)
    // Thực hiện reset.
    void resetToProvisioning();

    // API: Kích hoạt reset thiết bị thủ công (từ web)
    // Thực hiện khởi động lại thiết bị.
    void manualReset();

private:
    FileManager* fm;
    bool operational_mode = false;
    int scan_state = -2; // -2: chưa quét, -1: đang quét, >=0: số mạng tìm thấy

    // Hàm nội bộ: Tải Credentials từ SD Card
    bool loadCredentials(String& ssid, String& pass, String& ap_ssid, String& ap_pass);

    // Hàm nội bộ: Lưu Credentials vào SD Card
    bool saveCredentials(const String& ssid, const String& pass);

    // Hàm nội bộ: Xóa Credentials
    void clearCredentials();
};

#endif // CONNECTIVITYMANAGER_H