#include "FMRadio.h"

// =========================================================
// Constructor
// =========================================================
FMRadio::FMRadio(FileManager *fm)
    : fileManager(fm), currentFreq(99.5f), isPowered(false), rssi(0), currentVolume(10), numSavedChannels(0)
{
    // Constructor body (rx object initialized by default)
}

// =========================================================
// Initialization
// =========================================================
void FMRadio::begin()
{
    // 1. Load configuration from SD Card
    loadConfig();

    // 2. Initialize RDA5807 chip using library
    // Note: Wire.begin() is already called in setup(), so I2C bus is ready
    rx.setup();
    delay(100);

    // 4. Configure band and spacing
    rx.setBand(RDA5807_BAND);
    rx.setSpace(RDA5807_SPACE);

    // 5. Set volume
    rx.setVolume(currentVolume);
    rx.setMono(false);
    rx.setGpio(3,1);


    // 6. Wait for chip to stabilize
    delay(500);

    // 7. Set loaded frequency
    setFrequency(currentFreq);
    isPowered = true;
    Serial.println("FMRadio: RDA5807 chip initialized successfully.");
}

// =========================================================
// Frequency Control
// =========================================================
void FMRadio::setFrequency(float freq_mhz)
{
    // Convert MHz to library format (frequency in 10 kHz units)
    // Example: 99.5 MHz = 9950 in library format (99.5 * 100)
    uint16_t freq_code = (uint16_t)(freq_mhz * 100);

    rx.setFrequency(freq_code);
    currentFreq = freq_mhz;
    saveConfig();
    Serial.printf("FMRadio: Frequency set to %.1f MHz\n", freq_mhz);
}

// =========================================================
// Seek/Tuning
// =========================================================
void FMRadio::seekUp()
{
    Serial.println("FMRadio: Seeking up...");
    // RDA5807 library seek function
    // RDA_SEEK_WRAP: wrap around at band edges
    // RDA_SEEK_UP: seek upward
    rx.seek(RDA_SEEK_WRAP, RDA_SEEK_UP);
    // Get the new frequency from chip (in 10 kHz units)
    uint16_t freq_code = rx.getRealFrequency();
    currentFreq = freq_code / 100.0f;
    Serial.printf("FMRadio: Seek up complete. New frequency: %.1f MHz\n", currentFreq);
}

void FMRadio::seekDown()
{
    Serial.println("FMRadio: Seeking down...");
    rx.seek(RDA_SEEK_WRAP, RDA_SEEK_DOWN);
    uint16_t freq_code = rx.getRealFrequency();
    currentFreq = freq_code / 100.0f;
    Serial.printf("FMRadio: Seek down complete. New frequency: %.1f MHz\n", currentFreq);
}

float FMRadio::autoSeekNext()
{
    seekUp();
    Serial.printf("FMRadio: Auto seek completed. New frequency: %.1f MHz\n", currentFreq);
    return currentFreq;
}

// =========================================================
// Stereo/Mono Control
// =========================================================
void FMRadio::setStereo(bool enable)
{
    rx.setMono(!enable); // setMono(true) = mono, setMono(false) = stereo
    Serial.printf("FMRadio: Stereo mode set to %s\n", enable ? "ON" : "OFF");
}

// =========================================================
// Power Management
// =========================================================
void FMRadio::powerOn()
{
    // RDA5807 library handles power internally
    // If needed, you can enable specific features
    Serial.println("FMRadio: Power ON");
    isPowered = true;
}

void FMRadio::powerOff()
{
    // Disable receiver or put into low power mode
    rx.powerDown();
    Serial.println("FMRadio: Power OFF");
    isPowered = false;
}

// =========================================================
// Volume Control
// =========================================================
void FMRadio::setVolume(uint8_t volume)
{
    if (volume > 15)
        volume = 15;
    if (volume == currentVolume)
        return;

    currentVolume = volume;
    rx.setVolume(volume);
    saveConfig(); // Save volume to SD card
    Serial.printf("FMRadio: Volume set to %d\n", currentVolume);
}

// =========================================================
// Configuration Management
// =========================================================
void FMRadio::loadConfig()
{
    JsonDocument doc;

    // Try to load fm.json from SD Card
    if (fileManager->loadJsonFile(FM_CONFIG_FILE, &doc))
    {
        // 1. Load volume
        currentVolume = doc["volume"] | 10;
        if (currentVolume > 15)
            currentVolume = 15;

        // 2. Load current frequency
        currentFreq = doc["current_freq"] | 99.5f;

        // 3. Load saved channels
        JsonArray channels = doc["channels"].as<JsonArray>();
        numSavedChannels = 0;

        for (JsonObject channel : channels)
        {
            if (numSavedChannels < MAX_CHANNELS)
            {
                float freq = channel["freq"] | 0.0f;
                if (freq >= 87.0f && freq <= 108.0f)
                {
                    savedChannels[numSavedChannels] = freq;
                    numSavedChannels++;
                }
            }
        }
        Serial.printf("FMRadio: Config loaded. Vol: %d, Channels: %d\n", currentVolume, numSavedChannels);
    }
    else
    {
        // Initialize defaults if load fails
        Serial.println("FMRadio: Config not found. Using defaults.");
        currentVolume = 10;
        currentFreq = 99.5f;
        numSavedChannels = 0;
        saveConfig();
    }
}

void FMRadio::saveConfig()
{
    JsonDocument doc;

    doc["volume"] = currentVolume;
    doc["current_freq"] = currentFreq;

    JsonArray channels = doc["channels"].as<JsonArray>();
    for (int i = 0; i < numSavedChannels; i++)
    {
        JsonObject channel = channels.add<JsonObject>();
        channel["freq"] = savedChannels[i];
    }

    if (fileManager->saveJsonFile(FM_CONFIG_FILE, doc))
    {
        Serial.println("FMRadio: Config saved successfully.");
    }
    else
    {
        Serial.println("FMRadio: Failed to save config.");
    }
}

// =========================================================
// Status & Information
// =========================================================
void FMRadio::updateStatus()
{
    if (!isPowered)
        return;

    // Get RSSI (signal strength) from chip
    rssi = rx.getRssi(); // 0-63 scale
}

void FMRadio::getStatus(JsonDocument *doc)
{
    if (!isPowered)
    {
        (*doc)["error"] = "Chip is powered off.";
        return;
    }

    updateStatus();
    (*doc)["freq"] = currentFreq;
    (*doc)["rssi"] = rssi;
    (*doc)["stereo"] = rx.isStereo(); // Use isStereo() instead of getStereoIndicator()
    (*doc)["isPowered"] = isPowered;
    (*doc)["volume"] = currentVolume;
}

// =========================================================
// Channel Management
// =========================================================
void FMRadio::saveChannel(float freq_mhz)
{
    if (numSavedChannels >= MAX_CHANNELS)
    {
        Serial.println("FMRadio: Channel limit reached.");
        return;
    }

    savedChannels[numSavedChannels] = freq_mhz;
    numSavedChannels++;

    Serial.printf("FMRadio: Channel saved - %.1f MHz at index %d\n", freq_mhz, numSavedChannels - 1);
    saveConfig();
}

void FMRadio::selectSavedChannel(uint8_t index)
{
    if (index >= numSavedChannels)
    {
        Serial.println("FMRadio: Invalid channel index.");
        return;
    }

    float savedFreq = savedChannels[index];
    Serial.printf("FMRadio: Selecting channel at index %d: %.1f MHz\n", index, savedFreq);
    setFrequency(savedFreq);
    saveConfig();
}

void FMRadio::getSavedChannels(JsonDocument *doc)
{
    JsonArray channels = (*doc)["channels"].as<JsonArray>();

    for (int i = 0; i < numSavedChannels; i++)
    {
        JsonObject channel = channels.add<JsonObject>();
        channel["index"] = i;
        channel["freq"] = savedChannels[i];
    }
}

void FMRadio::deleteChannel(uint8_t index)
{
    if (index >= numSavedChannels)
    {
        Serial.println("FMRadio: Invalid channel index to delete.");
        return;
    }

    // Shift remaining channels
    for (int i = index; i < numSavedChannels - 1; i++)
    {
        savedChannels[i] = savedChannels[i + 1];
    }

    numSavedChannels--;
    Serial.printf("FMRadio: Channel deleted. Remaining: %d\n", numSavedChannels);
    saveConfig();
}
