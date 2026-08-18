/**
 * LogicAnalyzer_RLE.h - Run-Length Encoding Compression Header
 * 
 * Defines the RLE compression interface for logic analyzer data.
 * Supports both compression and decompression operations.
 */

#ifndef LOGIC_ANALYZER_RLE_H
#define LOGIC_ANALYZER_RLE_H

#include <Arduino.h>
#include <cstdint>
#include <cstring>

// ========================================
// RLE Compression Class
// ========================================

class LogicAnalyzerRLE {
private:
  // Buffers
  uint8_t* input_buffer;
  uint8_t* output_buffer;
  uint32_t input_size;
  uint32_t output_size;
  uint32_t output_buffer_size;
  
  // Position tracking
  uint32_t input_pos;
  uint32_t output_pos;
  
  // Configuration
  uint8_t sample_width;          // 1 (8-bit) or 2 (16-bit)
  uint8_t compression_mode;      // 0=Compact, 1=Fast
  float compression_ratio;       // Output/Input ratio
  
  // Private helper methods
  uint32_t countRepeats();
  bool writeRLEBlock(uint32_t count);
  bool writeZeroByte();
  bool writeDataByte();
  uint8_t readByte();
  bool writeByte(uint8_t value);
  
public:
  // Constructor/Destructor
  LogicAnalyzerRLE();
  ~LogicAnalyzerRLE();
  
  // Initialization
  bool initialize(uint8_t sample_width_bits = 16);
  
  // Compression operations
  bool compress(uint8_t* src, uint32_t src_len,
                uint8_t* dst, uint32_t dst_capacity,
                uint32_t& compressed_len);
  
  bool decompress(uint8_t* src, uint32_t src_len,
                  uint8_t* dst, uint32_t dst_capacity,
                  uint32_t& decompressed_len);
  
  // Configuration
  void setCompressionMode(uint8_t mode);  // 0=Compact, 1=Fast
  
  // Statistics
  float getCompressionRatio() const;
  uint32_t getCompressedSize() const;
  
  // Debug methods
  void printCompressionStats();
  void printTestCompression(uint8_t* test_data, uint32_t test_len);
};

#endif // LOGIC_ANALYZER_RLE_H
