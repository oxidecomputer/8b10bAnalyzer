#include "decode8b10bAnalyzer.h"
#include "decode8b10bAnalyzerSettings.h"
#include "decode8b10bSymbolUtils.h"
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
	U32 index = 0;

	U32 samples_per_bit = mSampleRateHz / mSettings.mBitRate;
	U32 samples_to_bit_center =  U32( 1.5 * double( mSampleRateHz ) / double( mSettings.mBitRate ) );

	bool drop_idle_frames = mSettings.mDropIdleFrames;
	bool last_was_k2805 = false; // Track if the last symbol was K28.5

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

	// Find first edge to start sampling
	mSerial->AdvanceToNextEdge();
	mSerial->AdvanceToNextEdge();

	// Move to center of first bit
	mSerial->Advance( samples_to_bit_center - 1);

	// Use sliding window to find K28.5 comma
	U16 sliding_window = 0;
	U8 running_disparity = 0; // Track running disparity
	bool synchronized = false;
	bool first_symbol = true; // Track if this is the first symbol after synchronization
	bool in_idle = false;
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
				last_was_k2805 = true;
			}

			// If synchronized, decode every 10-bit symbol
			if( synchronized )
			{
				in_idle = false;
				auto decode_result = decode8b10bSymbolUtils::DecodeSymbol(sliding_window);
				U16 decoded_value = std::get<0>(decode_result);
				decode8b10bSymbolUtils::Disparity disparity = std::get<1>(decode_result);
				bool is_valid = std::get<2>(decode_result);
				U8 ones_count = CountOnes(sliding_window);
				const char* symbol_name = decode8b10bSymbolUtils::GetSymbolName(decoded_value);

				if (decoded_value == 0x1BC || (decoded_value == 0x4A && last_was_k2805)
										   || (decoded_value == 0xB3 && last_was_k2805))
				{
					in_idle = true;
				}
				
				// Skip frame if dropping idle frames is enabled and this is an idle frame
				if( mSettings.mDropIdleFrames && in_idle )
				{
					// Clear positions and advance to next 10-bit symbol
					bit_sample_positions.clear();
					sliding_window = 0;
					first_symbol = false;
					last_was_k2805 = (decoded_value == 0x1BC && is_valid); // Update last_was_k2805 only if we just processed K28.5
					// Deal with disparity even though we are skipping the frame
					// TODO: no errors logged
					if (ones_count > 5 && running_disparity == 0)
					{
						running_disparity = 1; // Set running disparity to positive
					}
					else if (ones_count < 5 && running_disparity == 1)
					{
						running_disparity = 0; // Set running disparity to negative
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
					continue;
				}

				// Create frame for this 8b/10b symbol
				Frame frame;
				frame.mData1 = decoded_value; // 8-bit decoded value
				frame.mData2 = sliding_window; // Store 10-bit raw value in mData2
				frame.mFlags = is_valid ? 1 : 0; // Valid symbol flag
				frame.mStartingSampleInclusive = bit_sample_positions.back() - samples_per_bit/2;
				frame.mEndingSampleInclusive = bit_sample_positions.front() + samples_per_bit/2;
	
				// Use shared utility for symbol naming

				FrameV2 frame_v2;

				if (ones_count > 5 && running_disparity == 0)
				{
					running_disparity = 1; // Set running disparity to positive
				}
				else if (ones_count < 5 && running_disparity == 1)
				{
					running_disparity = 0; // Set running disparity to negative
				}
				else if (!first_symbol && ones_count > 5 && running_disparity == 1)
				{
					frame_v2.AddString("Error", "Disparity mismatch: too many ones");
				}
				else if (!first_symbol && ones_count < 5 && running_disparity == 0)
				{
					frame_v2.AddString("Error", "Disparity mismatch: too many zeros");
				}
				
				
	            frame_v2.AddString("Name", symbol_name);
 				frame_v2.AddInteger("Decoded Octet", decoded_value);

				char disp_str[32];
				if( disparity == decode8b10bSymbolUtils::Disparity::Positive )
				{
					sprintf( disp_str, "+" );
				}
				else if( disparity == decode8b10bSymbolUtils::Disparity::Negative )
				{
					sprintf( disp_str, "-" );
				}
				else
				{
					sprintf( disp_str, "neutral" );
				}

				frame_v2.AddString("Disparity", disp_str);
				
				// Determine frame type: K28.5=Control(1), Data=Data(0), Invalid=Error(2)
				if( !is_valid )
				{
					frame.mType = 2; // Error
					mResults->AddFrameV2( frame_v2, "error", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );
				}
				else if( decoded_value >= 0xFF ) // Control char
				{
					frame.mType = 1; // Control
					mResults->AddFrameV2( frame_v2, "control", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );
				}
				else
				{
					frame.mType = 0; // Data
					mResults->AddFrameV2( frame_v2, "data", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );
				}
				

				mResults->AddFrame( frame );
				mResults->CommitResults();
				ReportProgress( frame.mEndingSampleInclusive );

				// Clear positions and advance to next 10-bit symbol
				bit_sample_positions.clear();
				sliding_window = 0;
				first_symbol = false; // After first symbol, we are no longer in the initial state
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

U8 decode8b10bAnalyzer::CountOnes(U16 value)
{
	U8 count = 0;
	// Count '1' bits in the 10-bit value by checking each bit position
	for(int i = 0; i < 10; i++) {
		if(value & 0x3FF & (1 << i)) {
			count++;
		}
	}
	return count;
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