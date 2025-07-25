#include "decode8b10bAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


decode8b10bAnalyzerSettings::decode8b10bAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 9600 ),
	mDropIdleFrames( false ),
	mInputChannelInterface(),
	mBitRateInterface(),
	mDropIdleFramesInterface()
{
	mInputChannelInterface.SetTitleAndTooltip( "Serial", "Standard 8b10b Decoder" );
	mInputChannelInterface.SetChannel( mInputChannel );

	mBitRateInterface.SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface.SetMax( 10000000 );
	mBitRateInterface.SetMin( 1 );
	mBitRateInterface.SetInteger( mBitRate );

	mDropIdleFramesInterface.SetTitleAndTooltip( "Drop Idle Frames", "Drop idle frames from analysis output" );
	mDropIdleFramesInterface.SetValue( mDropIdleFrames );

	AddInterface( &mInputChannelInterface );
	AddInterface( &mBitRateInterface );
	AddInterface( &mDropIdleFramesInterface );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

decode8b10bAnalyzerSettings::~decode8b10bAnalyzerSettings()
{
}

bool decode8b10bAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface.GetChannel();
	mBitRate = mBitRateInterface.GetInteger();
	mDropIdleFrames = mDropIdleFramesInterface.GetValue();

	ClearChannels();
	AddChannel( mInputChannel, "8b10b Decoder", true );

	return true;
}

void decode8b10bAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface.SetChannel( mInputChannel );
	mBitRateInterface.SetInteger( mBitRate );
	mDropIdleFramesInterface.SetValue( mDropIdleFrames );
}

void decode8b10bAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	text_archive >> mDropIdleFrames;

	ClearChannels();
	AddChannel( mInputChannel, "8b10b Decoder", true );

	UpdateInterfacesFromSettings();
}

const char* decode8b10bAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;
	text_archive << mDropIdleFrames;

	return SetReturnString( text_archive.GetString() );
}
