#include "decode8b10bAnalyzer.h"
#include "decode8b10bAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <vector>
#include <map>

decode8b10bAnalyzer::decode8b10bAnalyzer()
:	Analyzer2(),  
	mSettings(),
	mSimulationInitialized( false )
{
	SetAnalyzerSettings( &mSettings );
	// Add this line to your Analyzer constructor!
    UseFrameV2();
}

decode8b10bAnalyzer::~decode8b10bAnalyzer()
{
	KillThread();
}

void decode8b10bAnalyzer::SetupResults()
{
	// SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the results each time.
	mResults.reset(new decode8b10bAnalyzerResults( this, &mSettings ));
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings.mInputChannel );
}

void decode8b10bAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	mSerial = GetAnalyzerChannelData( mSettings.mInputChannel );

	U32 samples_per_bit = mSampleRateHz / mSettings.mBitRate;
	U32 samples_to_bit_center =  U32( 1.5 * double( mSampleRateHz ) / double( mSettings.mBitRate ) );

	// K28.5 comma patterns for synchronization
	const U16 K28_5_RD_MINUS = 0xFA;   // K28.5 RD- as sampled (LSB left)
	const U16 K28_5_RD_PLUS = 0x305;   // K28.5 RD+ as sampled (LSB left)
	
	// Function to reverse bit order of a 10-bit value
	auto reverse_10_bits = [](U16 value) -> U16 {
		U16 reversed = 0;
		for(int i = 0; i < 10; i++) {
			if(value & (1 << i)) {
				reversed |= (1 << (9 - i));
			}
		}
		return reversed;
	};

	// Utility function: reverse lookup table (decoded octet -> code-word name)
	auto get_symbol_name = [](U8 decoded_octet) -> const char* {
		static const std::map<U8, const char*> name_map = {
			// D-characters (Dx.y format where decoded_octet = (x << 3) | y)
			{0x00, "D0.0"}, {0x01, "D1.0"}, {0x02, "D2.0"}, {0x03, "D3.0"},
			{0x04, "D4.0"}, {0x05, "D5.0"}, {0x06, "D6.0"}, {0x07, "D7.0"},
			{0x08, "D8.0"}, {0x09, "D9.0"}, {0x0A, "D10.0"}, {0x0B, "D11.0"},
			{0x0C, "D12.0"}, {0x0D, "D13.0"}, {0x0E, "D14.0"}, {0x0F, "D15.0"},
			{0x10, "D16.0"}, {0x11, "D17.0"}, {0x12, "D18.0"}, {0x13, "D19.0"},
			{0x14, "D20.0"}, {0x15, "D21.0"}, {0x16, "D22.0"}, {0x17, "D23.0"},
			{0x18, "D24.0"}, {0x19, "D25.0"}, {0x1A, "D26.0"}, {0x1B, "D27.0"},
			{0x1C, "D28.0"}, {0x1D, "D29.0"}, {0x1E, "D30.0"}, {0x1F, "D31.0"},
			
			{0x20, "D0.1"}, {0x21, "D1.1"}, {0x22, "D2.1"}, {0x23, "D3.1"},
			{0x24, "D4.1"}, {0x25, "D5.1"}, {0x26, "D6.1"}, {0x27, "D7.1"},
			{0x28, "D8.1"}, {0x29, "D9.1"}, {0x2A, "D10.1"}, {0x2B, "D11.1"},
			{0x2C, "D12.1"}, {0x2D, "D13.1"}, {0x2E, "D14.1"}, {0x2F, "D15.1"},
			{0x30, "D16.1"}, {0x31, "D17.1"}, {0x32, "D18.1"}, {0x33, "D19.1"},
			{0x34, "D20.1"}, {0x35, "D21.1"}, {0x36, "D22.1"}, {0x37, "D23.1"},
			{0x38, "D24.1"}, {0x39, "D25.1"}, {0x3A, "D26.1"}, {0x3B, "D27.1"},
			{0x3C, "D28.1"}, {0x3D, "D29.1"}, {0x3E, "D30.1"}, {0x3F, "D31.1"},
			
			{0x40, "D0.2"}, {0x41, "D1.2"}, {0x42, "D2.2"}, {0x43, "D3.2"},
			{0x44, "D4.2"}, {0x45, "D5.2"}, {0x46, "D6.2"}, {0x47, "D7.2"},
			{0x48, "D8.2"}, {0x49, "D9.2"}, {0x4A, "D10.2"}, {0x4B, "D11.2"},
			{0x4C, "D12.2"}, {0x4D, "D13.2"}, {0x4E, "D14.2"}, {0x4F, "D15.2"},
			{0x50, "D16.2"}, {0x51, "D17.2"}, {0x52, "D18.2"}, {0x53, "D19.2"},
			{0x54, "D20.2"}, {0x55, "D21.2"}, {0x56, "D22.2"}, {0x57, "D23.2"},
			{0x58, "D24.2"}, {0x59, "D25.2"}, {0x5A, "D26.2"}, {0x5B, "D27.2"},
			{0x5C, "D28.2"}, {0x5D, "D29.2"}, {0x5E, "D30.2"}, {0x5F, "D31.2"},
			
			{0x60, "D0.3"}, {0x61, "D1.3"}, {0x62, "D2.3"}, {0x63, "D3.3"},
			{0x64, "D4.3"}, {0x65, "D5.3"}, {0x66, "D6.3"}, {0x67, "D7.3"},
			{0x68, "D8.3"}, {0x69, "D9.3"}, {0x6A, "D10.3"}, {0x6B, "D11.3"},
			{0x6C, "D12.3"}, {0x6D, "D13.3"}, {0x6E, "D14.3"}, {0x6F, "D15.3"},
			{0x70, "D16.3"}, {0x71, "D17.3"}, {0x72, "D18.3"}, {0x73, "D19.3"},
			{0x74, "D20.3"}, {0x75, "D21.3"}, {0x76, "D22.3"}, {0x77, "D23.3"},
			{0x78, "D24.3"}, {0x79, "D25.3"}, {0x7A, "D26.3"}, {0x7B, "D27.3"},
			{0x7C, "D28.3"}, {0x7D, "D29.3"}, {0x7E, "D30.3"}, {0x7F, "D31.3"},
			
			{0x80, "D0.4"}, {0x81, "D1.4"}, {0x82, "D2.4"}, {0x83, "D3.4"},
			{0x84, "D4.4"}, {0x85, "D5.4"}, {0x86, "D6.4"}, {0x87, "D7.4"},
			{0x88, "D8.4"}, {0x89, "D9.4"}, {0x8A, "D10.4"}, {0x8B, "D11.4"},
			{0x8C, "D12.4"}, {0x8D, "D13.4"}, {0x8E, "D14.4"}, {0x8F, "D15.4"},
			{0x90, "D16.4"}, {0x91, "D17.4"}, {0x92, "D18.4"}, {0x93, "D19.4"},
			{0x94, "D20.4"}, {0x95, "D21.4"}, {0x96, "D22.4"}, {0x97, "D23.4"},
			{0x98, "D24.4"}, {0x99, "D25.4"}, {0x9A, "D26.4"}, {0x9B, "D27.4"},
			{0x9C, "D28.4"}, {0x9D, "D29.4"}, {0x9E, "D30.4"}, {0x9F, "D31.4"},
			
			{0xA0, "D0.5"}, {0xA1, "D1.5"}, {0xA2, "D2.5"}, {0xA3, "D3.5"},
			{0xA4, "D4.5"}, {0xA5, "D5.5"}, {0xA6, "D6.5"}, {0xA7, "D7.5"},
			{0xA8, "D8.5"}, {0xA9, "D9.5"}, {0xAA, "D10.5"}, {0xAB, "D11.5"},
			{0xAC, "D12.5"}, {0xAD, "D13.5"}, {0xAE, "D14.5"}, {0xAF, "D15.5"},
			{0xB0, "D16.5"}, {0xB1, "D17.5"}, {0xB2, "D18.5"}, {0xB3, "D19.5"},
			{0xB4, "D20.5"}, {0xB5, "D21.5"}, {0xB6, "D22.5"}, {0xB7, "D23.5"},
			{0xB8, "D24.5"}, {0xB9, "D25.5"}, {0xBA, "D26.5"}, {0xBB, "D27.5"},
			{0xBC, "D28.5"}, {0xBD, "D29.5"}, {0xBE, "D30.5"}, {0xBF, "D31.5"},
			
			{0xC0, "D0.6"}, {0xC1, "D1.6"}, {0xC2, "D2.6"}, {0xC3, "D3.6"},
			{0xC4, "D4.6"}, {0xC5, "D5.6"}, {0xC6, "D6.6"}, {0xC7, "D7.6"},
			{0xC8, "D8.6"}, {0xC9, "D9.6"}, {0xCA, "D10.6"}, {0xCB, "D11.6"},
			{0xCC, "D12.6"}, {0xCD, "D13.6"}, {0xCE, "D14.6"}, {0xCF, "D15.6"},
			{0xD0, "D16.6"}, {0xD1, "D17.6"}, {0xD2, "D18.6"}, {0xD3, "D19.6"},
			{0xD4, "D20.6"}, {0xD5, "D21.6"}, {0xD6, "D22.6"}, {0xD7, "D23.6"},
			{0xD8, "D24.6"}, {0xD9, "D25.6"}, {0xDA, "D26.6"}, {0xDB, "D27.6"},
			{0xDC, "D28.6"}, {0xDD, "D29.6"}, {0xDE, "D30.6"}, {0xDF, "D31.6"},
			
			{0xE0, "D0.7"}, {0xE1, "D1.7"}, {0xE2, "D2.7"}, {0xE3, "D3.7"},
			{0xE4, "D4.7"}, {0xE5, "D5.7"}, {0xE6, "D6.7"}, {0xE7, "D7.7"},
			{0xE8, "D8.7"}, {0xE9, "D9.7"}, {0xEA, "D10.7"}, {0xEB, "D11.7"},
			{0xEC, "D12.7"}, {0xED, "D13.7"}, {0xEE, "D14.7"}, {0xEF, "D15.7"},
			{0xF0, "D16.7"}, {0xF1, "D17.7"}, {0xF2, "D18.7"}, {0xF3, "D19.7"},
			{0xF4, "D20.7"}, {0xF5, "D21.7"}, {0xF6, "D22.7"}, {0xF7, "D23.7"},
			{0xF8, "D24.7"}, {0xF9, "D25.7"}, {0xFA, "D26.7"}, {0xFB, "D27.7"},
			{0xFC, "D28.7"}, {0xFD, "D29.7"}, {0xFE, "D30.7"}, {0xFF, "D31.7"},
			
			// K-characters (special control codes)
			{0x11C, "K28.0"}, {0x13C, "K28.1"}, {0x15C, "K28.2"}, {0x17C, "K28.3"},
			{0x19C, "K28.4"}, {0x1BC, "K28.5"}, {0x1DC, "K28.6"}, {0x1FC, "K28.7"},
			{0x1F7, "K23.7"}, {0x1FB, "K27.7"}, {0x1FD, "K29.7"}, {0x1FE, "K30.7"}
		};
		
		auto it = name_map.find(decoded_octet);
		if (it != name_map.end()) {
			return it->second;
		}
		return "UNKNOWN";
	};


	// Complete 8b/10b decode table using actual observed patterns
	auto decode_symbol = [](U16 ten_bit_code) -> std::pair<U8, bool> {
		
		// Complete 8b/10b lookup table using verified patterns for LSB-first sampling
		// Based on working K28.5 and observed D-character patterns
		static const std::map<U16, U8> decode_map = {
		
			
			{0x274, 0x00}, {0x18b, 0x00}, // D0.0: 0b1001110100 RD-, 0b0110001011 RD+
			{0x1d4, 0x01}, {0x22b, 0x01}, // D1.0: 0b0111010100 RD-, 0b1000101011 RD+
			{0x2d4, 0x02}, {0x12b, 0x02}, // D2.0: 0b1011010100 RD-, 0b0100101011 RD+
			{0x31b, 0x03}, {0x314, 0x03}, // D3.0: 0b1100011011 RD-, 0b1100010100 RD+
			{0x354, 0x04}, {0x0ab, 0x04}, // D4.0: 0b1101010100 RD-, 0b0010101011 RD+
			{0x29b, 0x05}, {0x294, 0x05}, // D5.0: 0b1010011011 RD-, 0b1010010100 RD+
			{0x19b, 0x06}, {0x194, 0x06}, // D6.0: 0b0110011011 RD-, 0b0110010100 RD+
			{0x38b, 0x07}, {0x074, 0x07}, // D7.0: 0b1110001011 RD-, 0b0001110100 RD+
			{0x394, 0x08}, {0x06b, 0x08}, // D8.0: 0b1110010100 RD-, 0b0001101011 RD+
			{0x25b, 0x09}, {0x254, 0x09}, // D9.0: 0b1001011011 RD-, 0b1001010100 RD+
			{0x15b, 0x0a}, {0x154, 0x0a}, // D10.0: 0b0101011011 RD-, 0b0101010100 RD+
			{0x34b, 0x0b}, {0x344, 0x0b}, // D11.0: 0b1101001011 RD-, 0b1101000100 RD+
			{0x0db, 0x0c}, {0x0d4, 0x0c}, // D12.0: 0b0011011011 RD-, 0b0011010100 RD+
			{0x2cb, 0x0d}, {0x2c4, 0x0d}, // D13.0: 0b1011001011 RD-, 0b1011000100 RD+
			{0x1cb, 0x0e}, {0x1c4, 0x0e}, // D14.0: 0b0111001011 RD-, 0b0111000100 RD+
			{0x174, 0x0f}, {0x28b, 0x0f}, // D15.0: 0b0101110100 RD-, 0b1010001011 RD+
			{0x1b4, 0x10}, {0x24b, 0x10}, // D16.0: 0b0110110100 RD-, 0b1001001011 RD+
			{0x23b, 0x11}, {0x234, 0x11}, // D17.0: 0b1000111011 RD-, 0b1000110100 RD+
			{0x13b, 0x12}, {0x134, 0x12}, // D18.0: 0b0100111011 RD-, 0b0100110100 RD+
			{0x32b, 0x13}, {0x324, 0x13}, // D19.0: 0b1100101011 RD-, 0b1100100100 RD+
			{0x0bb, 0x14}, {0x0b4, 0x14}, // D20.0: 0b0010111011 RD-, 0b0010110100 RD+
			{0x2ab, 0x15}, {0x2a4, 0x15}, // D21.0: 0b1010101011 RD-, 0b1010100100 RD+
			{0x1ab, 0x16}, {0x1a4, 0x16}, // D22.0: 0b0110101011 RD-, 0b0110100100 RD+
			{0x3a4, 0x17}, {0x05b, 0x17}, // D23.0: 0b1110100100 RD-, 0b0001011011 RD+
			{0x334, 0x18}, {0x0cb, 0x18}, // D24.0: 0b1100110100 RD-, 0b0011001011 RD+
			{0x26b, 0x19}, {0x264, 0x19}, // D25.0: 0b1001101011 RD-, 0b1001100100 RD+
			{0x16b, 0x1a}, {0x164, 0x1a}, // D26.0: 0b0101101011 RD-, 0b0101100100 RD+
			{0x364, 0x1b}, {0x09b, 0x1b}, // D27.0: 0b1101100100 RD-, 0b0010011011 RD+
			{0x0eb, 0x1c}, {0x0e4, 0x1c}, // D28.0: 0b0011101011 RD-, 0b0011100100 RD+
			{0x2e4, 0x1d}, {0x11b, 0x1d}, // D29.0: 0b1011100100 RD-, 0b0100011011 RD+
			{0x1e4, 0x1e}, {0x21b, 0x1e}, // D30.0: 0b0111100100 RD-, 0b1000011011 RD+
			{0x2b4, 0x1f}, {0x14b, 0x1f}, // D31.0: 0b1010110100 RD-, 0b0101001011 RD+
			{0x279, 0x20}, {0x189, 0x20}, // D00.1: 0b1001111001 RD-, 0b0110001001 RD+
			{0x1d9, 0x21}, {0x229, 0x21}, // D01.1: 0b0111011001 RD-, 0b1000101001 RD+
			{0x2d9, 0x22}, {0x129, 0x22}, // D02.1: 0b1011011001 RD-, 0b0100101001 RD+
			{0x319, 0x23}, {0x319, 0x23}, // D03.1: 0b1100011001 RD-, 0b1100011001 RD+
			{0x359, 0x24}, {0x0a9, 0x24}, // D04.1: 0b1101011001 RD-, 0b0010101001 RD+
			{0x299, 0x25}, {0x299, 0x25}, // D05.1: 0b1010011001 RD-, 0b1010011001 RD+
			{0x199, 0x26}, {0x199, 0x26}, // D06.1: 0b0110011001 RD-, 0b0110011001 RD+
			{0x389, 0x27}, {0x079, 0x27}, // D07.1: 0b1110001001 RD-, 0b0001111001 RD+
			{0x399, 0x28}, {0x069, 0x28}, // D08.1: 0b1110011001 RD-, 0b0001101001 RD+
			{0x259, 0x29}, {0x259, 0x29}, // D09.1: 0b1001011001 RD-, 0b1001011001 RD+
			{0x159, 0x2a}, {0x159, 0x2a}, // D10.1: 0b0101011001 RD-, 0b0101011001 RD+
			{0x349, 0x2b}, {0x349, 0x2b}, // D11.1: 0b1101001001 RD-, 0b1101001001 RD+
			{0x0d9, 0x2c}, {0x0d9, 0x2c}, // D12.1: 0b0011011001 RD-, 0b0011011001 RD+
			{0x2c9, 0x2d}, {0x2c9, 0x2d}, // D13.1: 0b1011001001 RD-, 0b1011001001 RD+
			{0x1c9, 0x2e}, {0x1c9, 0x2e}, // D14.1: 0b0111001001 RD-, 0b0111001001 RD+
			{0x179, 0x2f}, {0x289, 0x2f}, // D15.1: 0b0101111001 RD-, 0b1010001001 RD+
			{0x1b9, 0x30}, {0x249, 0x30}, // D16.1: 0b0110111001 RD-, 0b1001001001 RD+
			{0x239, 0x31}, {0x239, 0x31}, // D17.1: 0b1000111001 RD-, 0b1000111001 RD+
			{0x139, 0x32}, {0x139, 0x32}, // D18.1: 0b0100111001 RD-, 0b0100111001 RD+
			{0x329, 0x33}, {0x329, 0x33}, // D19.1: 0b1100101001 RD-, 0b1100101001 RD+
			{0x0b9, 0x34}, {0x0b9, 0x34}, // D20.1: 0b0010111001 RD-, 0b0010111001 RD+
			{0x2a9, 0x35}, {0x2a9, 0x35}, // D21.1: 0b1010101001 RD-, 0b1010101001 RD+
			{0x1a9, 0x36}, {0x1a9, 0x36}, // D22.1: 0b0110101001 RD-, 0b0110101001 RD+
			{0x3a9, 0x37}, {0x059, 0x37}, // D23.1: 0b1110101001 RD-, 0b0001011001 RD+
			{0x339, 0x38}, {0x0c9, 0x38}, // D24.1: 0b1100111001 RD-, 0b0011001001 RD+
			{0x269, 0x39}, {0x269, 0x39}, // D25.1: 0b1001101001 RD-, 0b1001101001 RD+
			{0x169, 0x3a}, {0x169, 0x3a}, // D26.1: 0b0101101001 RD-, 0b0101101001 RD+
			{0x369, 0x3b}, {0x099, 0x3b}, // D27.1: 0b1101101001 RD-, 0b0010011001 RD+
			{0x0e9, 0x3c}, {0x0e9, 0x3c}, // D28.1: 0b0011101001 RD-, 0b0011101001 RD+
			{0x2e9, 0x3d}, {0x119, 0x3d}, // D29.1: 0b1011101001 RD-, 0b0100011001 RD+
			{0x1e9, 0x3e}, {0x219, 0x3e}, // D30.1: 0b0111101001 RD-, 0b1000011001 RD+
			{0x2b9, 0x3f}, {0x149, 0x3f}, // D31.1: 0b1010111001 RD-, 0b0101001001 RD+
			{0x275, 0x40}, {0x185, 0x40}, // D0.2: 0b1001110101 RD-, 0b0110000101 RD+
			{0x1d5, 0x41}, {0x225, 0x41}, // D1.2: 0b0111010101 RD-, 0b1000100101 RD+
			{0x2d5, 0x42}, {0x125, 0x42}, // D2.2: 0b1011010101 RD-, 0b0100100101 RD+
			{0x315, 0x43}, {0x315, 0x43}, // D3.2: 0b1100010101 RD-, 0b1100010101 RD+
			{0x355, 0x44}, {0x0a5, 0x44}, // D4.2: 0b1101010101 RD-, 0b0010100101 RD+
			{0x295, 0x45}, {0x295, 0x45}, // D5.2: 0b1010010101 RD-, 0b1010010101 RD+
			{0x195, 0x46}, {0x195, 0x46}, // D6.2: 0b0110010101 RD-, 0b0110010101 RD+
			{0x385, 0x47}, {0x075, 0x47}, // D7.2: 0b1110000101 RD-, 0b0001110101 RD+
			{0x395, 0x48}, {0x065, 0x48}, // D8.2: 0b1110010101 RD-, 0b0001100101 RD+
			{0x255, 0x49}, {0x255, 0x49}, // D9.2: 0b1001010101 RD-, 0b1001010101 RD+
			{0x155, 0x4a}, {0x155, 0x4a}, // D10.2: 0b0101010101 RD-, 0b0101010101 RD+
			{0x345, 0x4b}, {0x345, 0x4b}, // D11.2: 0b1101000101 RD-, 0b1101000101 RD+
			{0x0d5, 0x4c}, {0x0d5, 0x4c}, // D12.2: 0b0011010101 RD-, 0b0011010101 RD+
			{0x2c5, 0x4d}, {0x2c5, 0x4d}, // D13.2: 0b1011000101 RD-, 0b1011000101 RD+
			{0x1c5, 0x4e}, {0x1c5, 0x4e}, // D14.2: 0b0111000101 RD-, 0b0111000101 RD+
			{0x175, 0x4f}, {0x285, 0x4f}, // D15.2: 0b0101110101 RD-, 0b1010000101 RD+
			{0x1b5, 0x50}, {0x245, 0x50}, // D16.2: 0b0110110101 RD-, 0b1001000101 RD+
			{0x235, 0x51}, {0x235, 0x51}, // D17.2: 0b1000110101 RD-, 0b1000110101 RD+
			{0x135, 0x52}, {0x135, 0x52}, // D18.2: 0b0100110101 RD-, 0b0100110101 RD+
			{0x325, 0x53}, {0x325, 0x53}, // D19.2: 0b1100100101 RD-, 0b1100100101 RD+
			{0x0b5, 0x54}, {0x0b5, 0x54}, // D20.2: 0b0010110101 RD-, 0b0010110101 RD+
			{0x2a5, 0x55}, {0x2a5, 0x55}, // D21.2: 0b1010100101 RD-, 0b1010100101 RD+
			{0x1a5, 0x56}, {0x1a5, 0x56}, // D22.2: 0b0110100101 RD-, 0b0110100101 RD+
			{0x3a5, 0x57}, {0x055, 0x57}, // D23.2: 0b1110100101 RD-, 0b0001010101 RD+
			{0x335, 0x58}, {0x0c5, 0x58}, // D24.2: 0b1100110101 RD-, 0b0011000101 RD+
			{0x265, 0x59}, {0x265, 0x59}, // D25.2: 0b1001100101 RD-, 0b1001100101 RD+
			{0x165, 0x5a}, {0x165, 0x5a}, // D26.2: 0b0101100101 RD-, 0b0101100101 RD+
			{0x365, 0x5b}, {0x095, 0x5b}, // D27.2: 0b1101100101 RD-, 0b0010010101 RD+
			{0x0e5, 0x5c}, {0x0e5, 0x5c}, // D28.2: 0b0011100101 RD-, 0b0011100101 RD+
			{0x2e5, 0x5d}, {0x115, 0x5d}, // D29.2: 0b1011100101 RD-, 0b0100010101 RD+
			{0x1e5, 0x5e}, {0x215, 0x5e}, // D30.2: 0b0111100101 RD-, 0b1000010101 RD+
			{0x2b5, 0x5f}, {0x145, 0x5f}, // D31.2: 0b1010110101 RD-, 0b0101000101 RD+
			{0x273, 0x60}, {0x18c, 0x60}, // D0.3: 0b1001110011 RD-, 0b0110001100 RD+
			{0x1d3, 0x61}, {0x22c, 0x61}, // D1.3: 0b0111010011 RD-, 0b1000101100 RD+
			{0x2d3, 0x62}, {0x12c, 0x62}, // D2.3: 0b1011010011 RD-, 0b0100101100 RD+
			{0x31c, 0x63}, {0x313, 0x63}, // D3.3: 0b1100011100 RD-, 0b1100010011 RD+
			{0x353, 0x64}, {0x0ac, 0x64}, // D4.3: 0b1101010011 RD-, 0b0010101100 RD+
			{0x29c, 0x65}, {0x293, 0x65}, // D5.3: 0b1010011100 RD-, 0b1010010011 RD+
			{0x19c, 0x66}, {0x193, 0x66}, // D6.3: 0b0110011100 RD-, 0b0110010011 RD+
			{0x38c, 0x67}, {0x073, 0x67}, // D7.3: 0b1110001100 RD-, 0b0001110011 RD+
			{0x393, 0x68}, {0x06c, 0x68}, // D8.3: 0b1110010011 RD-, 0b0001101100 RD+
			{0x25c, 0x69}, {0x253, 0x69}, // D9.3: 0b1001011100 RD-, 0b1001010011 RD+
			{0x15c, 0x6a}, {0x153, 0x6a}, // D10.3: 0b0101011100 RD-, 0b0101010011 RD+
			{0x34c, 0x6b}, {0x343, 0x6b}, // D11.3: 0b1101001100 RD-, 0b1101000011 RD+
			{0x0dc, 0x6c}, {0x0d3, 0x6c}, // D12.3: 0b0011011100 RD-, 0b0011010011 RD+
			{0x2cc, 0x6d}, {0x2c3, 0x6d}, // D13.3: 0b1011001100 RD-, 0b1011000011 RD+
			{0x1cc, 0x6e}, {0x1c3, 0x6e}, // D14.3: 0b0111001100 RD-, 0b0111000011 RD+
			{0x173, 0x6f}, {0x28c, 0x6f}, // D15.3: 0b0101110011 RD-, 0b1010001100 RD+
			{0x1b3, 0x70}, {0x24c, 0x70}, // D16.3: 0b0110110011 RD-, 0b1001001100 RD+
			{0x23c, 0x71}, {0x233, 0x71}, // D17.3: 0b1000111100 RD-, 0b1000110011 RD+
			{0x13c, 0x72}, {0x133, 0x72}, // D18.3: 0b0100111100 RD-, 0b0100110011 RD+
			{0x32c, 0x73}, {0x323, 0x73}, // D19.3: 0b1100101100 RD-, 0b1100100011 RD+
			{0x0bc, 0x74}, {0x0b3, 0x74}, // D20.3: 0b0010111100 RD-, 0b0010110011 RD+
			{0x2ac, 0x75}, {0x2a3, 0x75}, // D21.3: 0b1010101100 RD-, 0b1010100011 RD+
			{0x1ac, 0x76}, {0x1a3, 0x76}, // D22.3: 0b0110101100 RD-, 0b0110100011 RD+
			{0x3a3, 0x77}, {0x05c, 0x77}, // D23.3: 0b1110100011 RD-, 0b0001011100 RD+
			{0x333, 0x78}, {0x0cc, 0x78}, // D24.3: 0b1100110011 RD-, 0b0011001100 RD+
			{0x26c, 0x79}, {0x263, 0x79}, // D25.3: 0b1001101100 RD-, 0b1001100011 RD+
			{0x16c, 0x7a}, {0x163, 0x7a}, // D26.3: 0b0101101100 RD-, 0b0101100011 RD+
			{0x363, 0x7b}, {0x09c, 0x7b}, // D27.3: 0b1101100011 RD-, 0b0010011100 RD+
			{0x0ec, 0x7c}, {0x0e3, 0x7c}, // D28.3: 0b0011101100 RD-, 0b0011100011 RD+
			{0x2e3, 0x7d}, {0x11c, 0x7d}, // D29.3: 0b1011100011 RD-, 0b0100011100 RD+
			{0x1e3, 0x7e}, {0x21c, 0x7e}, // D30.3: 0b0111100011 RD-, 0b1000011100 RD+
			{0x2b3, 0x7f}, {0x14c, 0x7f}, // D31.3: 0b1010110011 RD-, 0b0101001100 RD+
			{0x272, 0x80}, {0x18d, 0x80}, // D00.4: 0b1001110010 RD-, 0b0110001101 RD+
			{0x1d2, 0x81}, {0x22d, 0x81}, // D01.4: 0b0111010010 RD-, 0b1000101101 RD+
			{0x2d2, 0x82}, {0x12d, 0x82}, // D02.4: 0b1011010010 RD-, 0b0100101101 RD+
			{0x31d, 0x83}, {0x312, 0x83}, // D03.4: 0b1100011101 RD-, 0b1100010010 RD+
			{0x352, 0x84}, {0x0ad, 0x84}, // D04.4: 0b1101010010 RD-, 0b0010101101 RD+
			{0x29d, 0x85}, {0x292, 0x85}, // D05.4: 0b1010011101 RD-, 0b1010010010 RD+
			{0x19d, 0x86}, {0x192, 0x86}, // D06.4: 0b0110011101 RD-, 0b0110010010 RD+
			{0x38d, 0x87}, {0x072, 0x87}, // D07.4: 0b1110001101 RD-, 0b0001110010 RD+
			{0x392, 0x88}, {0x06d, 0x88}, // D08.4: 0b1110010010 RD-, 0b0001101101 RD+
			{0x25d, 0x89}, {0x252, 0x89}, // D09.4: 0b1001011101 RD-, 0b1001010010 RD+
			{0x15d, 0x8a}, {0x152, 0x8a}, // D10.4: 0b0101011101 RD-, 0b0101010010 RD+
			{0x34d, 0x8b}, {0x342, 0x8b}, // D11.4: 0b1101001101 RD-, 0b1101000010 RD+
			{0x0dd, 0x8c}, {0x0d2, 0x8c}, // D12.4: 0b0011011101 RD-, 0b0011010010 RD+
			{0x2cd, 0x8d}, {0x2c2, 0x8d}, // D13.4: 0b1011001101 RD-, 0b1011000010 RD+
			{0x1cd, 0x8e}, {0x1c2, 0x8e}, // D14.4: 0b0111001101 RD-, 0b0111000010 RD+
			{0x172, 0x8f}, {0x28d, 0x8f}, // D15.4: 0b0101110010 RD-, 0b1010001101 RD+
			{0x1b2, 0x90}, {0x24d, 0x90}, // D16.4: 0b0110110010 RD-, 0b1001001101 RD+
			{0x23d, 0x91}, {0x232, 0x91}, // D17.4: 0b1000111101 RD-, 0b1000110010 RD+
			{0x13d, 0x92}, {0x132, 0x92}, // D18.4: 0b0100111101 RD-, 0b0100110010 RD+
			{0x32d, 0x93}, {0x322, 0x93}, // D19.4: 0b1100101101 RD-, 0b1100100010 RD+
			{0x0bd, 0x94}, {0x0b2, 0x94}, // D20.4: 0b0010111101 RD-, 0b0010110010 RD+
			{0x2ad, 0x95}, {0x2a2, 0x95}, // D21.4: 0b1010101101 RD-, 0b1010100010 RD+
			{0x1ad, 0x96}, {0x1a2, 0x96}, // D22.4: 0b0110101101 RD-, 0b0110100010 RD+
			{0x3a2, 0x97}, {0x05d, 0x97}, // D23.4: 0b1110100010 RD-, 0b0001011101 RD+
			{0x332, 0x98}, {0x0cd, 0x98}, // D24.4: 0b1100110010 RD-, 0b0011001101 RD+
			{0x26d, 0x99}, {0x262, 0x99}, // D25.4: 0b1001101101 RD-, 0b1001100010 RD+
			{0x16d, 0x9a}, {0x162, 0x9a}, // D26.4: 0b0101101101 RD-, 0b0101100010 RD+
			{0x362, 0x9b}, {0x09d, 0x9b}, // D27.4: 0b1101100010 RD-, 0b0010011101 RD+
			{0x0ed, 0x9c}, {0x0e2, 0x9c}, // D28.4: 0b0011101101 RD-, 0b0011100010 RD+
			{0x2e2, 0x9d}, {0x11d, 0x9d}, // D29.4: 0b1011100010 RD-, 0b0100011101 RD+
			{0x1e2, 0x9e}, {0x21d, 0x9e}, // D30.4: 0b0111100010 RD-, 0b1000011101 RD+
			{0x2b2, 0x9f}, {0x14d, 0x9f}, // D31.4: 0b1010110010 RD-, 0b0101001101 RD+
			{0x27a, 0xa0}, {0x18a, 0xa0}, // D00.5: 0b1001111010 RD-, 0b0110001010 RD+
			{0x1da, 0xa1}, {0x22a, 0xa1}, // D01.5: 0b0111011010 RD-, 0b1000101010 RD+
			{0x2da, 0xa2}, {0x12a, 0xa2}, // D02.5: 0b1011011010 RD-, 0b0100101010 RD+
			{0x31a, 0xa3}, {0x31a, 0xa3}, // D03.5: 0b1100011010 RD-, 0b1100011010 RD+
			{0x35a, 0xa4}, {0x0aa, 0xa4}, // D04.5: 0b1101011010 RD-, 0b0010101010 RD+
			{0x29a, 0xa5}, {0x29a, 0xa5}, // D05.5: 0b1010011010 RD-, 0b1010011010 RD+
			{0x19a, 0xa6}, {0x19a, 0xa6}, // D06.5: 0b0110011010 RD-, 0b0110011010 RD+
			{0x38a, 0xa7}, {0x07a, 0xa7}, // D07.5: 0b1110001010 RD-, 0b0001111010 RD+
			{0x39a, 0xa8}, {0x06a, 0xa8}, // D08.5: 0b1110011010 RD-, 0b0001101010 RD+
			{0x25a, 0xa9}, {0x25a, 0xa9}, // D09.5: 0b1001011010 RD-, 0b1001011010 RD+
			{0x15a, 0xaa}, {0x15a, 0xaa}, // D10.5: 0b0101011010 RD-, 0b0101011010 RD+
			{0x34a, 0xab}, {0x34a, 0xab}, // D11.5: 0b1101001010 RD-, 0b1101001010 RD+
			{0x0da, 0xac}, {0x0da, 0xac}, // D12.5: 0b0011011010 RD-, 0b0011011010 RD+
			{0x2ca, 0xad}, {0x2ca, 0xad}, // D13.5: 0b1011001010 RD-, 0b1011001010 RD+
			{0x1ca, 0xae}, {0x1ca, 0xae}, // D14.5: 0b0111001010 RD-, 0b0111001010 RD+
			{0x17a, 0xaf}, {0x28a, 0xaf}, // D15.5: 0b0101111010 RD-, 0b1010001010 RD+
			{0x1ba, 0xb0}, {0x24a, 0xb0}, // D16.5: 0b0110111010 RD-, 0b1001001010 RD+
			{0x23a, 0xb1}, {0x23a, 0xb1}, // D17.5: 0b1000111010 RD-, 0b1000111010 RD+
			{0x13a, 0xb2}, {0x13a, 0xb2}, // D18.5: 0b0100111010 RD-, 0b0100111010 RD+
			{0x32a, 0xb3}, {0x32a, 0xb3}, // D19.5: 0b1100101010 RD-, 0b1100101010 RD+
			{0x0ba, 0xb4}, {0x0ba, 0xb4}, // D20.5: 0b0010111010 RD-, 0b0010111010 RD+
			{0x2aa, 0xb5}, {0x2aa, 0xb5}, // D21.5: 0b1010101010 RD-, 0b1010101010 RD+
			{0x1aa, 0xb6}, {0x1aa, 0xb6}, // D22.5: 0b0110101010 RD-, 0b0110101010 RD+
			{0x3aa, 0xb7}, {0x05a, 0xb7}, // D23.5: 0b1110101010 RD-, 0b0001011010 RD+
			{0x33a, 0xb8}, {0x0ca, 0xb8}, // D24.5: 0b1100111010 RD-, 0b0011001010 RD+
			{0x26a, 0xb9}, {0x26a, 0xb9}, // D25.5: 0b1001101010 RD-, 0b1001101010 RD+
			{0x16a, 0xba}, {0x16a, 0xba}, // D26.5: 0b0101101010 RD-, 0b0101101010 RD+
			{0x36a, 0xbb}, {0x09a, 0xbb}, // D27.5: 0b1101101010 RD-, 0b0010011010 RD+
			{0x0ea, 0xbc}, {0x0ea, 0xbc}, // D28.5: 0b0011101010 RD-, 0b0011101010 RD+
			{0x2ea, 0xbd}, {0x11a, 0xbd}, // D29.5: 0b1011101010 RD-, 0b0100011010 RD+
			{0x1ea, 0xbe}, {0x21a, 0xbe}, // D30.5: 0b0111101010 RD-, 0b1000011010 RD+
			{0x2ba, 0xbf}, {0x14a, 0xbf}, // D31.5: 0b1010111010 RD-, 0b0101001010 RD+
			{0x276, 0xc0}, {0x186, 0xc0}, // D00.6: 0b1001110110 RD-, 0b0110000110 RD+
			{0x1d6, 0xc1}, {0x226, 0xc1}, // D01.6: 0b0111010110 RD-, 0b1000100110 RD+
			{0x2d6, 0xc2}, {0x126, 0xc2}, // D02.6: 0b1011010110 RD-, 0b0100100110 RD+
			{0x316, 0xc3}, {0x316, 0xc3}, // D03.6: 0b1100010110 RD-, 0b1100010110 RD+
			{0x356, 0xc4}, {0x0a6, 0xc4}, // D04.6: 0b1101010110 RD-, 0b0010100110 RD+
			{0x296, 0xc5}, {0x296, 0xc5}, // D05.6: 0b1010010110 RD-, 0b1010010110 RD+
			{0x196, 0xc6}, {0x196, 0xc6}, // D06.6: 0b0110010110 RD-, 0b0110010110 RD+
			{0x386, 0xc7}, {0x076, 0xc7}, // D07.6: 0b1110000110 RD-, 0b0001110110 RD+
			{0x396, 0xc8}, {0x066, 0xc8}, // D08.6: 0b1110010110 RD-, 0b0001100110 RD+
			{0x256, 0xc9}, {0x256, 0xc9}, // D09.6: 0b1001010110 RD-, 0b1001010110 RD+
			{0x156, 0xca}, {0x156, 0xca}, // D10.6: 0b0101010110 RD-, 0b0101010110 RD+
			{0x346, 0xcb}, {0x346, 0xcb}, // D11.6: 0b1101000110 RD-, 0b1101000110 RD+
			{0x0d6, 0xcc}, {0x0d6, 0xcc}, // D12.6: 0b0011010110 RD-, 0b0011010110 RD+
			{0x2c6, 0xcd}, {0x2c6, 0xcd}, // D13.6: 0b1011000110 RD-, 0b1011000110 RD+
			{0x1c6, 0xce}, {0x1c6, 0xce}, // D14.6: 0b0111000110 RD-, 0b0111000110 RD+
			{0x176, 0xcf}, {0x286, 0xcf}, // D15.6: 0b0101110110 RD-, 0b1010000110 RD+
			{0x1b6, 0xd0}, {0x246, 0xd0}, // D16.6: 0b0110110110 RD-, 0b1001000110 RD+
			{0x236, 0xd1}, {0x236, 0xd1}, // D17.6: 0b1000110110 RD-, 0b1000110110 RD+
			{0x136, 0xd2}, {0x136, 0xd2}, // D18.6: 0b0100110110 RD-, 0b0100110110 RD+
			{0x326, 0xd3}, {0x326, 0xd3}, // D19.6: 0b1100100110 RD-, 0b1100100110 RD+
			{0x0b6, 0xd4}, {0x0b6, 0xd4}, // D20.6: 0b0010110110 RD-, 0b0010110110 RD+
			{0x2a6, 0xd5}, {0x2a6, 0xd5}, // D21.6: 0b1010100110 RD-, 0b1010100110 RD+
			{0x1a6, 0xd6}, {0x1a6, 0xd6}, // D22.6: 0b0110100110 RD-, 0b0110100110 RD+
			{0x3a6, 0xd7}, {0x056, 0xd7}, // D23.6: 0b1110100110 RD-, 0b0001010110 RD+
			{0x336, 0xd8}, {0x0c6, 0xd8}, // D24.6: 0b1100110110 RD-, 0b0011000110 RD+
			{0x266, 0xd9}, {0x266, 0xd9}, // D25.6: 0b1001100110 RD-, 0b1001100110 RD+
			{0x166, 0xda}, {0x166, 0xda}, // D26.6: 0b0101100110 RD-, 0b0101100110 RD+
			{0x366, 0xdb}, {0x096, 0xdb}, // D27.6: 0b1101100110 RD-, 0b0010010110 RD+
			{0x0e6, 0xdc}, {0x0e6, 0xdc}, // D28.6: 0b0011100110 RD-, 0b0011100110 RD+
			{0x2e6, 0xdd}, {0x116, 0xdd}, // D29.6: 0b1011100110 RD-, 0b0100010110 RD+
			{0x1e6, 0xde}, {0x216, 0xde}, // D30.6: 0b0111100110 RD-, 0b1000010110 RD+
			{0x2b6, 0xdf}, {0x146, 0xdf}, // D31.6: 0b1010110110 RD-, 0b0101000110 RD+
			{0x271, 0xe0}, {0x18e, 0xe0}, // D00.7: 0b1001110001 RD-, 0b0110001110 RD+
			{0x1d1, 0xe1}, {0x22e, 0xe1}, // D01.7: 0b0111010001 RD-, 0b1000101110 RD+
			{0x2d1, 0xe2}, {0x12e, 0xe2}, // D02.7: 0b1011010001 RD-, 0b0100101110 RD+
			{0x31e, 0xe3}, {0x311, 0xe3}, // D03.7: 0b1100011110 RD-, 0b1100010001 RD+
			{0x351, 0xe4}, {0x0ae, 0xe4}, // D04.7: 0b1101010001 RD-, 0b0010101110 RD+
			{0x29e, 0xe5}, {0x291, 0xe5}, // D05.7: 0b1010011110 RD-, 0b1010010001 RD+
			{0x19e, 0xe6}, {0x191, 0xe6}, // D06.7: 0b0110011110 RD-, 0b0110010001 RD+
			{0x38e, 0xe7}, {0x071, 0xe7}, // D07.7: 0b1110001110 RD-, 0b0001110001 RD+
			{0x391, 0xe8}, {0x06e, 0xe8}, // D08.7: 0b1110010001 RD-, 0b0001101110 RD+
			{0x25e, 0xe9}, {0x251, 0xe9}, // D09.7: 0b1001011110 RD-, 0b1001010001 RD+
			{0x15e, 0xea}, {0x151, 0xea}, // D10.7: 0b0101011110 RD-, 0b0101010001 RD+
			{0x34e, 0xeb}, {0x348, 0xeb}, // D11.7: 0b1101001110 RD-, 0b1101001000 RD+
			{0x0de, 0xec}, {0x0d1, 0xec}, // D12.7: 0b0011011110 RD-, 0b0011010001 RD+
			{0x2ce, 0xed}, {0x2c8, 0xed}, // D13.7: 0b1011001110 RD-, 0b1011001000 RD+
			{0x1ce, 0xee}, {0x1c8, 0xee}, // D14.7: 0b0111001110 RD-, 0b0111001000 RD+
			{0x171, 0xef}, {0x28e, 0xef}, // D15.7: 0b0101110001 RD-, 0b1010001110 RD+
			{0x1b1, 0xf0}, {0x24e, 0xf0}, // D16.7: 0b0110110001 RD-, 0b1001001110 RD+
			{0x237, 0xf1}, {0x231, 0xf1}, // D17.7: 0b1000110111 RD-, 0b1000110001 RD+
			{0x137, 0xf2}, {0x131, 0xf2}, // D18.7: 0b0100110111 RD-, 0b0100110001 RD+
			{0x32e, 0xf3}, {0x321, 0xf3}, // D19.7: 0b1100101110 RD-, 0b1100100001 RD+
			{0x0b7, 0xf4}, {0x0b1, 0xf4}, // D20.7: 0b0010110111 RD-, 0b0010110001 RD+
			{0x2ae, 0xf5}, {0x2a1, 0xf5}, // D21.7: 0b1010101110 RD-, 0b1010100001 RD+
			{0x1ae, 0xf6}, {0x1a1, 0xf6}, // D22.7: 0b0110101110 RD-, 0b0110100001 RD+
			{0x3a1, 0xf7}, {0x05e, 0xf7}, // D23.7: 0b1110100001 RD-, 0b0001011110 RD+
			{0x331, 0xf8}, {0x0ce, 0xf8}, // D24.7: 0b1100110001 RD-, 0b0011001110 RD+
			{0x26e, 0xf9}, {0x261, 0xf9}, // D25.7: 0b1001101110 RD-, 0b1001100001 RD+
			{0x16e, 0xfa}, {0x161, 0xfa}, // D26.7: 0b0101101110 RD-, 0b0101100001 RD+
			{0x361, 0xfb}, {0x09e, 0xfb}, // D27.7: 0b1101100001 RD-, 0b0010011110 RD+
			{0x0ee, 0xfc}, {0x0e1, 0xfc}, // D28.7: 0b0011101110 RD-, 0b0011100001 RD+
			{0x2e1, 0xfd}, {0x11e, 0xfd}, // D29.7: 0b1011100001 RD-, 0b0100011110 RD+
			{0x1e1, 0xfe}, {0x21e, 0xfe}, // D30.7: 0b0111100001 RD-, 0b1000011110 RD+
			{0x2b1, 0xff}, {0x14e, 0xff}, // D31.7: 0b1010110001 RD-, 0b0101001110 RD+
			{0x0f4, 0x11c}, {0x30b, 0x11c}, // b'K28.0': 0b0011110100 RD-, 0b1100001011 RD+
			{0x0f9, 0x13c}, {0x306, 0x13c}, // b'K28.1': 0b0011111001 RD-, 0b1100000110 RD+
			{0x0f5, 0x15c}, {0x30a, 0x15c}, // b'K28.2': 0b0011110101 RD-, 0b1100001010 RD+
			{0x0f3, 0x17c}, {0x30c, 0x17c}, // b'K28.3': 0b0011110011 RD-, 0b1100001100 RD+
			{0x0f2, 0x19c}, {0x30d, 0x19c}, // b'K28.4': 0b0011110010 RD-, 0b1100001101 RD+
			{0x0fa, 0x1bc}, {0x305, 0x1bc}, // b'K28.5': 0b0011111010 RD-, 0b1100000101 RD+
			{0x0f6, 0x1dc}, {0x309, 0x1dc}, // b'K28.6': 0b0011110110 RD-, 0b1100001001 RD+
			{0x0f8, 0x1fc}, {0x307, 0x1fc}, // b'K28.7': 0b0011111000 RD-, 0b1100000111 RD+
			{0x3a8, 0x1f7}, {0x057, 0x1f7}, // b'K23.7': 0b1110101000 RD-, 0b0001010111 RD+
			{0x368, 0x1fb}, {0x097, 0x1fb}, // b'K27.7': 0b1101101000 RD-, 0b0010010111 RD+
			{0x2e8, 0x1fd}, {0x117, 0x1fd}, // b'K29.7': 0b1011101000 RD-, 0b0100010111 RD+
			{0x1e8, 0x1fe}, {0x217, 0x1fe}, // b'K30.7': 0b0111101000 RD-, 0b1000010111 RD+
		};
		
		auto it = decode_map.find(ten_bit_code);
		if (it != decode_map.end()) {
			return {it->second, true};
		}
		
		return {0x00, false}; // Unknown symbol
	};


	// Find first edge to start sampling
	mSerial->AdvanceToNextEdge();
	mSerial->AdvanceToNextEdge();

	// Move to center of first bit
	mSerial->Advance( samples_to_bit_center - 1);

	// Use sliding window to find K28.5 comma
	U16 sliding_window = 0;
	bool synchronized = false;
	std::vector<U64> bit_sample_positions; // Track sample positions that shift with sliding window

	for( ; ; )
	{

		// Store current bit sample position (at bit center)
		U64 bit_center_position = mSerial->GetSampleNumber();
		bit_sample_positions.insert(bit_sample_positions.begin(), bit_center_position);
		if( bit_sample_positions.size() > 10 )
			bit_sample_positions.pop_back();

		// Sample current bit into sliding window coming lsb 1st but we need to match the bit order with the lsb on the left
		// for ease of reading the 10-bit value
		sliding_window <<= 1;
		sliding_window &= 0x3FF; // Keep only 10 bits in the sliding window
		if( mSerial->GetBitState() == BIT_HIGH )
            sliding_window |= 1;
        if( mSerial->GetBitState() == BIT_LOW )
			sliding_window &= 0x3FE;

		// Process 8b/10b symbols once we have 10 samples
		if( bit_sample_positions.size() == 10 )
		{
			// Check if we have K28.5 comma pattern to establish synchronization
			if( !synchronized && ((sliding_window == K28_5_RD_MINUS) || (sliding_window == K28_5_RD_PLUS)) )
			{
				synchronized = true;
			}

			// If synchronized, decode every 10-bit symbol
			if( synchronized )
			{
				auto decode_result = decode_symbol(sliding_window);
				U8 decoded_value = decode_result.first;
				bool is_valid = decode_result.second;
				
				// Create frame for this 8b/10b symbol
				Frame frame;
				frame.mData1 = decoded_value; // 8-bit decoded value
				frame.mData2 = sliding_window; // Store 10-bit raw value in mData2
				frame.mFlags = is_valid ? 1 : 0; // Valid symbol flag
				
				// Determine frame type: K28.5=Control(1), Data=Data(0), Invalid=Error(2)
				if( !is_valid )
				{
					frame.mType = 2; // Error
				}
				else if( decoded_value == 0xBC ) // K28.5
				{
					frame.mType = 1; // Control
				}
				else
				{
					frame.mType = 0; // Data
				}
				frame.mStartingSampleInclusive = bit_sample_positions.back() - samples_per_bit/2;
				frame.mEndingSampleInclusive = bit_sample_positions.front() + samples_per_bit/2;

				// Add bit-level markers with color coding for bit values
				for( U32 i = 0; i < 6; i++ )
				{
					U32 bit_index = 9 - i;
					bool bit_is_high = (sliding_window & (1 << i)) != 0;
					AnalyzerResults::MarkerType marker_type = bit_is_high ? AnalyzerResults::One : AnalyzerResults::Dot;
					mResults->AddMarker( bit_sample_positions[bit_index], marker_type, mSettings.mInputChannel );
				}
				
				for( U32 i = 6; i < 10; i++ )
				{
					U32 bit_index = 9 - i;
					bool bit_is_high = (sliding_window & (1 << i)) != 0;
					AnalyzerResults::MarkerType marker_type = bit_is_high ? AnalyzerResults::One : AnalyzerResults::Square;
					mResults->AddMarker( bit_sample_positions[bit_index], marker_type, mSettings.mInputChannel );
				}

				// Add special marker for K28.5 comma
				if( decoded_value == 0xBC )
				{
					mResults->AddMarker( frame.mStartingSampleInclusive, AnalyzerResults::UpArrow, mSettings.mInputChannel );
				}

				mResults->AddFrame( frame );
				mResults->CommitResults();
				ReportProgress( frame.mEndingSampleInclusive );

				// Clear positions and advance to next 10-bit symbol
				bit_sample_positions.clear();
				sliding_window = 0;
			}
		}

		// Move to next bit center
		if (mSerial->WouldAdvancingCauseTransition( samples_per_bit ) == false)
		{
			// No transition, just advance
			mSerial->Advance( samples_per_bit );
		}
		else
		{
			// Transition detected, advance to next edge
			mSerial->AdvanceToNextEdge();
			mSerial->Advance( samples_to_bit_center - samples_per_bit);
		}

		// Check for end of data
		if( mSerial->DoMoreTransitionsExistInCurrentData() == false )
			break;
	}
}

bool decode8b10bAnalyzer::NeedsRerun()
{
	return false;
}

U32 decode8b10bAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitialized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
		mSimulationInitialized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 decode8b10bAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings.mBitRate * 4;
}

const char* decode8b10bAnalyzer::GetAnalyzerName() const
{
	return "8b10b Decoder";
}

const char* GetAnalyzerName()
{
	return "8b10b Decoder";
}

Analyzer* CreateAnalyzer()
{
	return new decode8b10bAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}