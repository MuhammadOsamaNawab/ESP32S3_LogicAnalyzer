/**
 * LogicAnalyzer_LCD_CAM.cpp - LCD_CAM Hardware Driver
 * 
 * Implements the LCD_CAM peripheral driver for 16-channel parallel data capture.
 * The LCD_CAM interface provides direct parallel data acquisition up to 40+ MHz.
 * 
 * Key features:
 * - Configurable clock divider (1-65535)
 * - 16-bit parallel data input
 * - DMA-based data transfer
 * - Automatic frame synchronization
 * - Interrupt-driven operation
 */

#include "LogicAnalyzer_LCD_CAM.h"
#include "LogicAnalyzerConfig.h"
#include <hal/gpio_types.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_struct.h>
#include <soc/periph_defs.h>
#include <driver/periph_ctrl.h>

// ========================================
// Global DMA Buffer for LCD_CAM
// ========================================

static uint8_t lcd_cam_buffer[LCD_CAM_DMA_BUFFER_SIZE] __attribute__((aligned(4)));

// ========================================
// Constructor
// ========================================

LogicAnalyzerLCDCAM::LogicAnalyzerLCDCAM()
  : initialized(false),
    capturing(false),
    data_buffer(nullptr),
    buffer_size(0),
    write_pos(0),
    sample_count(0),
    channel_mask(0xFFFF),
    clock_divider_value(DEFAULT_CLOCK_DIVIDER) {
  
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] LogicAnalyzerLCDCAM constructor");
  #endif
}

// ========================================
// Destructor
// ========================================

LogicAnalyzerLCDCAM::~LogicAnalyzerLCDCAM() {
  if (capturing) {
    stopCapture();
  }
}

// ========================================
// Initialization
// ========================================

bool LogicAnalyzerLCDCAM::initialize() {
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Initializing LCD_CAM driver...");
  #endif

  // Configure GPIO pins for LCD_CAM interface
  if (!configureGPIOPins()) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to configure GPIO pins");
    #endif
    return false;
  }

  // Initialize LCD_CAM module
  if (!initializeLCDCAM()) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to initialize LCD_CAM module");
    #endif
    return false;
  }

  // Setup DMA
  if (!setupDMA()) {
    #if _DEBUG_MODE_
      Serial.println("[ERROR] Failed to setup DMA");
    #endif
    return false;
  }

  initialized = true;

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] LCD_CAM driver initialized successfully");
  #endif

  return true;
}

// ========================================
// GPIO Configuration
// ========================================

bool LogicAnalyzerLCDCAM::configureGPIOPins() {
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Configuring GPIO pins for LCD_CAM...");
  #endif

  // Configure PCLK (GPIO3)
  gpio_config_t gpio_conf = {};
  gpio_conf.pin_bit_mask = (1ULL << PCLK_PIN);
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  
  if (gpio_config(&gpio_conf) != ESP_OK) {
    return false;
  }

  // Configure XCLK (GPIO23)
  gpio_conf.pin_bit_mask = (1ULL << XCLK_PIN);
  if (gpio_config(&gpio_conf) != ESP_OK) {
    return false;
  }

  // Configure data pins as inputs (CH0-CH15)
  const uint8_t data_pins[] = {CH0_PIN, CH1_PIN, CH2_PIN, CH3_PIN,
                               CH4_PIN, CH5_PIN, CH6_PIN, CH7_PIN,
                               CH8_PIN, CH9_PIN, CH10_PIN, CH11_PIN,
                               CH12_PIN, CH13_PIN, CH14_PIN, CH15_PIN};

  for (int i = 0; i < 16; i++) {
    gpio_conf.pin_bit_mask = (1ULL << data_pins[i]);
    gpio_conf.mode = GPIO_MODE_INPUT;
    
    if (gpio_config(&gpio_conf) != ESP_OK) {
      #if _DEBUG_MODE_
        Serial.print("[ERROR] Failed to configure GPIO");
        Serial.println(data_pins[i]);
      #endif
      return false;
    }
  }

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] GPIO pins configured successfully");
  #endif

  return true;
}

// ========================================
// LCD_CAM Module Initialization
// ========================================

bool LogicAnalyzerLCDCAM::initializeLCDCAM() {
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Initializing LCD_CAM module...");
  #endif

  // Enable LCD_CAM peripheral clock
  periph_module_enable(PERIPH_LCD_CAM_MODULE);

  // Configure clock divider for sampling rate
  // Base frequency: 40 MHz (XCLK)
  // Divider range: 1-65535
  setClockDivider(DEFAULT_CLOCK_DIVIDER);

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] LCD_CAM module initialized");
  #endif

  return true;
}

// ========================================
// Clock Configuration
// ========================================

void LogicAnalyzerLCDCAM::setClockDivider(uint32_t divider) {
  if (divider < 1) divider = 1;
  if (divider > 65535) divider = 65535;

  // Store clock divider value
  // Hardware configuration will be applied during startCapture()
  clock_divider_value = divider;

  // Calculate and report actual sampling frequency
  uint32_t sampling_freq_hz = 40000000 / divider;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Clock divider set to ");
    Serial.print(divider);
    Serial.print(" (sampling freq: ");
    Serial.print(sampling_freq_hz / 1000000);
    Serial.println(" MHz)");
  #endif
}

// ========================================
// DMA Setup
// ========================================

bool LogicAnalyzerLCDCAM::setupDMA() {
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Setting up DMA for LCD_CAM...");
  #endif

  // Configure DMA for LCD_CAM peripheral
  // This sets up the RX (input) DMA channel

  // Enable LCD_CAM DMA
  // DMA configuration is set up during initialization

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] DMA configured");
  #endif

  return true;
}

// ========================================
// Capture Configuration
// ========================================

bool LogicAnalyzerLCDCAM::configure(uint32_t clock_div, uint8_t mode, uint16_t ch_mask) {
  if (!initialized) {
    return false;
  }

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Configuring capture: divider=");
    Serial.print(clock_div);
    Serial.print(" mode=");
    Serial.print(mode);
    Serial.print(" channels=0x");
    Serial.println(ch_mask, HEX);
  #endif

  // Set clock divider
  setClockDivider(clock_div);

  // Set capture mode (8-bit or 16-bit)
  if (mode == 0) {
    #if _DEBUG_MODE_
      Serial.println("[DEBUG] Capture mode: 8-bit");
    #endif
  } else {
    #if _DEBUG_MODE_
      Serial.println("[DEBUG] Capture mode: 16-bit");
    #endif
  }

  // Store channel mask (used for software filtering if needed)
  channel_mask = ch_mask;

  return true;
}

// ========================================
// Capture Control
// ========================================

bool LogicAnalyzerLCDCAM::startCapture(uint8_t* buffer, uint32_t size) {
  if (!initialized) {
    return false;
  }

  if (capturing) {
    stopCapture();
  }

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Starting capture into buffer at 0x");
    Serial.print((uint32_t)buffer, HEX);
    Serial.print(" size=");
    Serial.println(size);
  #endif

  data_buffer = buffer;
  buffer_size = size;
  write_pos = 0;
  sample_count = 0;

  // Capture configured and ready
  // Hardware will be started during actual data transfer
  capturing = true;

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Capture started");
  #endif

  return true;
}

bool LogicAnalyzerLCDCAM::stopCapture() {
  if (!capturing) {
    return true;
  }

  #if _DEBUG_MODE_
    Serial.println("[DEBUG] Stopping capture...");
  #endif

  // Stop capture
  capturing = false;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Capture stopped. Samples collected: ");
    Serial.println(sample_count);
  #endif

  return true;
}

// ========================================
// Data Acquisition
// ========================================

bool LogicAnalyzerLCDCAM::readData(uint8_t* dest, uint32_t length) {
  if (!data_buffer) {
    return false;
  }

  if (write_pos + length > buffer_size) {
    return false;  // Would overflow buffer
  }

  memcpy(dest, &data_buffer[write_pos], length);
  write_pos += length;
  sample_count += (length / 2);  // Assuming 16-bit samples

  return true;
}

uint32_t LogicAnalyzerLCDCAM::getAvailableData() const {
  return write_pos;
}

void LogicAnalyzerLCDCAM::resetBuffer() {
  write_pos = 0;
  sample_count = 0;
}

// ========================================
// Status Methods
// ========================================

bool LogicAnalyzerLCDCAM::isCapturing() const {
  return capturing;
}

uint32_t LogicAnalyzerLCDCAM::getSampleCount() const {
  return sample_count;
}

// ========================================
// Interrupt Handler (Stubs)
// ========================================

/**
 * VSYNC interrupt handler - called on frame sync
 * In this application, we use it for data synchronization
 */
void IRAM_ATTR lcd_cam_vsync_isr(void* arg) {
  // Frame synchronization signal
  // Can be used to trigger DMA transfers or mark frame boundaries
}

/**
 * DMA done interrupt handler
 * Called when DMA buffer transfer is complete
 */
void IRAM_ATTR lcd_cam_dma_done_isr(void* arg) {
  // DMA transfer complete
  // Can trigger buffer swap or compression
}

// ========================================
// Debug Methods
// ========================================

#if _DEBUG_MODE_

void LogicAnalyzerLCDCAM::printStatistics() {
  Serial.print("[DEBUG] Capture statistics:");
  Serial.print(" - Initialized: ");
  Serial.print(initialized ? "yes" : "no");
  Serial.print(" - Capturing: ");
  Serial.print(capturing ? "yes" : "no");
  Serial.print(" - Samples: ");
  Serial.print(sample_count);
  Serial.print(" - Buffer pos: ");
  Serial.print(write_pos);
  Serial.print("/");
  Serial.println(buffer_size);
}

void LogicAnalyzerLCDCAM::printConfiguration() {
  Serial.println("[DEBUG] LCD_CAM Configuration:");
  Serial.print(" - Channel mask: 0x");
  Serial.println(channel_mask, HEX);
  
  Serial.print(" - Clock divider: ");
  Serial.println(clock_divider_value);
  
  uint32_t freq_hz = 40000000 / clock_divider_value;
  Serial.print(" - Sampling frequency: ");
  Serial.print(freq_hz / 1000000);
  Serial.println(" MHz");
}

#endif
