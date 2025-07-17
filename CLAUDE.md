# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Saleae Logic Analyzer plugin for decoding 8b/10b encoded data streams. The project implements a custom analyzer that can decode 8b/10b protocol data from captured waveforms in the Saleae Logic software.

## Architecture

The codebase follows the Saleae Analyzer SDK architecture pattern:

- **Main Analyzer Class** (`decode8b10bAnalyzer`): Core analyzer logic in `src/decode8b10bAnalyzer.cpp/h`
- **Settings Class** (`decode8b10bAnalyzerSettings`): Configuration and channel settings in `src/decode8b10bAnalyzerSettings.cpp/h`
- **Results Class** (`decode8b10bAnalyzerResults`): Data processing and result formatting in `src/decode8b10bAnalyzerResults.cpp/h`
- **Simulation Generator** (`decode8b10bSimulationDataGenerator`): Test data generation in `src/decode8b10bSimulationDataGenerator.cpp/h`

The analyzer inherits from `Analyzer2` and implements the required virtual methods for the Saleae SDK. The main decoding logic is in the `WorkerThread()` method which processes the input channel data to find and decode 8b/10b frames.

## Build Commands

This project uses CMake for building across platforms:

### Initial Setup
```bash
mkdir build
cd build
cmake ..
```

### Build Commands
```bash
# Debug build
cmake --build .

# Release build (recommended for distribution)
cmake --build . --config Release
```

### Platform-Specific Output Locations
- **Windows**: `build/Analyzers/Debug/decode8b10bAnalyzer.dll` (debug) or `build/Analyzers/Release/decode8b10bAnalyzer.dll` (release)
- **macOS/Linux**: `build/Analyzers/libdecode8b10bAnalyzer.so`

## Development Workflow

1. **Renaming**: If creating a new analyzer, use `python rename_analyzer.py` to customize class names and display names
2. **Building**: Use CMake commands above
3. **Testing**: Load the built analyzer into Saleae Logic 2 software via the developer directory
4. **Debugging**: Attach debugger to Logic 2 process using the PID shown in the About dialog

## Key Dependencies

- **Saleae AnalyzerSDK**: Automatically fetched via CMake from GitHub
- **CMake 3.13+**: Required for building
- **C++11**: Language standard used
- **Platform-specific compilers**: Visual Studio 2017+ (Windows), Xcode (macOS), gcc 5+ (Linux)

## 8b/10b Decoding Logic

The analyzer implements complete 8b/10b decoding with the following features:

### Core Functionality
- **serial data stream sends bits in least-significant-bit (LSB) first manner
- **K28.5 Comma Detection**: Searches for K28.5 synchronization patterns (0x17A RD- or 0x285 RD+) to establish symbol boundaries
- **10-bit Symbol Extraction**: Samples 10 consecutive bits per symbol at the configured bit rate
- **Reverse Lookup Decoding**: Maps 10-bit symbols back to 8-bit values using lookup tables
- **Running Disparity Tracking**: Maintains running disparity state for protocol validation
- **Error Detection**: Identifies invalid symbols and disparity errors
- **Symbol Classification**: Distinguishes between K-characters (control) and D-characters (data)
- **Configurable debug**: Frame information is optionally sprinted, controllable by a user setting, defaulted
to off.

### Implementation Details
- **Sliding Window Synchronization**: Uses 10-bit sliding window to find the K28.5 comma characters that may start at any bit position. It first aligns to a rising or falling edge in the sampled data, then starts sampling 0.5 bit period later so that we're sampling the bit in the middle of the bit period. The sliding
window then moves 1 bit period at a time until the 10-bit symbol matches a comma character. This is
the first reported frame. This search will continue through the entire record, discarding samples until the
comma character is located.
- **Symbol Extraction**: `ExtractSymbol()` samples 10 bits at the correct timing once synchronized
- **Decoding**: `DecodeSymbol()` uses reverse lookup table to convert 10-bit codes to 8-bit values
- **Error Handling**: Graceful handling of unknown symbols with debugging information
- **Re-synchronization**: Attempts to find comma characters when errors are detected
- **Display**: Results show symbol types (K28.5, D0x00, etc.) with running disparity information
- **Export**: CSV format includes symbol type, value, control/data classification, and error status

### Frame Annotations
- **Multi-level Display**: Short (K28.5), medium (K28.5 RD+), and detailed (K28.5: 001111|1010) formats, configurable via settings. Medium is the default setting.
- **Bit-level Markers**: Dots for 5b/6b portion, squares for 3b/4b portion
- **Special Indicators**: Up arrows mark K28.5 comma characters for synchronization
- **Frame Types**: Data (type 0), Control (type 1), Error (type 2) for color coding these names are reflected in the mType so that they display nicely.
- **10-bit Breakdown**: Shows 5b/6b|3b/4b structure with binary representation
- **Export Details**: Includes 10-bit code, 5b/6b portion, 3b/4b portion in CSV output

### Supported Symbols
- **K28.5** (0xBC): Primary synchronization comma
- **K28.0-K28.7**: Additional control characters
- **K23.7**: Additional control character
- **K27.7**: Additional control character
- **K29.7**: Additional control character
- **K30.7**: Additional control character
- **D0.0-D31.0**: Complete x.0 subblock data characters
- **D0.1-D31.1**: Complete x.1 subblock data characters  
- **D0.2-D31.2**: Complete x.2 subblock data characters
- **D0.3-D31.3**: Complete x.3 subblock data characters
- **D0.4-D31.4**: Complete x.4 subblock data characters
- **D0.5-D31.5**: Complete x.5 subblock data characters
- **D0.6-D31.6**: Complete x.6 subblock data characters
- **D0.7-D31.7**: Complete x.7 subblock data characters
- **Comprehensive Coverage**: Supports all used 8b/10b symbols

### Test Data
The simulation generator creates test patterns with:
- K28.5 synchronization sequences
- Mixed control and data characters from multiple subblocks
- Proper 8b/10b encoding with disparity tracking
- Test patterns covering D-characters from .0, .1, .2, .4, .5, .6 and .7 subblocks

## Configuration

The analyzer accepts configuration through the settings class, including:
- Input channel selection
- Bit rate configuration (1 to 25M bits/sec)
- Export format options

No lint or test commands are defined in this project - it follows the standard Saleae SDK pattern focused on the analyzer implementation.

## Usage Notes

### Data Processing
- Uses sliding 10-bit window to initially find K28.5 comma characters at any bit position to determine
proper symbol boundaries.
- Invalid symbols are marked as errors but don't stop analysis

### Expected Performance
- Works best with clean 8b/10b signals containing comma characters
- Re-synchronization attempts are made when errors are detected
- Sample rate should be at least 4x the bit rate for reliable decoding