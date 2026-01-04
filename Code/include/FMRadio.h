#ifndef FMRADIO_H
#define FMRADIO_H

#include <Arduino.h>
#include <Wire.h>          // I2C library
#include <ArduinoJson.h>   // JSON support
#include <RDA5807.h>       // PU2CLR RDA5807 library
#include "FileManager.h"

#define FM_CONFIG_FILE "/config/fm.json" 
#define MAX_CHANNELS 10    // Maximum number of saved channels

// RDA5807 library configuration
// Band options: 0=FM World (87-108MHz), 1=Japan wide (76-91MHz), 2=World wide (76-108MHz), 3=Special (65-76MHz or 50-65MHz)
#define RDA5807_BAND 0       // FM World band (87-108 MHz)
// Space options: 0=100kHz, 1=200kHz, 2=50kHz, 3=25kHz
#define RDA5807_SPACE 0      // 100 kHz channel spacing

class FMRadio {
public:
    // Constructor
    FMRadio(FileManager* fm);

    // Initialize I2C and RDA5807 chip
    void begin();
    
    // Set frequency in MHz (e.g., 99.5 for 99.5 MHz)
    void setFrequency(float freq_mhz);
    
    // Auto seek - returns new frequency
    float autoSeekNext();

    // Hardware seek up/down
    void seekUp();
    void seekDown();

    // Stereo/Mono control
    void setStereo(bool enable);

    // Power management
    void powerOff();
    void powerOn();
    
    // Volume control (0-15)
    void setVolume(uint8_t volume);
    uint8_t getVolume() const { return currentVolume; }

    // Save configuration to SD card
    void saveConfig();
    // Channel management
    void saveChannel(float freq_mhz);                
    void selectSavedChannel(uint8_t index);         
    void getSavedChannels(JsonDocument* doc);       
    void deleteChannel(uint8_t index);

    // Get receiver status (for WebServer)
    void getStatus(JsonDocument* doc);

    // Get current frequency
    float getCurrentFrequency() const { return currentFreq; }

private:
    RDA5807 rx;                         // RDA5807 receiver from library
    FileManager* fileManager;           // Reference to FileManager
    float currentFreq;                  // Current frequency in MHz
    bool isPowered;                     // Power state
    int rssi;                           // Signal strength (RSSI)
    uint8_t currentVolume;              // Current volume (0-15)
    float savedChannels[MAX_CHANNELS];  // Saved channel frequencies
    uint8_t numSavedChannels;           // Number of saved channels

    // Helper functions
    void loadConfig();       // Load volume and channels from SD card
    void updateStatus();     // Update RSSI from chip
};

#endif // FMRADIO_H