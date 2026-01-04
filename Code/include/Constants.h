#ifndef CONSTANTS_H
#define CONSTANTS_H

// =========================================================
// 1. Cấu hình SD Card (SPI Pins)
// =========================================================
#define SPI_SCK_PIN 18  // Serial Clock (SCK)
#define SPI_MISO_PIN 19 // Master In Slave Out (MISO)
#define SPI_MOSI_PIN 23 // Master Out Slave In (MOSI)
#define SD_CS_PIN 5     // Chip Select (CS)

// =========================================================
// 2. Quản lý Nguồn (PowerManager)
// =========================================================
// Pin cho Pin Lithium ADC (Đọc điện áp pin 3S)
#define BATTERY_ADC_PIN 34

// Pin ADC để đọc Biến trở điều chỉnh Âm lượng
#define VOLUME_POT_ADC_PIN 35

// Pin điều khiển Âm lượng (PWM/DAC Output, đã đổi từ 18 sang 25)
#define VOLUME_CONTROL_PIN 25

// =========================================================
// 3. Cấu hình Wi-Fi Mặc định (WebServer)
// =========================================================
#define DEFAULT_WIFI_SSID "ESP32_Famio_AP"
#define DEFAULT_WIFI_PASS "12345678"
#define MDNS_HOSTNAME "famio"

#define STA_SSID_CONFIG_KEY "sta_ssid"
#define STA_PWD_CONFIG_KEY "sta_password"

#define AP_SSID_CONFIG_KEY "ap_ssid"
#define AP_PWD_CONFIG_KEY "ap_password"

#define CONNECTION_TIMEOUT_S 30

// Thư mục gốc chứa tất cả dữ liệu dự án trên SD Card
#define PROJECT_ROOT_DIR "/famio"
#define CONFIG_FILE_PATH "/config" // Đường dẫn file config Wi-Fi trên SD Card
#define UI_PATH "/ui"
#define WIFI_CONFIG_FILE "/wifi.json"
#define COMMON_CONFIG_FILE "/common.json"

#endif // CONSTANTS_H