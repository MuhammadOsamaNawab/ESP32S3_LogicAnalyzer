/**
 * LogicAnalyzer_LCD_CAM.h - LCD_CAM Hardware Driver Header
 * 
 * Defines the interface for the LCD_CAM peripheral driver.
 * LCD_CAM is an ESP32-S3 specialized interface for parallel data capture.
 */

#ifndef LOGIC_ANALYZER_LCD_CAM_H
#define LOGIC_ANALYZER_LCD_CAM_H

#include <Arduino.h>
#include <cstdint>
#include <driver/periph_ctrl.h>
#include <driver/gpio.h>
#include <soc/lcd_cam_reg.h>
#include <soc/lcd_cam_struct.h>

// ========================================
// LCD_CAM Configuration Constants
// ========================================

/** Size of single DMA buffer for LCD_CAM */
#define LCD_CAM_DMA_BUFFER_SIZE 4096

/** Maximum number of DMA descriptors */
#define LCD_CAM_DMA_DESC_COUNT 4

// ========================================
// LCD_CAM Driver Class
// ========================================

class LogicAnalyzerLCDCAM {
private:
  // State
  bool initialized;
  bool capturing;
  
  // Buffer management
  uint8_t* data_buffer;
  uint32_t buffer_size;
  uint32_t write_pos;
  uint32_t sample_count;
  
  // Configuration
  uint16_t channel_mask;
  uint32_t clock_divider_value;
  
  // Private initialization methods
  bool configureGPIOPins();
  bool initializeLCDCAM();
  bool setupDMA();
  
public:
  // Constructor/Destructor
  LogicAnalyzerLCDCAM();
  ~LogicAnalyzerLCDCAM();
  
  // Public methods
  bool initialize();
  bool configure(uint32_t clock_div, uint8_t mode, uint16_t ch_mask);
  
  // Capture control
  bool startCapture(uint8_t* buffer, uint32_t size);
  bool stopCapture();
  
  // Clock control
  void setClockDivider(uint32_t divider);
  
  // Data access
  bool readData(uint8_t* dest, uint32_t length);
  uint32_t getAvailableData() const;
  void resetBuffer();
  
  // Status
  bool isCapturing() const;
  uint32_t getSampleCount() const;
  
  // Debug
  void printStatistics();
  void printConfiguration();
};

#endif // LOGIC_ANALYZER_LCD_CAM_H
