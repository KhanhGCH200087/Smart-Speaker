#include "ConnectivityManager.h"
#include <ESPmDNS.h>

ConnectivityManager::ConnectivityManager(FileManager *fileManager) : fm(fileManager)
{
    // Khởi tạo
}

// Hàm nội bộ: Tải Credentials từ SD Card
bool ConnectivityManager::loadCredentials(String &ssid, String &pass, String &ap_ssid, String &ap_pass)
{
    JsonDocument doc;
    if (fm->loadJsonFile(CONFIG_FILE_PATH WIFI_CONFIG_FILE, &doc))
    {
        ssid = doc[STA_SSID_CONFIG_KEY] | "";
        pass = doc[STA_PWD_CONFIG_KEY] | "";
        ap_ssid = doc[AP_SSID_CONFIG_KEY] | "Famio_Setup_AP";
        ap_pass = doc[AP_PWD_CONFIG_KEY] | "12345678";
        return ssid.length() > 0;
    }
    return false;
}

// Hàm nội bộ: Lưu Credentials vào SD Card
bool ConnectivityManager::saveCredentials(const String &ssid, const String &pass)
{
    JsonDocument doc;
    fm->loadJsonFile(CONFIG_FILE_PATH WIFI_CONFIG_FILE, &doc);
    doc[STA_SSID_CONFIG_KEY] = ssid;
    doc[STA_PWD_CONFIG_KEY] = pass;
    return fm->saveJsonFile(CONFIG_FILE_PATH WIFI_CONFIG_FILE, doc);
}

// Hàm nội bộ: Xóa Credentials
void ConnectivityManager::clearCredentials()
{
    ConnectivityManager::saveCredentials("", "");
}

// Hàm chính khởi tạo
bool ConnectivityManager::begin()
{
    String saved_ssid, saved_pass, ap_ssid, ap_pass;

    if (loadCredentials(saved_ssid, saved_pass, ap_ssid, ap_pass))
    {
        // --- PHA HOẠT ĐỘNG (OPERATIONAL PHASE) ---
        WiFi.mode(WIFI_STA);
        WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

        Serial.printf("Connecting to STA: %s\n", saved_ssid.c_str());

        long start_time = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start_time < CONNECTION_TIMEOUT_S * 1000))
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("\nSTA Connected. IP: %s\n", WiFi.localIP().toString().c_str());
            operational_mode = true;
        }
        else
        {
            // Thất bại: Chuyển về chế độ cấu hình
            Serial.println("\nSTA Connect FAILED/TIMEOUT. Entering Provisioning Mode.");
            clearCredentials(); // Xóa cấu hình sai để bắt đầu lại
            operational_mode = false;
            ESP.restart();
            // FALL-THROUGH để chạy Provisioning Mode
        }
    }

    if (!operational_mode)
    {
        // --- PHA CẤU HÌNH (PROVISIONING PHASE) ---
        Serial.println("Starting Provisioning Mode (AP+STA)...");
        WiFi.mode(WIFI_AP_STA);
        if (!WiFi.softAP(ap_ssid, ap_pass))
        {
            log_e("Soft AP creation failed.");
            while (1)
                ;
        }
        Serial.printf("AP SSID: %s | IP: %s\n", ap_ssid.c_str(), WiFi.softAPIP().toString().c_str());
    }
    // =========================================================
    // *** KHỞI TẠO MDNS (ÁP DỤNG CHO CẢ AP VÀ STA) ***
    // =========================================================
    if (MDNS.begin(MDNS_HOSTNAME))
    {
        // Đăng ký dịch vụ HTTP (Web Server)
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS Ready. Access at: http://%s.local\n", MDNS_HOSTNAME);
    }
    else
    {
        Serial.println("mDNS failed to start.");
    }
    // =========================================================
    return true;
}

// API: Bắt đầu quét mạng (Non-blocking)
int ConnectivityManager::startScanNetworks()
{
    if (!operational_mode)
    {
        if (scan_state == -2)
        {
            // Chỉ quét khi đang ở chế độ Provisioning (AP+STA)
            scan_state = WiFi.scanNetworks(true, false); // true: async, false: passive
        }
        else if (scan_state == -1)
        {
            int res = WiFi.scanComplete();
            if (res >= 0)
            {
                scan_state = res;
            }
        }
    }
    return scan_state;
}

// API: Lấy kết quả quét
void ConnectivityManager::getScanResults(JsonArray &array)
{
    if (scan_state >= 0)
    {
        // Nếu quét xong (scan_state >= 0)
        for (int i = 0; i < scan_state; ++i)
        {
            JsonObject network = array.add<JsonObject>();
            network["bssid"] = WiFi.BSSIDstr(i);
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["channel"] = WiFi.channel(i);
        }
        WiFi.scanDelete(); // Xóa kết quả tạm thời
        scan_state = -2;   // Reset trạng thái
    }
}

// API: Kiểm tra Credentials và lưu nếu thành công
bool ConnectivityManager::checkAndSaveCredentials(const String &ssid, const String &pass)
{
    if (operational_mode)
        return false; // Chỉ thực hiện khi đang ở Provisioning

    // 1. Cố gắng kết nối với Timeout 30s
    WiFi.begin(ssid.c_str(), pass.c_str());

    long start_time = millis();
    bool connected = false;

    while (millis() - start_time < CONNECTION_TIMEOUT_S * 1000)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            connected = true;
            break;
        }
        delay(500);
    }

    // 2. Đánh giá kết quả
    if (connected)
    {
        // Thành công: Lưu config và reset
        saveCredentials(ssid, pass);
        operational_mode = true;
        // Lưu ý: Chúng ta không gọi ESP.restart() trong hàm này mà để AppWebServer xử lý phản hồi API và reset
    }
    WiFi.mode(WIFI_AP_STA); // Đảm bảo AP+STA vẫn chạy
    return connected;
}

// API: Buộc đưa thiết bị về chế độ cấu hình
void ConnectivityManager::resetToProvisioning()
{
    Serial.println("Manual reset to Provisioning from API. Restarting device...");
    clearCredentials();
    ESP.restart(); // Reset sẽ tự động đưa về Provisioning Mode
}

// API: Kích hoạt reset thiết bị thủ công
void ConnectivityManager::manualReset()
{
    Serial.println("Manual reset triggered from API. Restarting device...");
    ESP.restart();
}