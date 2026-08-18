/**
 * ESP32-S3 Logic Analyzer - Main Firmware Sketch
 * 
 * This is the main Arduino sketch for the ESP32-S3 based 16-channel Logic Analyzer.
 * It initializes hardware, manages capture state, and handles SUMP protocol communication.
 * 
 * Features:
 * - 16-bit parallel data capture via LCD_CAM interface
 * - Up to 40+ MHz sampling rate
 * - RLE (Run-Length Encoding) compression
 * - SUMP protocol compatibility with PulseView
 * - LED status indicator
 * - Power management
 * 
 * Author: [Your Name]
 * Date: 2026-08-18
 * Version: 1.0
 */

#include <Arduino.h>
#include "LogicAnalyzerConfig.h"
#include "LogicAnalyzer.h"

// ========================================
// Global Objects
// ========================================

/** Main logic analyzer instance */
LogicAnalyzer g_analyzer;

// ========================================
// Setup Function
// ========================================

void setup() {
  // Initialize Serial communications (only once, even if both OLS and DEBUG use it)
  OLS_PORT.begin(OLS_PORT_BAUD);
  delay(500);  // Wait longer for USB enumeration on ESP32-S3

  #if _DEBUG_MODE_
    DEBUG_PORT.println("\n[DEBUG] ======================================");
    DEBUG_PORT.println("[DEBUG] ESP32-S3 Logic Analyzer Initializing");
    DEBUG_PORT.println("[DEBUG] ======================================");
    DEBUG_PORT.flush();
    DEBUG_PORT.print("[DEBUG] Heap before init: ");
    DEBUG_PORT.println(ESP.getFreeHeap());
    DEBUG_PORT.flush();
  #endif

  // Initialize GPIO pins
  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] Calling initializePins()...");
    DEBUG_PORT.flush();
  #endif
  initializePins();
  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] initializePins() complete");
    DEBUG_PORT.flush();
  #endif

  // Initialize LED (status indicator)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] Calling g_analyzer.initialize()...");
    DEBUG_PORT.flush();
  #endif

  // Initialize the analyzer
  if (!g_analyzer.initialize()) {
    #if _DEBUG_MODE_
      DEBUG_PORT.println("[ERROR] Analyzer initialization failed!");
      DEBUG_PORT.flush();
    #endif
    blinkError();
    return;
  }

  #if _DEBUG_MODE_
    DEBUG_PORT.print("[DEBUG] Heap after init: ");
    DEBUG_PORT.println(ESP.getFreeHeap());
    DEBUG_PORT.println("[DEBUG] ESP32-S3 Logic Analyzer Ready");
    DEBUG_PORT.println("[DEBUG] Waiting for SUMP commands...");
    DEBUG_PORT.println("[DEBUG] Type ID command: 0x02");
    DEBUG_PORT.flush();
  #endif

  // Signal ready (blink LED twice)
  blinkReady();
}

// ========================================
// Main Loop
// ========================================

void loop() {
  // Handle incoming SUMP protocol commands
  if (OLS_PORT.available()) {
    uint8_t cmd = OLS_PORT.read();
    
    // Process SUMP command
    if (!g_analyzer.handleCommand(cmd)) {
      #if _DEBUG_MODE_
        DEBUG_PORT.print("[ERROR] Invalid command: 0x");
        DEBUG_PORT.println(cmd, HEX);
      #endif
    }
  }

  // Handle idle power saving
  if (POWER_SAVE_ENABLED && !g_analyzer.isCapturing()) {
    static unsigned long last_activity = millis();
    unsigned long current_time = millis();

    if (current_time - last_activity > POWER_SAVE_IDLE_TIME) {
      // Enter idle mode (lower CPU frequency)
      if (POWER_SAVE_REDUCE_CPU_CLOCK) {
        // Would reduce CPU clock here (requires esp_pm API)
        #if _DEBUG_MODE_
          DEBUG_PORT.println("[INFO] Entering power save mode");
        #endif
      }
      last_activity = current_time;
    }
  }

  // Update LED status
  updateLED();

  // Watchdog check
  if (g_analyzer.isCapturing() && millis() - g_analyzer.getCaptureStartTime() > WATCHDOG_TIMEOUT) {
    #if _DEBUG_MODE_
      DEBUG_PORT.println("[WARNING] Capture watchdog timeout!");
    #endif
    g_analyzer.abortCapture();
  }

  // Small delay to prevent watchdog reset
  delay(1);
}

// ========================================
// Pin Initialization
// ========================================

void initializePins() {
  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] Setting data pins to INPUT...");
    DEBUG_PORT.flush();
  #endif
  
  // Configure data pins as inputs
  pinMode(CH0_PIN, INPUT);
  pinMode(CH1_PIN, INPUT);
  pinMode(CH2_PIN, INPUT);
  pinMode(CH3_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);
  pinMode(CH5_PIN, INPUT);
  pinMode(CH6_PIN, INPUT);
  pinMode(CH7_PIN, INPUT);
  pinMode(CH8_PIN, INPUT);
  pinMode(CH9_PIN, INPUT);
  pinMode(CH10_PIN, INPUT);
  pinMode(CH11_PIN, INPUT);
  pinMode(CH12_PIN, INPUT);
  pinMode(CH13_PIN, INPUT);
  pinMode(CH14_PIN, INPUT);
  pinMode(CH15_PIN, INPUT);

  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] Setting clock pins...");
    DEBUG_PORT.flush();
  #endif

  // Configure clock pins
  pinMode(PCLK_PIN, INPUT);    // PCLK is input from LCD_CAM
  pinMode(XCLK_PIN, INPUT);    // XCLK reference clock

  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] Setting level shifter control pins...");
    DEBUG_PORT.flush();
  #endif

  // Configure level shifter control pins as outputs
  // NOTE: GPIO 42-44 may not be available on ESP32-S3-DevKitC-1
  // Commenting out for now to avoid hang - level shifters can be controlled manually
  //pinMode(DIR_A_PIN, OUTPUT);
  //pinMode(DIR_B_PIN, OUTPUT);
  //pinMode(OE_A_PIN, OUTPUT);
  //pinMode(OE_B_PIN, OUTPUT);

  // Initialize level shifter control (disabled, direction = receive)
  // digitalWrite(DIR_A_PIN, LOW);   // DIR=0: A→B (receive mode)
  // digitalWrite(DIR_B_PIN, LOW);   // DIR=0: A→B (receive mode)
  // digitalWrite(OE_A_PIN, HIGH);   // OE=1: outputs disabled
  // digitalWrite(OE_B_PIN, HIGH);   // OE=1: outputs disabled

  #if _DEBUG_MODE_
    DEBUG_PORT.println("[DEBUG] GPIO pins initialized successfully");
    DEBUG_PORT.flush();
  #endif
}

// ========================================
// LED Status Functions
// ========================================

/** Blink LED to indicate ready state */
void blinkReady() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

/** Blink LED rapidly to indicate error */
void blinkError() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }
}

/** Update LED based on capture state */
void updateLED() {
  static unsigned long last_blink = millis();
  unsigned long current_time = millis();

  if (g_analyzer.isCapturing()) {
    // LED steady on during capture
    digitalWrite(LED_PIN, HIGH);
  } else {
    // LED blinks slowly when idle
    if (current_time - last_blink > LED_IDLE_BLINK_MS) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      last_blink = current_time;
    }
  }
}

// ========================================
// Debug/Utility Functions
// ========================================

#if _DEBUG_MODE_

/**
 * Print heap and stack statistics for debugging
 */
void printMemoryStats() {
  DEBUG_PORT.print("[DEBUG] Free Heap: ");
  DEBUG_PORT.print(ESP.getFreeHeap());
  DEBUG_PORT.print(" bytes | Free PSRAM: ");
  DEBUG_PORT.print(ESP.getFreePsram());
  DEBUG_PORT.println(" bytes");
}

/**
 * Print capture statistics
 */
void printCaptureStats() {
  DEBUG_PORT.print("[INFO] Samples captured: ");
  DEBUG_PORT.print(g_analyzer.getSampleCount());
  DEBUG_PORT.print(" | Compressed: ");
  DEBUG_PORT.println(g_analyzer.getCompressedSize());
}

#endif

// ========================================
// Interrupt Handlers (if needed)
// ========================================

/**
 * VSYNC interrupt handler for LCD_CAM frame sync
 * (To be implemented in LogicAnalyzer_LCD_CAM.cpp)
 */
void IRAM_ATTR onVSyncInterrupt() {
  // Frame sync signal - used for synchronization
  // Implementation in LCD_CAM driver
}

/**
 * DMA interrupt handler for data transfer completion
 * (To be implemented in LogicAnalyzer_LCD_CAM.cpp)
 */
void IRAM_ATTR onDMAComplete() {
  // DMA buffer transfer complete
  // Implementation in LCD_CAM driver
}
