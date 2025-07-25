#include "decode8b10bAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "decode8b10bAnalyzer.h"
#include "decode8b10bAnalyzerSettings.h"
#include "decode8b10bSymbolUtils.h"
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

	if( frame.mType == 1 || frame.mType == 0)
	{
		// Use shared utility for symbol naming
		U16 ten_bit_pattern = frame.mData2;
		
		auto decode_result = decode8b10bSymbolUtils::DecodeSymbol(ten_bit_pattern);
		U16 decoded_value = std::get<0>(decode_result);
		decode8b10bSymbolUtils::Disparity disparity = std::get<1>(decode_result);
		bool is_valid = std::get<2>(decode_result);

		const char* symbol_name = decode8b10bSymbolUtils::GetSymbolName(decoded_value);

		char disp_str[32];
		if( disparity == decode8b10bSymbolUtils::Disparity::Positive )
		{
			sprintf( disp_str, " RD+" );
		}
		else if( disparity == decode8b10bSymbolUtils::Disparity::Negative )
		{
			sprintf( disp_str, " RD-" );
		}
		else
		{
			sprintf( disp_str, " RD+/-" );
		}

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		char long_str[128];
		sprintf( long_str, "%s%s", symbol_name, disp_str );       // Medium format (default)

		// Multi-level display with correct RD
		if( frame.mType == 0) {
			AddResultString( number_str );
			char data_str[512];
			snprintf( data_str, sizeof(data_str), "%s (%s)", number_str, long_str); 
			AddResultString( data_str );
		}
		else {
			AddResultString( symbol_name );                                           // Short format
			AddResultString( long_str );
		}
		
		
	}
	else
	{
		// Invalid symbol - show the 10-bit pattern in hex for analysis
		char hex_str[32];
		sprintf( hex_str, "0x%03llX", (unsigned long long)(frame.mData2 & 0x3FF) );
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

	// if( frame.mFlags == 1 && frame.mData1 == 0xBC )
	// {
	// 	AddTabularText( "K28.5" );
	// }
	// else
	// {
	// 	char number_str[128];
	// 	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	// 	AddTabularText( number_str );
	// }
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