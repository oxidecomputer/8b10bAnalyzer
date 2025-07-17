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
		// K28.5 comma character - multi-level display
		AddResultString( "K28.5" );                          // Short format
		AddResultString( "K28.5 RD+" );                      // Medium format (default)
		AddResultString( "K28.5: 001111|1010" );            // Detailed format with 5b/6b|3b/4b
	}
	else
	{
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		AddResultString( number_str );
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
			// K28.5 with 10-bit breakdown
			file_stream << time_str << ",K28.5,Control," << number_str << ",0011111010,001111,1010" << std::endl;
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