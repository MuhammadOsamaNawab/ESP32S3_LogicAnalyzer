# Logic Analyzer Hardware Architecture (ESP32S3)

## Hardware Specifications

### Clock & Performance
- **System Clock**: 160 MHz (LA_HW_CLK_SAMPLE_RATE)
- **Capture Peripheral**: LCD_CAM (Camera/LCD interface)
- **DMA**: GDMA (Generic DMA with 32-byte burst capability)

### Channel Configuration
| Mode | Channels | Max Sample Rate (RAM) | Max Sample Rate (PSRAM) | Max Samples |
|------|----------|----------------------|------------------------|-------------|
| 8-bit | 8 | 80 MHz | 20 MHz | 200K (RAM) / 8M (PSRAM) |
| 16-bit | 16 | 40 MHz | 10 MHz | 100K (RAM) / 4M (PSRAM) |

### Minimum Specifications
- **Minimum Sample Rate**: 1 MHz (or 20 kHz with LEDC timer for slow rates)
- **Minimum Samples**: 100 per capture
- **GPIO Range**: 0-48 (ESP32S3 has 49 GPIO pins)

### Memory Architecture
1. **Internal RAM (DMA-accessible)**
   - Used for fast captures up to 80 MHz
   - Limited by free DMA-capable heap
   - Typical: 100K-200K samples for 16-channel mode

2. **PSRAM (Pseudo-RAM)**
   - For large captures (millions of samples)
   - Maximum: 8M samples (8-ch) / 4M samples (16-ch)
   - Reduced max sample rate due to SPI bus limitations
   - Requires ESP-IDF v5.2+ for proper cache management

## Capture Hardware Flow

### 1. LCD_CAM Peripheral
```
GPIO Pins (8 or 16) → LCD_CAM Interface
                         ↓
                    Parallel Data Capture
                         ↓
                    GDMA (DMA Controller)
                         ↓
                    Memory Buffer (RAM/PSRAM)
```

### 2. DMA Configuration
- **Frame Buffer**: 4096 bytes minus 64-byte header = 3968 bytes per frame
- **Descriptor Alignment**: 32-byte aligned for GDMA burst mode
- **Transfer Mode**: 
  - EOF interrupt on descriptor empty (`in_dscr_empty`)
  - Alternative: Success EOF mode (`in_suc_eof`)

### 3. Trigger Mechanism
The analyzer uses **Hi-Level Interrupt (Level 5)** for low-latency triggering:

```
GPIO Pin Edge → Hi-Level ISR (Level 5)
                     ↓
            Enable V_SYNC (LCD_CAM)
                     ↓
            Start DMA Transfer
                     ↓
            Disable GPIO Interrupt
            
Trigger Latency: ~0.3 µs (much faster than regular interrupts at ~2 µs)
```

### 4. Trigger ISR Handler
Located in `logic_analyzer_hi_interrupt_handler.s` (assembler for speed)

Registers involved:
- **DPORT_INTF_MAP_REG**: Route GPIO interrupt to level 5
- **GPIO_STAT_REG**: Check GPIO pin status
- **GPIO_PIN_CFG_REG**: Configure GPIO interrupt
- **I2S_SET_VSYNC_REG**: Enable V_SYNC for LCD_CAM capture start

## Data Capture Process

### Initialization Phase
1. Configure LCD_CAM to capture from selected GPIO pins
2. Setup GDMA descriptors pointing to buffer
3. Register trigger GPIO interrupt (if enabled)
4. Initialize DMA EOF interrupt handler

### Capture Phase
1. **Wait for Trigger** (if configured):
   - GPIO edge detected → Hi-Level ISR fires
   - ISR enables V_SYNC signal
   - LCD_CAM starts capturing data

2. **Without Trigger**: 
   - Call `logic_analyzer_ll_start()` to immediately begin capture

3. **Data Flow**:
   - GPIO pins sampled at configured sample rate
   - Data streamed through LCD_CAM to GDMA
   - GDMA transfers to buffer in 3968-byte frames
   - Each sample = 2 bytes (16-bit word per sample)

4. **Completion**:
   - GDMA EOF interrupt signals capture complete
   - Main task notified via FreeRTOS task notification
   - Callback invoked with sample buffer

## Configuration Options (Menuconfig)

### Physical GPIO
- `ANALYZER_PCLK_PIN`: GPIO for PCLK (parallel clock) - must be unused
- Channels: GPIO 0-48 configurable per channel

### Sample Control
- `ANALYZER_CHANNELS`: 8 or 16 channels
- `ANALYZER_SAMPLE_RATE`: 1M-80M Hz (depends on channels & memory)
- `ANALYZER_SAMPLES_COUNT`: Number of samples to capture

### Memory Selection
- `ANALYZER_PSRAM`: 0 = RAM (fast), 1 = PSRAM (large captures)

### Trigger Setup
- `ANALYZER_TRIG_PIN`: GPIO for trigger (-1 = disabled)
- `ANALYZER_TRIG_EDGE`: 1 = rising, 2 = falling

### Slow Sampling (< 1 MHz)
- `ANALYZER_USE_LEDC_TIMER_FOR_PCLK`: Enable LEDC PWM for PCLK generation
- `ANALYZER_LEDC_TIMER_NUMBER`: LEDC timer 0-3
- `ANALYZER_LEDC_CHANNEL_NUMBER`: LEDC channel 0-7

## CPU Overhead

### Minimal
- No CPU intervention during capture once started
- DMA operates independently with interrupts
- High-level interrupt for trigger is very efficient

### When Trigger Fires
- ~50-100 CPU cycles to enable V_SYNC in ISR
- Total trigger-to-capture latency: ~0.3 µs

## Limitations

1. **PCLK Pin**: Must have one free GPIO (not used by application)
2. **Sample Rates**: 
   - Can't achieve all rates (40-80 MHz) when 16-channel mode
   - Maximum depends on free memory
3. **Trigger Conflicts**: 
   - If GPIO has interrupt in user app, analyzer will override
   - Works but may lose some trigger events
4. **PSRAM Overhead**: 
   - Adds ~1.5-2x latency due to SPI bus sharing
   - Not recommended if main app uses SPI heavily

## Power Consumption
- LCD_CAM operational: ~5-15 mA depending on activity
- DMA transfers: ~2-3 mA
- Idle waiting for trigger: minimal (interrupt-driven)
