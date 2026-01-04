#include "PowerManager.h"

// Constructor
PowerManager::PowerManager() : currentVolume(50)
{
}

// =========================================================
// Khởi tạo (Setup Pins)
// =========================================================

void PowerManager::begin()
{
    // Thiết lập Pin ADC để đọc điện áp Pin
    // pinMode(BATTERY_ADC_PIN, INPUT);

    // Thiết lập Pin ADC để đọc biến trở
    // pinMode(VOLUME_POT_ADC_PIN, INPUT);

    // Thiết lập Pin điều khiển Âm lượng (PWM Output)
    // pinMode(VOLUME_CONTROL_PIN, OUTPUT);

    // Thiết lập âm lượng ban đầu (ví dụ: mức 50%)
    setVolume(currentVolume);

    Serial.println("PowerManager: Khởi tạo hoàn tất cho pin 3S.");
}

// =========================================================
// 1. Quản lý Pin (Battery Management)
// =========================================================

float PowerManager::mapAdcToVoltage(int raw_adc)
{
    const float VREF = 3.3f;
    const float ADC_MAX = 4095.0f;

    // Giả định tỷ lệ mạch chia áp 4:1 cho khối pin 12.6V
    const float VOLTAGE_MULTIPLIER = 4.0f;

    return (float)raw_adc / ADC_MAX * VREF * VOLTAGE_MULTIPLIER;
}

float PowerManager::getBatteryVoltage()
{
    // int raw_adc = analogRead(BATTERY_ADC_PIN);
    // GIẢ LẬP: Giá trị ADC tương ứng với dải 9V - 12.6V
    int raw_adc = random(2792, 3915);
    return mapAdcToVoltage(raw_adc);
}

int PowerManager::getBatteryLevel()
{
    float voltage = getBatteryVoltage();

    if (voltage >= VOLTAGE_MAX)
        return 100;
    if (voltage <= VOLTAGE_MIN)
        return 0;

    int percentage = (int)((voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN) * 100.0f);

    return percentage;
}

// =========================================================
// 2. Quản lý Âm lượng (Volume Control)
// =========================================================

void PowerManager::setVolume(int level)
{
    // Giới hạn mức âm lượng trong khoảng 0-100
    if (level < 0)
        level = 0;
    if (level > 100)
        level = 100;

    currentVolume = level;

    // Ánh xạ dải 0-100 sang dải PWM 0-255 để điều khiển TPA3110 (Giả định PWM)
    int pwm_duty = map(level, 0, 100, 0, 255);

    // analogWrite(VOLUME_CONTROL_PIN, pwm_duty);

    Serial.printf("PowerManager: Đặt âm lượng thành %d (PWM: %d)\n", currentVolume, pwm_duty);
}

// =========================================================
// 3. Đọc biến trở (Potentiometer Read)
// =========================================================

int PowerManager::readPotentiometer()
{
    // 1. Đọc giá trị ADC thô (0-4095)
    // int raw_adc = analogRead(VOLUME_POT_ADC_PIN);

    // GIẢ LẬP: Giá trị ngẫu nhiên
    int raw_adc = random(0, 4095);

    // 2. Ánh xạ từ 0-4095 sang 0-100
    int volume_percent = map(raw_adc, 0, 4095, 0, 100);

    // Cập nhật currentVolume để phản ánh vị trí biến trở
    currentVolume = volume_percent;

    return volume_percent;
}

// =========================================================
// 4. Quản lý Nguồn (Power Management)
// =========================================================

void PowerManager::shutdown()
{
    Serial.println("PowerManager: Đang chuyển sang chế độ Deep Sleep/Tắt nguồn...");
    // esp_deep_sleep_start();
    delay(100);
    Serial.println("Hệ thống đã ngừng.");
}