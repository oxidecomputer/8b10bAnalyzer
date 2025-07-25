#ifndef DECODE8B10B_ANALYZER_H
#define DECODE8B10B_ANALYZER_H

#include <Analyzer.h>
#include "decode8b10bAnalyzerSettings.h"
#include "decode8b10bAnalyzerResults.h"
#include "decode8b10bSimulationDataGenerator.h"
#include <memory>

class ANALYZER_EXPORT decode8b10bAnalyzer : public Analyzer2
{
public:
	decode8b10bAnalyzer();
	virtual ~decode8b10bAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

private:
	U8 CountOnes(U16 value);

protected: //vars
	decode8b10bAnalyzerSettings mSettings;
	std::unique_ptr<decode8b10bAnalyzerResults> mResults;
	AnalyzerChannelData* mSerial;

	decode8b10bSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitialized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //DECODE8B10B_ANALYZER_H
