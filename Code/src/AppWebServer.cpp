#include "AppWebServer.h"
#include <ArduinoJson.h>
#include <ConnectivityManager.h>

// Constructor: Khởi tạo Web Server ở cổng 80 và lưu trữ con trỏ
AppWebServer::AppWebServer(FMRadio *radio, PowerManager *power, FileManager *fileMgr, ConnectivityManager *connectivity)
    : server(80), fmRadio(radio), powerManager(power), fileManager(fileMgr), connectivity(connectivity)
{

    // Kiểm tra tính hợp lệ của con trỏ (tùy chọn)
    if (!fmRadio || !powerManager || !fileManager)
    {
        Serial.println("Cảnh báo: Một số module Radio/Power/File chưa được cấp phát.");
    }
}

// =========================================================
// Khởi tạo Wi-Fi và Server
// =========================================================

bool AppWebServer::begin()
{
    // Đăng ký tất cả các API endpoints
    registerAPIs();

    // Bắt đầu Web Server
    server.begin();
    return true;
}

// =========================================================
// Hàm Đăng ký API (Sử dụng std::bind)
// =========================================================

void AppWebServer::registerAPIs()
{

    // API Lấy trạng thái FM
    server.on("/api/fm/status", HTTP_GET, std::bind(&AppWebServer::handleFmStatus, this));
    server.on("/api/fm/power", HTTP_POST, std::bind(&AppWebServer::handleFmPower, this));
    server.on("/api/fm/setfreq", HTTP_POST, std::bind(&AppWebServer::handleFmSetFreq, this));
    server.on("/api/fm/seek", HTTP_GET, std::bind(&AppWebServer::handleFmSeek, this));
    server.on("/api/fm/volume", HTTP_POST, std::bind(&AppWebServer::handleFmVolume, this));
    server.on("/api/fm/save", HTTP_POST, std::bind(&AppWebServer::handleFmSaveChannel, this));
    server.on("/api/fm/select", HTTP_GET, std::bind(&AppWebServer::handleFmSelectChannel, this));
    server.on("/api/fm/channels", HTTP_GET, std::bind(&AppWebServer::handleFmLoadChannels, this));
    server.on("/api/fm/delete", HTTP_DELETE, std::bind(&AppWebServer::handleFmDeleteChannel, this));

    // API Điều chỉnh âm lượng
    server.on("/api/system/volume", HTTP_POST, std::bind(&AppWebServer::handleSystemVolume, this));

    // API Cấu hình Wi-Fi
    server.on("/api/wifi/status", HTTP_GET, std::bind(&AppWebServer::handleGetWifiStatus, this));
    server.on("/api/wifi/scan", HTTP_GET, std::bind(&AppWebServer::handleScanNetworks, this));
    server.on("/api/wifi/config", HTTP_POST, std::bind(&AppWebServer::handleSubmitWifiConfig, this));
    server.on("/api/wifi/reset", HTTP_POST, std::bind(&AppWebServer::handleResetWifiConfig, this));

    // API Hệ thống
    server.on("/api/system/reset", HTTP_POST, std::bind(&AppWebServer::handleSystemReset, this));

    // 1. Root ("/") - Trang chính
    server.on("/", HTTP_GET, std::bind(&AppWebServer::handleRoot, this));
    AppWebServer::handleStaticFile();

    // Global handler: tất cả các OPTIONS (preflight) và các request không khớp
    server.onNotFound([this]()
                      {
        // Trả lời preflight (OPTIONS) hoặc phục vụ file tĩnh từ SD
        if (server.method() == HTTP_OPTIONS) {
            sendCORSHeaders();
            server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            server.send(204, "text/plain", "");
            return;
        }

        // Nếu không phải OPTIONS, thử phục vụ file từ /ui
        String path = server.uri();
        if (path == "/") path = "/index.html";
        String fsPath = String(UI_PATH) + path;
        File f = fileManager->openFile(fsPath.c_str());
        if (f) {
            sendCORSHeaders();
            const char* contentType = getContentType(path);
            server.streamFile(f, contentType);
            f.close();
            return;
        }

        // Không tìm thấy
        sendCORSHeaders();
        server.send(404, "text/plain", "Not Found"); });
}

// =========================================================
// Xử lý Request Cụ thể
// =========================================================

void AppWebServer::handleRoot()
{
    // Phục vụ file index.html từ thẻ SD
    File file = fileManager->openFile(UI_PATH "/index.html");
    if (file)
    {
        sendCORSHeaders();
        server.streamFile(file, "text/html");
        file.close();
    }
    else
    {
        sendCORSHeaders();
        server.send(404, "text/plain", "File /index.html not found on SD Card!");
    }
}

void AppWebServer::handleStaticFile()
{
    server.serveStatic("/", SD, PROJECT_ROOT_DIR "/ui/");
}

void AppWebServer::handleFmStatus()
{
    // Cấp phát bộ nhớ cho phản hồi JSON
    JsonDocument statusDoc;

    // GỌI HÀM CỦA FMRADIO ĐỂ LẤY TRẠNG THÁI THẬT
    if (fmRadio)
    {
        fmRadio->getStatus(&statusDoc);
    }
    else
    {
        // Giả lập dữ liệu (Xóa khi tích hợp FMRadio)
        statusDoc["isPowered"] = false;
        statusDoc["freq"] = 99.5;
        statusDoc["volume"] = 75;
        statusDoc["stereo"] = true;
        statusDoc["rssi"] = 68;
    }

    String response;
    serializeJson(statusDoc, response);

    sendCORSHeaders();
    server.send(200, "application/json", response);
}

void AppWebServer::handleSystemVolume()
{
    // Kiểm tra xem tham số POST 'level' có được gửi không
    if (server.hasArg("level"))
    {
        int newLevel = server.arg("level").toInt();

        // powerManager->setVolume(newLevel); // Gọi hàm của module PowerManager

        sendCORSHeaders();
        server.send(200, "text/plain", "OK");
    }
    else
    {
        sendCORSHeaders();
        server.send(400, "text/plain", "Tham số 'level' bị thiếu.");
    }
}

// =========================================================
// Hàm xử lý chính (BẮT BUỘC TRONG LOOP)
// =========================================================

void AppWebServer::handleClient()
{
    // Hàm này phải được gọi liên tục trong main loop() để Web Server hoạt động
    server.handleClient();
}

// --- XỬ LÝ API WIFI ---

void AppWebServer::handleGetWifiStatus()
{
    JsonDocument doc;
    doc["isOperational"] = connectivity->isOperational();
    doc["ip"] = connectivity->isOperational() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    sendCORSHeaders();
    server.send(200, "application/json", jsonResponse);
}

void AppWebServer::handleScanNetworks()
{
    int state = connectivity->startScanNetworks();
    JsonDocument doc; // Kích thước lớn hơn để chứa danh sách mạng

    if (state == -1)
    {
        // Đang quét
        Serial.println("Scan in progress ...");
        doc["status"] = "scanning";
    }
    else if (state == -2)
    {
        // Chưa quét/Quét bị xóa, gọi lại scan để kích hoạt
        Serial.println("Scan started in background");
        doc["status"] = "ready_to_scan";
        connectivity->startScanNetworks(); // Gọi lại để bắt đầu quét
    }
    else
    {
        // Quét hoàn tất (state >= 0)
        Serial.println("Scan networks completed. Gathering result...");
        doc["status"] = "complete";
        doc["count"] = state;
        JsonArray networks = doc["networks"].to<JsonArray>();
        connectivity->getScanResults(networks);
    }
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    sendCORSHeaders();
    server.send(200, "application/json", jsonResponse);
}

void AppWebServer::handleSubmitWifiConfig()
{
    // Giả định dữ liệu gửi lên là JSON: {"ssid": "...", "pass": "..."}
    if (server.method() != HTTP_POST || !server.hasArg("plain"))
    {
        server.send(400, "text/plain", "Bad Request");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error)
    {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    String ssid = doc["ssid"].as<String>();
    String pass = doc["pass"].as<String>();

    if (connectivity->checkAndSaveCredentials(ssid, pass))
    {
        // Kết nối thành công, đã lưu config. Phản hồi và reset.
        Serial.printf("Connect sussessfully to Wifi ssid: %s", ssid.c_str());
        sendCORSHeaders();
        server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Config saved. Restarting device to continue.\"}");
        // Thực hiện reset
    }
    else
    {
        // Kết nối thất bại/timeout
        sendCORSHeaders();
        server.send(400, "application/json", "{\"status\":\"failed\", \"message\":\"Connection failed or timeout after 30s. Check credentials.\"}");
    }
}

void AppWebServer::handleResetWifiConfig()
{
    // API đưa về Provisioning Mode
    sendCORSHeaders();
    server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Resetting to Provisioning Mode. Device will restart.\"}");
    delay(500);
    connectivity->resetToProvisioning(); // Thực hiện reset
}

void AppWebServer::handleSystemReset()
{
    // API kích hoạt reset thiết bị thủ công
    sendCORSHeaders();
    server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Device is restarting...\"}");
    delay(500);
    connectivity->manualReset(); // Thực hiện reset
}

// ---------------------------------------------------------
// CORS và MIME helpers
// ---------------------------------------------------------
void AppWebServer::sendCORSHeaders()
{
    // Cho phép tất cả origin
    server.sendHeader("Access-Control-Allow-Origin", "*");
    // Không cho phép cookie mặc định
    server.sendHeader("Access-Control-Allow-Credentials", "false");
}

const char *AppWebServer::getContentType(const String &path)
{
    if (path.endsWith(".htm") || path.endsWith(".html"))
        return "text/html";
    if (path.endsWith(".css"))
        return "text/css";
    if (path.endsWith(".js"))
        return "application/javascript";
    if (path.endsWith(".png"))
        return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
        return "image/jpeg";
    if (path.endsWith(".gif"))
        return "image/gif";
    if (path.endsWith(".svg"))
        return "image/svg+xml";
    if (path.endsWith(".ico"))
        return "image/x-icon";
    if (path.endsWith(".json"))
        return "application/json";
    if (path.endsWith(".txt"))
        return "text/plain";
    return "application/octet-stream";
}

void AppWebServer::handleFmPower()
{
    sendCORSHeaders();
    if (server.hasArg("state"))
    {
        String state = server.arg("state");
        if (state == "on")
        {
            // Initialize and power on FM radio hardware
            fmRadio->begin();
            server.send(200, "application/json", "{\"status\":\"success\", \"powered\":true}");
            return;
        }
        else if (state == "off")
        {
            fmRadio->powerOff();
            server.send(200, "application/json", "{\"status\":\"success\", \"powered\":false}");
            return;
        }
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Thiếu tham số state (on/off)\"}");
}

void AppWebServer::handleFmSeek()
{
    sendCORSHeaders();
    if (server.hasArg("direction"))
    {
        String dir = server.arg("direction");
        if (dir == "up")
        {
            fmRadio->seekUp();
        }
        else if (dir == "down")
        {
            fmRadio->seekDown();
        }
        else if (dir == "next")
        {
            fmRadio->autoSeekNext();
        }
        else
        {
            server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Tham số direction không hợp lệ (up/down/next)\"}");
            return;
        }
        server.send(200, "application/json", "{\"status\":\"success\", \"freq\":" + String(fmRadio->getCurrentFrequency(), 1) + "}");
        return;
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Thiếu tham số direction (up/down/next)\"}");
}

void AppWebServer::handleFmSaveChannel()
{
    sendCORSHeaders();
    float currentFreq = fmRadio->getCurrentFrequency();
    fmRadio->saveChannel(currentFreq);
    server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Đã lưu kênh\", \"freq\":" + String(currentFreq, 1) + "}");
}

void AppWebServer::handleFmSelectChannel()
{
    sendCORSHeaders();
    if (server.hasArg("index"))
    {
        int index = server.arg("index").toInt();
        fmRadio->selectSavedChannel(index);
        server.send(200, "application/json", "{\"status\":\"success\", \"freq\":" + String(fmRadio->getCurrentFrequency(), 1) + "}");
        return;
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Thiếu tham số index\"}");
}

void AppWebServer::handleFmLoadChannels()
{
    sendCORSHeaders();
    JsonDocument doc;
    fmRadio->getSavedChannels(&doc);

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void AppWebServer::handleFmSetFreq()
{
    sendCORSHeaders();
    if (server.hasArg("freq"))
    {
        float freq = server.arg("freq").toFloat();
        if (freq >= 87.0 && freq <= 108.0)
        {
            fmRadio->setFrequency(freq);
            server.send(200, "application/json", "{\"status\":\"success\", \"freq\":" + String(fmRadio->getCurrentFrequency(), 1) + "}");
            return;
        }
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Tần số không hợp lệ (87.0-108.0)\"}");
}

void AppWebServer::handleFmVolume()
{
    sendCORSHeaders();
    if (server.hasArg("level"))
    {
        int level = server.arg("level").toInt();
        fmRadio->setVolume(level);
        server.send(200, "application/json", "{\"status\":\"success\", \"volume\":" + String(fmRadio->getVolume()) + "}");
        return;
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Thiếu tham số level (0-15)\"}");
}

// Thêm API xóa kênh
void AppWebServer::handleFmDeleteChannel()
{
    if (server.hasArg("index"))
    {
        int index = server.arg("index").toInt();
        fmRadio->deleteChannel(index);
        server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Đã xóa kênh index " + String(index) + "\"}");
        return;
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Thiếu tham số index\"}");
}