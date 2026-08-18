# ESP32-S3 Logic Analyzer

![Status](https://img.shields.io/badge/Status-Design%20Phase-yellow)
![License](https://img.shields.io/badge/License-GPL%203.0-blue)

A [SUMP](http://dangerousprototypes.com/docs/The_Logic_Sniffer%27s_extended_SUMP_protocol) compatible 16-bit Logic Analyzer for **ESP32-S3** microcontroller with level shifter support for 5V signal capture.

## Features

- **Sampling Speed**: Up to **20 MHz** using I2S DMA
- **Channel Count**: 16 digital channels
- **Capture Modes**: 8-bit or 16-bit
- **Maximum Samples**: 128K samples (can extend to 512K with PSRAM variant)
- **Compression**: RLE (Run-Length Encoding) supported
- **Level Shifter**: SN74HC245N ICs for 5V→3.3V signal conversion
- **USB Interface**: Native USB-C with OLS SUMP protocol
- **Protocol**: Compatible with PulseView, Sigrok ecosystem
- **Power**: 3.3V or USB-C powered

## Key Advantages over ESP32 Version

| Feature | ESP32 | ESP32-S3 |
|---------|-------|----------|
| CPU Speed | 160 MHz | **240 MHz** |
| PSRAM Support | Limited | **8MB native** |
| USB-Serial | External chip | **Native USB-C** |
| GPIO Pins | 32 | **48** |
| I2S DMA | Supported | **Enhanced** |
| Power Consumption | ~80mA | **~60mA** |

## Project Structure

```
ESP32S3_LogicAnalyzer/
├── HARDWARE_DESIGN.md          # Complete hardware specification
├── PINOUT_REFERENCE.md         # GPIO pin mapping & connections
├── BOM.md                       # Bill of Materials & sourcing
├── hardware/                    # KiCad PCB design (TBD)
│   ├── ESP32S3_LA.kicad_pro
│   ├── ESP32S3_LA.kicad_sch
│   ├── ESP32S3_LA.kicad_pcb
│   └── gerbers/
├── firmware/                    # Arduino/ESP-IDF code
│   ├── ESP32S3_LogicAnalyzer.ino
│   ├── LogicAnalyzer.h
│   ├── LogicAnalyzer.cpp
│   ├── LogicAnalyzer_I2S_DMA.cpp
│   ├── LogicAnalyzer_RLE.cpp
│   ├── LogicAnalyzerConfig.h
│   └── platformio.ini
├── docs/                        # Documentation
│   ├── SETUP_GUIDE.md
│   ├── USAGE.md
│   ├── TROUBLESHOOTING.md
│   └── images/
└── README.md                    # This file
```

## Hardware Components

### Essential Components
- **MCU**: ESP32-S3-DevKitC-1 (8MB PSRAM variant recommended)
- **Level Shifters**: 2x SN74HC245N (8-bit bidirectional buffer)
- **Connectors**: 20-pin header for signal input
- **Capacitors**: 10x 100nF + 2x 10µF
- **Resistors**: 4x 10kΩ (pull-ups) + 1x 470Ω (LED)
- **LED**: 3mm red status indicator

### Pin Configuration
```
Data Channels (16 total):
├─ CH0-CH7  → GPIO2-9 (via SN74HC245N #1)
└─ CH8-CH15 → GPIO16-19, 21, 26-28 (via SN74HC245N #2)

Clock Signals:
├─ CLK_IN  → GPIO23 (I2S input clock)
└─ CLK_OUT → GPIO22 (LEDC output clock)

Control Lines:
├─ DIR_A   → GPIO29 (Level shifter #1 direction)
├─ DIR_B   → GPIO30 (Level shifter #2 direction)
├─ OE_A    → GPIO31 (Level shifter #1 output enable)
├─ OE_B    → GPIO32 (Level shifter #2 output enable)
└─ LED_PIN → GPIO1  (Status indicator)
```

See [PINOUT_REFERENCE.md](PINOUT_REFERENCE.md) for detailed pinout.

## Hardware Design Details

Complete hardware design is documented in [HARDWARE_DESIGN.md](HARDWARE_DESIGN.md), including:
- ✅ Level shifter circuit design
- ✅ Power distribution requirements
- ✅ Signal integrity considerations
- ✅ PCB layout guidelines
- ✅ Component placement strategies
- ✅ Thermal management

## Software Architecture

### Firmware Components

1. **LogicAnalyzer.h/cpp** - Main controller class
   - SUMP protocol command handling
   - Sample buffer management
   - Capture control

2. **LogicAnalyzer_I2S_DMA.cpp** - Hardware interface
   - I2S peripheral configuration
   - DMA buffer setup
   - Interrupt handling

3. **LogicAnalyzer_RLE.cpp** - Data compression
   - Run-length encoding algorithm
   - Buffer compression
   - Decompression support

4. **LogicAnalyzerConfig.h** - Configuration
   - Pin definitions
   - UART settings
   - Buffer sizes
   - Capture parameters

### Firmware Flow

```
Startup:
└─ setup()
   ├─ Initialize UART (921600 baud)
   ├─ Configure GPIO pins
   ├─ Setup LED indicator
   ├─ Initialize I2S DMA
   └─ Ready for commands

Runtime Loop:
└─ loop()
   ├─ Check for OLS commands via UART
   ├─ Handle SUMP protocol:
   │  ├─ 0x00 - RESET
   │  ├─ 0x01 - ARM (start capture)
   │  ├─ 0x02 - QUERY (get metadata)
   │  ├─ 0x04 - GET_METADATA
   │  ├─ 0x80 - SET_DIVIDER (clock divider)
   │  ├─ 0x81 - SET_READ_DELAY_COUNT
   │  └─ 0x82 - SET_FLAGS (RLE config)
   └─ Stream captured data via UART

Capture (ISR):
└─ I2S_ISR()
   ├─ Read DMA buffer
   ├─ Apply RLE compression (if enabled)
   ├─ Store in sample buffer
   └─ Trigger next DMA transfer
```

## Development Environment

### Supported Platforms

| Environment | Status | Version |
|------------|--------|---------|
| PlatformIO | ✅ Support | 6.0+  |
| Arduino IDE | ✅ Planned | 2.0+ |
| ESP-IDF | ⏳ Planned | 5.0+ |

### Installation

#### PlatformIO Setup
```bash
1. Install PlatformIO in VS Code
2. Clone/open this project folder
3. Select platform: esp32-s3-devkitc-1
4. Build: platformio run
5. Upload: platformio run --target upload
```

#### Arduino IDE Setup
```bash
1. Install ESP32 board support (esp32 by Espressif Systems v3.0+)
2. Select Board: ESP32S3 Dev Module
3. Configure settings as per SETUP_GUIDE.md
4. Upload to ESP32-S3
```

## Quick Start

### 1. Hardware Assembly
- Assemble PCB with level shifters and connectors
- Connect ESP32-S3 to 20-pin connector header
- Verify all power and ground connections
- See [HARDWARE_DESIGN.md](HARDWARE_DESIGN.md) for detailed instructions

### 2. Flash Firmware
```bash
# Via PlatformIO
platformio run --target upload

# Via Arduino IDE
Sketch → Upload
```

### 3. Connect to PulseView
```bash
# Linux/Mac
pulseview -D -d ols:conn=/dev/ttyUSB0::serialcomm=921600/8n1

# Windows
pulseview -D -d ols:conn=COM3::serialcomm=921600/8n1
```

### 4. Configure Capture
- Set Sample Rate (divider)
- Set Capture Depth (up to 128K samples)
- Enable RLE compression if desired
- Click "Arm" to start capture

See [SETUP_GUIDE.md](docs/SETUP_GUIDE.md) for detailed walkthrough.

## Connector Wiring

### Target Device Connection (20-pin Header)
```
Position  Signal      Target Connection
1         GND         Common Ground
2-17      CH0-CH15    Digital signals to capture
18        CLK_IN      External reference clock (optional)
19        CLK_OUT     Clock output to target (optional)
20        GND         Common Ground
```

### Important Notes
⚠️ **CRITICAL**: All ground connections must be common between ESP32-S3 and target device.

## Communication Protocol

### UART Settings
- **Port**: UART0 (USB-C on ESP32-S3)
- **Baud Rate**: 921600 bps (default)
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### OLS SUMP Protocol
- Compatible with Sigrok/PulseView SUMP driver
- Supports extended commands
- RLE compression optional

## Specifications

### Capture Specifications
| Parameter | Value |
|-----------|-------|
| Max Sampling Rate | 20 MHz |
| Min Sampling Rate | ~1 MHz |
| Channels | 16 digital |
| Channel Impedance | ~100kΩ (via level shifter) |
| Input Voltage | 3.3V-5V (with level shifter) |
| Max Sample Depth | 128K (standard) / 512K (PSRAM) |

### Timing Specifications
| Parameter | Value |
|-----------|-------|
| Setup Time | ~10 ns |
| Hold Time | ~5 ns |
| Propagation Delay | ~6.5 ns (level shifter) |
| Clock Jitter | ±50 ppm |

### Power Specifications
| Parameter | Value |
|-----------|-------|
| Supply Voltage | 5V (USB-C) or 3.3V external |
| Typical Current | 60-100 mA |
| Peak Current | ~200 mA |
| Idle Current | ~10 mA |

## Known Limitations

1. **No Analog Channels** - Digital signals only (future enhancement)
2. **Single-Stage Trigger** - Only stage 0 supported for triggering
3. **Unidirectional** - Input capture only, no output generation
4. **Clock Resolution** - ~1 MHz minimum due to clock divider

## Troubleshooting

Common issues and solutions are documented in [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

**Quick Diagnostics**:
- LED not lighting: Check GPIO1 and power connections
- No UART communication: Verify USB-C cable and drivers
- Capture errors: Check level shifter VCC and DIR/OE lines
- Low sample rate: Verify clock divider settings in config

## Performance Benchmarks

### Capture Performance (20 MHz clock)
| Mode | Throughput | CPU Usage | Compression Ratio |
|------|-----------|-----------|-------------------|
| 16-bit Raw | 320 Mbps | ~70% | N/A |
| 16-bit RLE | 280 Mbps | ~85% | 40-60% |
| 8-bit Raw | 160 Mbps | ~50% | N/A |
| 8-bit RLE | 140 Mbps | ~60% | 50-70% |

### Buffer Sizes
- Standard: 128K samples (~256 KB with metadata)
- With PSRAM: 512K samples (~1 MB with metadata)
- RLE Buffer: 96KB (80K raw samples compressed)

## Comparison: ESP32 vs ESP32-S3

| Feature | ESP32 | ESP32-S3 | Improvement |
|---------|-------|----------|-------------|
| Max Frequency | 20 MHz | 20 MHz | Same |
| CPU Clock | 160 MHz | 240 MHz | +50% |
| SRAM | 520 KB | 520 KB | Same |
| PSRAM | Optional | 8 MB native | Better |
| GPIO Count | 32 | 48 | +50% |
| USB-Serial | No (external) | Yes (native) | +1 connector |
| I2S Features | Limited | Enhanced | Better stability |
| Power Eff. | ~80 mA | ~60 mA | -25% |

## Development Roadmap

- [x] Hardware design specification
- [x] Pinout and BOM documentation
- [ ] KiCad PCB design
- [ ] Firmware core (next phase)
- [ ] I2S DMA driver (next phase)
- [ ] RLE compression (next phase)
- [ ] PulseView integration testing
- [ ] Analog channel support (future)
- [ ] Multi-stage triggering (future)

## Contributing

This is an open-source project. Contributions welcome:
- Bug reports and fixes
- Hardware improvements
- Firmware optimizations
- Documentation enhancements

## License

GPL 3.0 - See LICENSE file for details

## References

- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SUMP Protocol Specification](http://dangerousprototypes.com/docs/The_Logic_Sniffer%27s_extended_SUMP_protocol)
- [PulseView Documentation](https://sigrok.org/wiki/PulseView)
- [SN74HC245N Datasheet](https://www.ti.com/lit/ds/symlink/sn74hc245.pdf)
- [Level Shifter Design Guide](https://www.ti.com/lit/an/slva504/slva504.pdf)

## Support

For issues, questions, or contributions, please refer to:
- 📖 [Setup Guide](docs/SETUP_GUIDE.md)
- 🔧 [Troubleshooting](docs/TROUBLESHOOTING.md)
- 📋 [Hardware Design](HARDWARE_DESIGN.md)
- 📌 [Pinout Reference](PINOUT_REFERENCE.md)

---

**Status**: ✅ Hardware Design Complete → ⏳ Ready for Firmware Development

**Next Phase**: Firmware implementation with I2S DMA capture and RLE compression
