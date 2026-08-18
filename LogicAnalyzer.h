/**
 * LogicAnalyzer.h - SUMP Protocol Handler Header
 * 
 * This header defines the main LogicAnalyzer class which handles:
 * - SUMP protocol command parsing and response
 * - Capture state management
 * - Configuration management
 * - Data compression and streaming
 * 
 * SUMP Protocol Reference:
 * - Open-source protocol used by PulseView/sigrok
 * - Supports 8/16-bit parallel capture
 * - Command set: configuration, arm, read data, metadata
 */

#ifndef LOGIC_ANALYZER_H
#define LOGIC_ANALYZER_H

#include <Arduino.h>
#include <cstdint>

// Forward declarations
class LogicAnalyzerLCDCAM;
class LogicAnalyzerRLE;

// ========================================
// SUMP Protocol Constants
// ========================================

/** SUMP protocol magic bytes for device identification */
#define SUMP_MAGIC_BYTE_0 0x5A  // 'Z'
#define SUMP_MAGIC_BYTE_1 0x4C  // 'L'
#define SUMP_MAGIC_BYTE_2 0x50  // 'P'
#define SUMP_MAGIC_BYTE_3 0x32  // '2'

/** SUMP command opcodes */
#define SUMP_CMD_RESET           0x00  // Reset device
#define SUMP_CMD_ARM             0x01  // Arm capture
#define SUMP_CMD_ID              0x02  // Request device ID
#define SUMP_CMD_META            0x04  // Request metadata
#define SUMP_CMD_READ            0x80  // Read captured data
#define SUMP_CMD_CONFIG          0x81  // Extended configuration commands

/** Extended configuration commands */
#define SUMP_EXTENDED_DIVIDER    0x80  // Set clock divider
#define SUMP_EXTENDED_COUNT      0x81  // Set sample count
#define SUMP_EXTENDED_FLAGS      0x82  // Set flags
#define SUMP_EXTENDED_TRIGGER    0xC0  // Set trigger (0xC0-0xC3)

// ========================================
// Capture State Enumeration
// ========================================

enum class CaptureState : uint8_t {
  IDLE = 0,        // Waiting for configuration
  ARMED = 1,       // Configured and waiting for trigger
  CAPTURING = 2,   // Actively capturing data
  DONE = 3,        // Capture complete, data ready
  ERROR = 4        // Error occurred
};

// ========================================
// Trigger Configuration
// ========================================

struct TriggerConfig {
  uint16_t channel_mask;    // Which channels trigger on
  uint16_t channel_value;   // Expected value on trigger
  uint8_t edge_type;        // 0=any, 1=rising, 2=falling
  uint16_t delay_samples;   // Samples to capture before trigger
  
  TriggerConfig() : channel_mask(0), channel_value(0), 
                   edge_type(0), delay_samples(0) {}
};

// ========================================
// Main LogicAnalyzer Class
// ========================================

class LogicAnalyzer {
private:
  // State management
  CaptureState state;
  unsigned long capture_start_time;
  
  // Configuration parameters
  uint32_t sample_count;           // Number of samples to capture
  uint32_t clock_divider;          // Clock divider (1-65535)
  uint32_t read_delay_count;       // Pre-capture delay samples
  uint8_t capture_mode;            // 0=8bit, 1=16bit
  bool rle_enabled;                // RLE compression enabled
  uint16_t channel_mask;           // Active channels
  
  // Trigger configuration
  TriggerConfig trigger_config;
  
  // Buffers
  uint8_t* capture_buffer;         // Raw capture data
  uint8_t* compressed_buffer;      // RLE compressed data
  uint32_t buffer_write_pos;       // Current write position
  uint32_t compressed_size;        // Size of compressed data
  
  // Hardware drivers
  LogicAnalyzerLCDCAM* lcd_cam_driver;
  LogicAnalyzerRLE* rle_compressor;
  
  // Private methods
  bool validateConfiguration();
  bool startCapture();
  void stopCapture();
  bool readDataChunk(uint8_t* dest, uint32_t offset, uint32_t length);
  
public:
  // Constructor/Destructor
  LogicAnalyzer();
  ~LogicAnalyzer();
  
  // Initialization
  bool initialize();
  void shutdown();
  
  // SUMP Protocol handlers
  bool handleCommand(uint8_t cmd);
  void handleConfigCommand(uint32_t param);
  void handleExtendedCommand(uint8_t ext_cmd, uint32_t param);
  void handleTriggerCommand(uint8_t trig_idx, uint32_t param);
  
  // Command implementations
  void resetDevice();
  void sendDeviceID();
  void sendMetadata();
  void armCapture();
  void sendCapturedData();
  
  // State accessors
  CaptureState getState() const { return state; }
  bool isCapturing() const { return state == CaptureState::CAPTURING; }
  bool isArmed() const { return state == CaptureState::ARMED; }
  bool hasData() const { return state == CaptureState::DONE; }
  
  // Capture control
  bool beginCapture();
  void abortCapture();
  
  // Configuration
  void setClockDivider(uint32_t divider);
  void setSampleCount(uint32_t count);
  void setReadDelayCount(uint32_t count);
  void setCaptureMode(uint8_t mode);
  void setChannelMask(uint16_t mask);
  void setRLEMode(bool enabled);
  void setTrigger(const TriggerConfig& config);
  
  // Status/Debug
  uint32_t getSampleCount() const { return sample_count; }
  uint32_t getCompressedSize() const { return compressed_size; }
  unsigned long getCaptureStartTime() const { return capture_start_time; }
  uint32_t getCaptureElapsedTime() const;
  
  // Memory management
  uint32_t getBufferUsage() const;
  uint32_t getFreeMemory() const;
};

// ========================================
// Inline Helper Functions
// ========================================

/**
 * Check if clock divider is valid for 16-bit capture
 */
inline bool isValidClockDivider(uint32_t divider) {
  return (divider >= 1 && divider <= 65535);
}

/**
 * Calculate actual sampling frequency from divider
 * Base frequency: 40 MHz (LCD_CAM base clock)
 */
inline uint32_t calculateSamplingFrequency(uint32_t divider) {
  return 40000000 / divider;  // Hz
}

/**
 * Convert sampling frequency to clock divider
 */
inline uint32_t frequencyToClockDivider(uint32_t frequency_hz) {
  if (frequency_hz == 0) return 1;
  uint32_t divider = 40000000 / frequency_hz;
  if (divider < 1) return 1;
  if (divider > 65535) return 65535;
  return divider;
}

#endif // LOGIC_ANALYZER_H
