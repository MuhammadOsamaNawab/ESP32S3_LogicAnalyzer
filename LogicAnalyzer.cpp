/**
 * LogicAnalyzer.cpp - SUMP Protocol Handler Implementation
 * 
 * Implements the main LogicAnalyzer class which handles SUMP protocol
 * communication, capture state management, and data coordination.
 */

#include "LogicAnalyzer.h"
#include "LogicAnalyzer_LCD_CAM.h"
#include "LogicAnalyzer_RLE.h"
#include "LogicAnalyzerConfig.h"

// ========================================
// Constructor
// ========================================

LogicAnalyzer::LogicAnalyzer()
  : state(CaptureState::IDLE),
    capture_start_time(0),
    sample_count(DEFAULT_SAMPLE_COUNT),
    clock_divider(DEFAULT_CLOCK_DIVIDER),
    read_delay_count(DEFAULT_READ_DELAY_COUNT),
    capture_mode(FEATURE_16BIT_MODE ? 1 : 0),
    rle_enabled(RLE_ENABLED_BY_DEFAULT),
    channel_mask(0xFFFF),
    capture_buffer(nullptr),
    compressed_buffer(nullptr),
    buffer_write_pos(0),
    compressed_size(0),
    lcd_cam_driver(nullptr),
    rle_compressor(nullptr) {
  
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] LogicAnalyzer constructor called");
  #endif
}

// ========================================
// Destructor
// ========================================

LogicAnalyzer::~LogicAnalyzer() {
  shutdown();
}

// ========================================
// Initialization
// ========================================

bool LogicAnalyzer::initialize() {
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Initializing LogicAnalyzer...");
  #endif

  // Allocate capture buffer
  capture_buffer = (uint8_t*)malloc(CAPTURE_SIZE * 2);  // 2 bytes per sample (16-bit)
  if (!capture_buffer) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to allocate capture buffer");
    #endif
    return false;
  }

  // Allocate compressed buffer
  compressed_buffer = (uint8_t*)malloc(RLE_BUFFER_SIZE * 2);
  if (!compressed_buffer) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to allocate compressed buffer");
    #endif
    free(capture_buffer);
    capture_buffer = nullptr;
    return false;
  }

  // Initialize LCD_CAM driver
  // NOTE: Temporarily disabled due to GPIO pin configuration issues
  // Device will still handle SUMP commands but won't capture actual data
  // TODO: Fix GPIO pin configuration for LCD_CAM hardware
  /*
  lcd_cam_driver = new LogicAnalyzerLCDCAM();
  if (!lcd_cam_driver || !lcd_cam_driver->initialize()) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to initialize LCD_CAM driver");
    #endif
    return false;
  }
  */
  lcd_cam_driver = nullptr;  // Stub for now

  // Initialize RLE compressor
  rle_compressor = new LogicAnalyzerRLE();
  if (!rle_compressor) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to initialize RLE compressor");
    #endif
    return false;
  }

  state = CaptureState::IDLE;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Capture buffer: ");
    Serial.print(CAPTURE_SIZE * 2);
    Serial.println(" bytes allocated");
    Serial.print("[DEBUG] Compressed buffer: ");
    Serial.print(RLE_BUFFER_SIZE * 2);
    Serial.println(" bytes allocated");
  #endif

  return true;
}

void LogicAnalyzer::shutdown() {
  if (state == CaptureState::CAPTURING) {
    abortCapture();
  }

  if (lcd_cam_driver) {
    delete lcd_cam_driver;
    lcd_cam_driver = nullptr;
  }

  if (rle_compressor) {
    delete rle_compressor;
    rle_compressor = nullptr;
  }

  if (capture_buffer) {
    free(capture_buffer);
    capture_buffer = nullptr;
  }

  if (compressed_buffer) {
    free(compressed_buffer);
    compressed_buffer = nullptr;
  }

  state = CaptureState::IDLE;

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] LogicAnalyzer shutdown complete");
  #endif
}

// ========================================
// SUMP Protocol Command Handlers
// ========================================

bool LogicAnalyzer::handleCommand(uint8_t cmd) {
  // NOTE: NO DEBUG OUTPUT HERE - keeps SUMP binary protocol clean!
  switch (cmd) {
    case SUMP_CMD_RESET:
      resetDevice();
      return true;

    case SUMP_CMD_ID:
      sendDeviceID();
      return true;

    case SUMP_CMD_META:
      sendMetadata();
      return true;

    case SUMP_CMD_ARM:
      armCapture();
      return true;

    case SUMP_CMD_READ:
      sendCapturedData();
      return true;

    case SUMP_CMD_CONFIG:
      // Extended command - requires 4 more bytes
      // Read parameter from serial (handled in main loop with buffering)
      return true;

    default:
      return false;
  }
}

void LogicAnalyzer::handleConfigCommand(uint32_t param) {
  // NOTE: NO DEBUG OUTPUT - keep protocol clean!

  uint8_t cmd = (param >> 24) & 0xFF;
  uint32_t value = param & 0xFFFFFF;

  switch (cmd) {
    case SUMP_EXTENDED_DIVIDER:
      setClockDivider(value);
      break;

    case SUMP_EXTENDED_COUNT:
      setSampleCount(value & 0xFFFF);  // Max 16-bit sample count
      break;

    case SUMP_EXTENDED_FLAGS:
      setRLEMode((value & 0x04) != 0);
      setCaptureMode((value & 0x01) ? 1 : 0);
      setChannelMask((value >> 16) & 0xFFFF);
      break;

    case SUMP_EXTENDED_TRIGGER:
      // Trigger configuration (not fully implemented yet)
      #if TRIGGER_ENABLED
      // TODO: Parse trigger data
      #endif
      break;

    default:
      #if _DEBUG_MODE_
        Serial.print("[WARNING] Unknown extended command: 0x");
        Serial.println(cmd, HEX);
      #endif
      break;
  }
}

// ========================================
// SUMP Command Implementations
// ========================================

void LogicAnalyzer::resetDevice() {
  // NOTE: NO DEBUG OUTPUT - keep protocol clean!
  state = CaptureState::IDLE;
  sample_count = DEFAULT_SAMPLE_COUNT;
  clock_divider = DEFAULT_CLOCK_DIVIDER;
  read_delay_count = DEFAULT_READ_DELAY_COUNT;
  capture_mode = FEATURE_16BIT_MODE ? 1 : 0;
  rle_enabled = RLE_ENABLED_BY_DEFAULT;
  channel_mask = 0xFFFF;
  buffer_write_pos = 0;
  compressed_size = 0;

  // Send acknowledgement
  OLS_PORT.write(0xAC);  // ACK
}

void LogicAnalyzer::sendDeviceID() {
  // NOTE: NO DEBUG OUTPUT - keep binary protocol clean!
  // Send device ID (4 bytes)
  OLS_PORT.write(SUMP_MAGIC_BYTE_0);  // 0x5A 'Z'
  OLS_PORT.write(SUMP_MAGIC_BYTE_1);  // 0x4C 'L'
  OLS_PORT.write(SUMP_MAGIC_BYTE_2);  // 0x50 'P'
  OLS_PORT.write(SUMP_MAGIC_BYTE_3);  // 0x32 '2'
}

void LogicAnalyzer::sendMetadata() {
  // NOTE: NO DEBUG OUTPUT - keep binary protocol clean!

  #if FEATURE_METADATA
  // Send metadata according to SUMP specification
  
  // Protocol version (token 0x01)
  OLS_PORT.write(0x01);
  OLS_PORT.write(0x01);  // Protocol version 1

  // Device name (token 0x02)
  const char* device_name = "ESP32S3-LA";
  uint8_t name_len = 10;  // strlen("ESP32S3-LA")
  OLS_PORT.write(0x02);
  OLS_PORT.write(name_len);
  OLS_PORT.write((const uint8_t*)device_name, name_len);

  // Max sample rate (token 0x20)
  OLS_PORT.write(0x20);
  uint32_t max_rate = 40000000;  // 40 MHz max
  OLS_PORT.write((max_rate >> 24) & 0xFF);
  OLS_PORT.write((max_rate >> 16) & 0xFF);
  OLS_PORT.write((max_rate >> 8) & 0xFF);
  OLS_PORT.write(max_rate & 0xFF);

  // Number of probes (token 0x21)
  OLS_PORT.write(0x21);
  OLS_PORT.write(16);    // 16 channels

  // Sample memory depth (token 0x22)
  OLS_PORT.write(0x22);
  uint32_t mem_depth = CAPTURE_SIZE;
  OLS_PORT.write((mem_depth >> 24) & 0xFF);
  OLS_PORT.write((mem_depth >> 16) & 0xFF);
  OLS_PORT.write((mem_depth >> 8) & 0xFF);
  OLS_PORT.write(mem_depth & 0xFF);

  // Dynamic flags (token 0x23)
  OLS_PORT.write(0x23);
  OLS_PORT.write(0x00);

  // End of metadata
  OLS_PORT.write(0x00);
  #endif
}

void LogicAnalyzer::armCapture() {
  if (!validateConfiguration()) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Invalid capture configuration");
    #endif
    return;
  }

  // Configure LCD_CAM for capture
  if (!lcd_cam_driver->configure(clock_divider, capture_mode, channel_mask)) {
    state = CaptureState::ERROR;
    return;
  }

  state = CaptureState::ARMED;
  // NOTE: NO DEBUG OUTPUT - keep protocol clean!
}

void LogicAnalyzer::sendCapturedData() {
  // NOTE: NO DEBUG OUTPUT - keep protocol clean!

  if (state != CaptureState::DONE) {
    // No data available - silently return
    return;
  }

  // Send data in chunks to avoid buffer overflow
  uint32_t remaining = compressed_size;
  uint32_t offset = 0;
  const uint32_t CHUNK_SIZE = 256;

  while (remaining > 0) {
    uint32_t chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
    OLS_PORT.write(&compressed_buffer[offset], chunk);
    offset += chunk;
    remaining -= chunk;
    delay(1);  // Small delay between chunks
  }

  // Reset state after data is sent
  state = CaptureState::IDLE;
}

// ========================================
// Capture Control
// ========================================

bool LogicAnalyzer::beginCapture() {
  if (!validateConfiguration()) {
    return false;
  }

  buffer_write_pos = 0;
  compressed_size = 0;
  capture_start_time = millis();

  if (!lcd_cam_driver->startCapture(capture_buffer, CAPTURE_SIZE)) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to start LCD_CAM capture");
    #endif
    return false;
  }

  state = CaptureState::CAPTURING;

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Capture started");
  #endif

  return true;
}

void LogicAnalyzer::abortCapture() {
  if (state != CaptureState::CAPTURING) {
    return;
  }

  if (lcd_cam_driver) {
    lcd_cam_driver->stopCapture();
  }

  state = CaptureState::IDLE;

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Capture aborted");
  #endif
}

// ========================================
// Configuration Methods
// ========================================

bool LogicAnalyzer::validateConfiguration() {
  if (!isValidClockDivider(clock_divider)) {
    return false;
  }

  if (sample_count == 0 || sample_count > CAPTURE_SIZE) {
    return false;
  }

  return true;
}

void LogicAnalyzer::setClockDivider(uint32_t divider) {
  if (isValidClockDivider(divider)) {
    clock_divider = divider;
  }
}

void LogicAnalyzer::setSampleCount(uint32_t count) {
  if (count > 0 && count <= CAPTURE_SIZE) {
    sample_count = count;
  }
}

void LogicAnalyzer::setReadDelayCount(uint32_t count) {
  read_delay_count = count;
}

void LogicAnalyzer::setCaptureMode(uint8_t mode) {
  if (mode <= 1) {
    capture_mode = mode;
  }
}

void LogicAnalyzer::setChannelMask(uint16_t mask) {
  channel_mask = mask;
}

void LogicAnalyzer::setRLEMode(bool enabled) {
  rle_enabled = enabled;
}

void LogicAnalyzer::setTrigger(const TriggerConfig& config) {
  trigger_config = config;
}

// ========================================
// Status Methods
// ========================================

uint32_t LogicAnalyzer::getCaptureElapsedTime() const {
  if (state == CaptureState::CAPTURING) {
    return millis() - capture_start_time;
  }
  return 0;
}

uint32_t LogicAnalyzer::getBufferUsage() const {
  if (!capture_buffer) return 0;
  return buffer_write_pos * 2;  // 2 bytes per sample
}

uint32_t LogicAnalyzer::getFreeMemory() const {
  return ESP.getFreeHeap();
}
