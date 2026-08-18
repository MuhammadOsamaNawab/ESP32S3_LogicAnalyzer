/**
 * LogicAnalyzer_RLE.cpp - Run-Length Encoding Compression Engine
 * 
 * Implements RLE compression for captured logic analyzer data.
 * Supports two modes:
 * - Fast mode (ALLOW_ZERO_RLE=1): Faster but slightly larger output
 * - Compact mode (ALLOW_ZERO_RLE=0): Better compression ratio
 * 
 * RLE Format:
 * - Count byte (0x00-0xFF): Number of repeats minus 1
 * - Data byte(s): 1 or 2 bytes depending on capture mode
 * - Zero-count marker (optional): RLE Count 0 for non-repeated values
 */

#include "LogicAnalyzer_RLE.h"
#include "LogicAnalyzerConfig.h"

// ========================================
// Constructor
// ========================================

LogicAnalyzerRLE::LogicAnalyzerRLE()
  : input_buffer(nullptr),
    output_buffer(nullptr),
    input_size(0),
    output_size(0),
    input_pos(0),
    output_pos(0),
    sample_width(2),  // Default 16-bit (2 bytes)
    compression_mode(ALLOW_ZERO_RLE),
    compression_ratio(0.0f) {
  
  #if _DEBUG_MODE_
    Serial.println("[DEBUG] LogicAnalyzerRLE constructor");
  #endif
}

// ========================================
// Destructor
// ========================================

LogicAnalyzerRLE::~LogicAnalyzerRLE() {
  // Buffers are managed externally
}

// ========================================
// Initialization
// ========================================

bool LogicAnalyzerRLE::initialize(uint8_t sample_width_bits) {
  if (sample_width_bits == 8) {
    sample_width = 1;
  } else if (sample_width_bits == 16) {
    sample_width = 2;
  } else {
    #if _DEBUG_MODE_
      Serial.print("[ERROR] Invalid sample width: ");
      Serial.println(sample_width_bits);
    #endif
    return false;
  }

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] RLE initialized with ");
    Serial.print(sample_width_bits);
    Serial.println("-bit samples");
  #endif

  return true;
}

// ========================================
// Compression
// ========================================

bool LogicAnalyzerRLE::compress(uint8_t* src, uint32_t src_len,
                                uint8_t* dst, uint32_t dst_capacity,
                                uint32_t& compressed_len) {
  if (!src || !dst) {
    return false;
  }

  input_buffer = src;
  output_buffer = dst;
  input_size = src_len;
  output_pos = 0;
  input_pos = 0;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Starting RLE compression: ");
    Serial.print(src_len);
    Serial.print(" bytes input, capacity: ");
    Serial.println(dst_capacity);
  #endif

  // Process data
  while (input_pos < input_size) {
    uint32_t count = countRepeats();

    if (count > 0) {
      // Repeated data
      if (!writeRLEBlock(count)) {
        #if _DEBUG_MODE_
          Serial.println("[ERROR] Buffer overflow during compression");
        #endif
        return false;
      }
    } else {
      // Non-repeated data
      if (compression_mode == 1) {
        // Fast mode: write zero-count marker + data
        if (!writeZeroByte()) {
          return false;
        }
        if (!writeDataByte()) {
          return false;
        }
      } else {
        // Compact mode: just write data (no marker)
        if (!writeDataByte()) {
          return false;
        }
      }
    }

    // Safety check to prevent buffer overflow
    if (output_pos >= dst_capacity) {
      #if _DEBUG_MODE_
        Serial.println("[ERROR] Output buffer full");
      #endif
      return false;
    }
  }

  compressed_len = output_pos;

  // Calculate compression ratio
  if (input_size > 0) {
    compression_ratio = (float)output_pos / (float)input_size;
  }

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Compression complete: ");
    Serial.print(compressed_len);
    Serial.print(" bytes (ratio: ");
    Serial.print(compression_ratio * 100, 1);
    Serial.println("%)");
  #endif

  return true;
}

// ========================================
// Decompression (for verification/testing)
// ========================================

bool LogicAnalyzerRLE::decompress(uint8_t* src, uint32_t src_len,
                                  uint8_t* dst, uint32_t dst_capacity,
                                  uint32_t& decompressed_len) {
  if (!src || !dst) {
    return false;
  }

  input_buffer = src;
  output_buffer = dst;
  input_size = src_len;
  output_pos = 0;
  input_pos = 0;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Starting RLE decompression: ");
    Serial.print(src_len);
    Serial.print(" bytes input");
  #endif

  // Process compressed data
  while (input_pos < input_size) {
    uint8_t count = readByte();
    input_pos++;

    if (count == 0 && compression_mode == 1) {
      // Zero-count marker - single uncompressed byte
      if (input_pos >= input_size) {
        #if _DEBUG_MODE_
          Serial.println("[ERROR] Truncated data in decompression");
        #endif
        return false;
      }
      uint8_t data = readByte();
      input_pos++;
      
      if (!writeByte(data)) {
        return false;
      }
    } else {
      // Compressed block (count+1) repetitions
      if (input_pos >= input_size) {
        return false;
      }
      
      uint8_t data = readByte();
      input_pos++;

      uint32_t reps = count + 1;
      for (uint32_t i = 0; i < reps; i++) {
        if (!writeByte(data)) {
          return false;
        }
      }
    }

    // Safety check
    if (output_pos >= dst_capacity) {
      return false;
    }
  }

  decompressed_len = output_pos;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] Decompression complete: ");
    Serial.print(decompressed_len);
    Serial.println(" bytes");
  #endif

  return true;
}

// ========================================
// Private Helper Methods
// ========================================

uint32_t LogicAnalyzerRLE::countRepeats() {
  if (input_pos + sample_width > input_size) {
    return 0;
  }

  uint8_t* current = &input_buffer[input_pos];
  uint32_t count = 1;
  uint32_t check_pos = input_pos + sample_width;

  // Count how many times the pattern repeats
  while (check_pos + sample_width <= input_size && count < RLE_MAX_COUNT) {
    uint8_t* next = &input_buffer[check_pos];

    if (memcmp(current, next, sample_width) == 0) {
      count++;
      check_pos += sample_width;
    } else {
      break;
    }
  }

  // Return count only if we have at least 2 repetitions
  return (count > 1) ? count : 0;
}

bool LogicAnalyzerRLE::writeRLEBlock(uint32_t count) {
  if (output_pos + sample_width + 1 > output_buffer_size) {
    return false;  // Would overflow
  }

  // Write count (minus 1)
  output_buffer[output_pos++] = (count - 1) & 0xFF;

  // Write data
  for (uint32_t i = 0; i < sample_width; i++) {
    output_buffer[output_pos++] = input_buffer[input_pos + i];
  }

  input_pos += count * sample_width;
  return true;
}

bool LogicAnalyzerRLE::writeZeroByte() {
  if (output_pos >= output_buffer_size) {
    return false;
  }
  output_buffer[output_pos++] = 0x00;  // Zero-count marker
  return true;
}

bool LogicAnalyzerRLE::writeDataByte() {
  if (output_pos + sample_width > output_buffer_size) {
    return false;
  }

  for (uint32_t i = 0; i < sample_width; i++) {
    output_buffer[output_pos++] = input_buffer[input_pos++];
  }
  return true;
}

uint8_t LogicAnalyzerRLE::readByte() {
  if (input_pos < input_size) {
    return input_buffer[input_pos];
  }
  return 0;
}

bool LogicAnalyzerRLE::writeByte(uint8_t value) {
  if (output_pos >= output_buffer_size) {
    return false;
  }
  output_buffer[output_pos++] = value;
  return true;
}

// ========================================
// Configuration
// ========================================

void LogicAnalyzerRLE::setCompressionMode(uint8_t mode) {
  compression_mode = (mode == 0) ? 0 : 1;

  #if _DEBUG_MODE_
    Serial.print("[DEBUG] RLE compression mode set to: ");
    Serial.println(compression_mode == 0 ? "Compact" : "Fast");
  #endif
}

// ========================================
// Statistics
// ========================================

float LogicAnalyzerRLE::getCompressionRatio() const {
  return compression_ratio;
}

uint32_t LogicAnalyzerRLE::getCompressedSize() const {
  return output_pos;
}

// ========================================
// Debug Methods
// ========================================

#if _DEBUG_MODE_

void LogicAnalyzerRLE::printCompressionStats() {
  Serial.println("[DEBUG] RLE Compression Statistics:");
  Serial.print(" - Sample width: ");
  Serial.print(sample_width);
  Serial.println(" bytes");
  
  Serial.print(" - Compression mode: ");
  Serial.println(compression_mode == 0 ? "Compact (ALLOW_ZERO_RLE=0)" : "Fast (ALLOW_ZERO_RLE=1)");
  
  Serial.print(" - Compression ratio: ");
  Serial.print(compression_ratio * 100, 1);
  Serial.println("%");
  
  Serial.print(" - Last output size: ");
  Serial.print(output_pos);
  Serial.println(" bytes");
}

void LogicAnalyzerRLE::printTestCompression(uint8_t* test_data, uint32_t test_len) {
  // Allocate temporary buffers for testing
  uint8_t* compressed = (uint8_t*)malloc(test_len);
  uint32_t compressed_len = 0;

  if (!compressed) {
    Serial.println("[ERROR] Failed to allocate test buffer");
    return;
  }

  Serial.print("[DEBUG] Testing RLE compression on ");
  Serial.print(test_len);
  Serial.println(" bytes of data");

  if (compress(test_data, test_len, compressed, test_len, compressed_len)) {
    Serial.print("[DEBUG] Compressed to ");
    Serial.print(compressed_len);
    Serial.print(" bytes (");
    Serial.print((float)compressed_len / test_len * 100, 1);
    Serial.println("%)");
  } else {
    Serial.println("[ERROR] Compression failed");
  }

  free(compressed);
}

#endif
