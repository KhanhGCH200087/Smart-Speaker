#ifndef APPWEBSERVER_H
#define APPWEBSERVER_H

#include <WiFi.h>
#include <WebServer.h>    // Thư viện WebServer chuẩn
#include "FMRadio.h"      // Cần để điều khiển FM
#include "PowerManager.h" // Cần để điều khiển nguồn
#include "FileManager.h"  // Cần để phục vụ file tĩnh và lưu config
#include "Constants.h"    // Nơi chứa các hằng số
#include "ConnectivityManager.h"

class AppWebServer
{
public:
    // Constructor nhận con trỏ của các module khác
    AppWebServer(FMRadio *radio, PowerManager *power, FileManager *fileMgr, ConnectivityManager *connectivity);

    bool begin();

    // QUAN TRỌNG: Hàm này PHẢI được gọi liên tục trong loop()
    void handleClient();

private:
    // Khai báo đối tượng WebServer
    WebServer server;

    // Con trỏ tới các module khác
    ConnectivityManager *connectivity;
    FMRadio *fmRadio;
    PowerManager *powerManager;
    FileManager *fileManager;

    // Hàm đăng ký tất cả các API endpoints
    void registerAPIs();

    // Các hàm xử lý request cụ thể
    void handleRoot();
    void handleStaticFile();
    void handleSystemVolume();

    // API FM module
    void handleFmPower();
    void handleFmSeek();
    void handleFmStatus();
    void handleFmSaveChannel();
    void handleFmSelectChannel();
    void handleFmLoadChannels();
    void handleFmSetFreq();
    void handleFmVolume();
    void handleFmDeleteChannel();
    // CORS helper
    void sendCORSHeaders();
    const char* getContentType(const String& path);
    // API Cấu hình Wi-Fi
    void handleGetWifiStatus();    // Trạng thái (AP/STA/Operational)
    void handleScanNetworks();     // Bắt đầu/Lấy kết quả quét
    void handleSubmitWifiConfig(); // Kiểm tra và lưu config
    void handleResetWifiConfig();  // Buộc về Provisioning Mode
    // API Hệ thống
    void handleSystemReset();      // Kích hoạt reset thủ công
    // ... Thêm các hàm xử lý API khác
};

#endif // APPWEBSERVER_H