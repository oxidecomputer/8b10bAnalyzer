#include "decode8b10bAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "decode8b10bAnalyzer.h"
#include "decode8b10bAnalyzerSettings.h"
#include <iostream>
#include <fstream>

decode8b10bAnalyzerResults::decode8b10bAnalyzerResults( decode8b10bAnalyzer* analyzer, decode8b10bAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

decode8b10bAnalyzerResults::~decode8b10bAnalyzerResults()
{
}

void decode8b10bAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	if( frame.mFlags == 1 && frame.mData1 == 0xBC )
	{
		// K28.5 comma character - determine RD from 10-bit pattern
		// Note: mData2 contains the sampled pattern (LSB-left), need to check against our reversed patterns
		U16 ten_bit_pattern = frame.mData2;
		
		// Helper to reverse bits for display
		auto reverse_10_bits = [](U16 value) -> U16 {
			U16 reversed = 0;
			for(int i = 0; i < 10; i++) {
				if(value & (1 << i)) {
					reversed |= (1 << (9 - i));
				}
			}
			return reversed;
		};
		
		// Check against the actual sampled patterns that were working
		bool is_rd_minus = (ten_bit_pattern == 0xFA);  // 0xFA = RD-, 0x305 = RD+
		
		// Multi-level display with correct RD
		AddResultString( "K28.5" );                                           // Short format
		AddResultString( is_rd_minus ? "K28.5 RD-" : "K28.5 RD+" );          // Medium format (default)
		AddResultString( is_rd_minus ? "K28.5: 001111|1010" : "K28.5: 110000|0101" ); // Detailed format
	}
	else if( frame.mFlags == 1 ) // Valid symbol but not K28.5
	{
		// Data character - show as D-notation (Dx.y format)
		U8 decoded_value = frame.mData1;
		U8 dx = (decoded_value >> 5) & 0x1F;  // Bits 7:5 = x part  
		U8 dy = decoded_value & 0x07;         // Bits 2:0 = y part
		
		char d_notation[32];
		sprintf( d_notation, "D%d.%d", dx, dy );
		
		char hex_str[32];
		sprintf( hex_str, "0x%02X", decoded_value );
		
		AddResultString( d_notation );                   // Short: D10.2
		AddResultString( d_notation );                   // Medium: D10.2
		AddResultString( hex_str );                      // Detailed: 0x4A
	}
	else
	{
		// Invalid symbol - show the 10-bit pattern in hex for analysis
		char hex_str[32];
		sprintf( hex_str, "0x%03X", frame.mData2 & 0x3FF );
		char error_str[64];
		sprintf( error_str, "UNK:%s", hex_str );
		
		AddResultString( "UNK" );
		AddResultString( error_str );
		AddResultString( error_str );
	}
}

void decode8b10bAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Symbol,Type,Value,10-bit Code,5b/6b,3b/4b" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		if( frame.mFlags == 1 && frame.mData1 == 0xBC )
		{
			// K28.5 with correct RD and 10-bit breakdown
			U16 ten_bit_pattern = frame.mData2;
			bool is_rd_minus = (ten_bit_pattern == 0xFA);
			
			if( is_rd_minus )
			{
				file_stream << time_str << ",K28.5 RD-,Control," << number_str << ",0011111010,001111,1010" << std::endl;
			}
			else
			{
				file_stream << time_str << ",K28.5 RD+,Control," << number_str << ",1100000101,110000,0101" << std::endl;
			}
		}
		else
		{
			file_stream << time_str << ",Data,Data," << number_str << ",,,," << std::endl;
		}

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void decode8b10bAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame( frame_index );
	ClearTabularText();

	if( frame.mFlags == 1 && frame.mData1 == 0xBC )
	{
		AddTabularText( "K28.5" );
	}
	else
	{
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		AddTabularText( number_str );
	}
#endif
}

void decode8b10bAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void decode8b10bAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}