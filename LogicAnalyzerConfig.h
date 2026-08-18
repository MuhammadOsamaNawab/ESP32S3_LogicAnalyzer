#ifndef LOGIC_ANALYZER_CONFIG_H
#define LOGIC_ANALYZER_CONFIG_H

#include <Arduino.h>

// ========================================
// Debug Configuration
// ========================================

#define _DEBUG_MODE_ 0  // Disable debug output to keep SUMP protocol clean
#define VERBOSE_DEBUG 1
#define LOG_LCD_CAM 1
#define LOG_RLE_STATS 0
#define PROFILE_ENABLED 0

// ========================================
// UART Configuration (OLS Communication)
// ========================================

#define OLS_PORT Serial
#define OLS_PORT_BAUD 921600

#if _DEBUG_MODE_
  #define DEBUG_PORT Serial
  #define DEBUG_PORT_BAUD 921600
#else
  #define DEBUG_PORT Serial
  #define DEBUG_PORT_BAUD 921600
#endif

// ========================================
// LCD_CAM Configuration
// ========================================

#define LCD_CAM_PORT I2S_NUM_0
#define LCD_CAM_PARALLEL_BITS 16
#define LCD_CAM_DMA_BUF_LEN 4096
#define LCD_CAM_DMA_BUF_COUNT 4

// ========================================
// Capture Buffer Configuration
// ========================================

#define CAPTURE_SIZE 32000
#define RLE_BUFFER_SIZE 24000
#define RLE_MAX_COUNT 0x10000

// ========================================
// RLE Compression Settings
// ========================================

#define ALLOW_ZERO_RLE 0
#define RLE_ENABLED_BY_DEFAULT 1

// ========================================
// GPIO Pin Assignments - Custom ESP32-S3 Mapping
// ========================================

#define PCLK_PIN  3
#define XCLK_PIN  23

#define CH0_PIN   4
#define CH1_PIN   5
#define CH2_PIN   6
#define CH3_PIN   7
#define CH4_PIN   16
#define CH5_PIN   17
#define CH6_PIN   18
#define CH7_PIN   8
#define CH8_PIN   9
#define CH9_PIN   10
#define CH10_PIN  12
#define CH11_PIN  13
#define CH12_PIN  14
#define CH13_PIN  19
#define CH14_PIN  20
#define CH15_PIN  21

#define DIR_A_PIN   2
#define DIR_B_PIN   42
#define OE_A_PIN    43
#define OE_B_PIN    44

#define LED_PIN   1
#define CLK_OUT_PIN 46

// ========================================
// Clock Configuration
// ========================================

#define DEFAULT_CLOCK_DIVIDER 2
#define MIN_CLOCK_DIVIDER 1
#define MAX_CLOCK_DIVIDER 65535
#define BASE_CLOCK_FREQ 40000000

// ========================================
// SUMP Protocol Configuration
// ========================================

#define SUMP_MAGIC 0x5A4C5032
#define SUMP_MAX_CHANNELS 16
#define SUMP_DEFAULT_SAMPLE_RATE 40000000
#define DEFAULT_SAMPLE_COUNT 4096
#define DEFAULT_READ_DELAY_COUNT 0

// ========================================
// Device Information
// ========================================

#define DEVICE_NAME "ESP32S3-LA"
#define DEVICE_VERSION "1.0"

// ========================================
// Feature Flags
// ========================================

#define FEATURE_METADATA 1
#define FEATURE_RLE_COMPRESSION 1
#define FEATURE_16BIT_MODE 1
#define FEATURE_TRIGGER 1
#define FEATURE_CLOCK_OUTPUT 1

// ========================================
// Performance Tuning
// ========================================

#define POWER_SAVE_ENABLED 0
#define POWER_SAVE_IDLE_TIME 30000
#define POWER_SAVE_REDUCE_CPU_CLOCK 0
#define POWER_SAVE_IDLE_FREQ 80
#define IDLE_TIMEOUT_MS 5000
#define WATCHDOG_TIMEOUT 5000
#define WATCHDOG_TIMEOUT_S 30
#define LED_IDLE_BLINK_MS 1000
#define LED_CAPTURE_BRIGHTNESS 100

// ========================================
// Trigger Configuration Defaults
// ========================================

#define TRIGGER_DEFAULT_DELAY 0
#define TRIGGER_DEFAULT_EDGE 0

#endif // LOGIC_ANALYZER_CONFIG_H
