#include "decode8b10bAnalyzer.h"
#include "decode8b10bAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <vector>

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

	// K28.5 comma patterns: 0xFA (RD-) and 0x305 (RD+)
	const U16 K28_5_RD_MINUS = 0xFA;  // 0011111010
	const U16 K28_5_RD_PLUS = 0x305;  // 1100000101

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

		//Check if we have K28.5 comma pattern (only if we have 10 samples)
		if( !synchronized && bit_sample_positions.size() == 10 && 
			((sliding_window  == K28_5_RD_MINUS) || (sliding_window == K28_5_RD_PLUS) ) )
		{

			synchronized = true;
			
			// Create frame for K28.5 comma using actual bit positions
			Frame frame;
			frame.mData1 = sliding_window; // sampled 10-bit value
			frame.mFlags = 1;    // Mark as control character
			frame.mType = 1;     // Control frame type for color coding
			frame.mStartingSampleInclusive = bit_sample_positions.back() - samples_per_bit/2; // Earliest sample (oldest bit)
			frame.mEndingSampleInclusive = bit_sample_positions.front() + samples_per_bit/2;  // Latest sample (newest bit)

			FrameV2 frame_v2;
			frame_v2.AddInteger( "10b sample", sliding_window ); // 0-bit value
			mResults->AddFrameV2( frame_v2, "test", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );

			// Add bit-level markers using actual sample positions
			// Add dots for 5b/6b portion (bits 0-5, rightmost bits in our LSB-first representation)
			for( U32 i = 0; i < 6; i++ )
			{
				U32 bit_index = 9 - i;
				AnalyzerResults::MarkerType marker_type = AnalyzerResults::Dot;
				mResults->AddMarker( bit_sample_positions[bit_index], marker_type, mSettings.mInputChannel );
			}
			
			// Add squares for 3b/4b portion (bits 6-9, leftmost bits in our LSB-first representation)
			for( U32 i = 6; i < 10; i++ )
			{
				U32 bit_index = 9 - i;
				AnalyzerResults::MarkerType marker_type = AnalyzerResults::Square;
				mResults->AddMarker( bit_sample_positions[bit_index], marker_type, mSettings.mInputChannel );
			}

			// Add up arrow marker for K28.5 comma synchronization
			mResults->AddMarker( frame.mStartingSampleInclusive, AnalyzerResults::UpArrow, mSettings.mInputChannel );

			mResults->AddFrame( frame );
			mResults->CommitResults();
			ReportProgress( frame.mEndingSampleInclusive );
            bit_sample_positions.clear(); // Clear positions after processing K28.5
            sliding_window = 0; // Reset sliding window after K28.5 detection
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