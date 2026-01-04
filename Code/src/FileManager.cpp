#include "FileManager.h"
#include "Constants.h"

// =========================================================
// Hàm Helper: Nối đường dẫn thư mục gốc
// =========================================================
// Hàm này đảm bảo mọi đường dẫn đều bắt đầu bằng /famio/
String getFullPath(const char *path)
{
    // Tránh nối đôi dấu '/', ví dụ: /famio//index.html
    if (path[0] == '/')
    {
        return String(PROJECT_ROOT_DIR) + String(path);
    }
    return String(PROJECT_ROOT_DIR) + "/" + String(path);
}

// =========================================================
// Khởi tạo SD Card
// =========================================================

bool FileManager::begin()
{
    Serial.print("Đang khởi tạo SD Card...");
    // Khởi tạo với Pin CS được định nghĩa (SD_CS_PIN)
    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println("Lỗi: Khởi tạo SD Card thất bại.");
        sd_initialized = false;
        return false;
    }

    // Kiểm tra loại thẻ
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("Lỗi: Không tìm thấy thẻ SD.");
        sd_initialized = false;
        return false;
    }

    Serial.printf("Thành công! Loại thẻ: %d\n", cardType);
    Serial.printf("Kích thước thẻ: %.2f GB\n", SD.cardSize() / (1024.0 * 1024.0 * 1024.0));
    sd_initialized = true;
    return true;
}

// =========================================================
// Đọc file JSON
// =========================================================

bool FileManager::loadJsonFile(const char *path, JsonDocument *doc)
{
    if (!sd_initialized)
    {
        Serial.println("Lỗi: SD Card chưa được khởi tạo.");
        return false;
    }

    // SỬ DỤNG HÀM HELPER ĐỂ CÓ ĐƯỜNG DẪN ĐẦY ĐỦ: /famio/config.json
    String fullPath = getFullPath(path);

    File file = SD.open(fullPath.c_str());
    if (!file)
    {
        Serial.printf("Lỗi: Không thể mở file JSON: %s\n", fullPath.c_str());
        return false;
    }

    // ... (Phần deserializeJson và xử lý lỗi giữ nguyên) ...
    DeserializationError error = deserializeJson(*doc, file);
    file.close();

    if (error)
    {
        Serial.printf("Lỗi giải mã JSON (%s) trong file: %s\n", error.c_str(), fullPath.c_str());
        doc->clear();
        return false;
    }

    return true;
}

// =========================================================
// Lưu file JSON
// =========================================================

bool FileManager::saveJsonFile(const char *path, const JsonDocument &doc)
{
    if (!sd_initialized)
    {
        Serial.println("Lỗi: SD Card chưa được khởi tạo.");
        return false;
    }

    // SỬ DỤNG HÀM HELPER ĐỂ CÓ ĐƯỜNG DẪN ĐẦY ĐỦ: /famio/ui/config
    String fullPath = getFullPath(path);

    File file = SD.open(fullPath.c_str(), FILE_WRITE);
    if (!file)
    {
        Serial.printf("Lỗi: Không thể mở file để ghi: %s\n", fullPath.c_str());
        return false;
    }

    // ... (Phần serializeJson và xử lý lỗi giữ nguyên) ...

    if (serializeJson(doc, file) == 0)
    {
        Serial.printf("Lỗi: Ghi file JSON thất bại: %s\n", fullPath.c_str());
        file.close();
        return false;
    }

    file.close();
    return true;
}

// =========================================================
// Mở file tĩnh (Phục vụ Web Server)
// =========================================================

File FileManager::openFile(const char *path)
{
    if (!sd_initialized)
    {
        return File();
    }

    // SỬ DỤNG HÀM HELPER ĐỂ CÓ ĐƯỜNG DẪN ĐẦY ĐỦ: /famio/index.html
    String fullPath = getFullPath(path);

    return SD.open(fullPath.c_str());
}