#ifndef DECODE8B10B_SIMULATION_DATA_GENERATOR
#define DECODE8B10B_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class decode8b10bAnalyzerSettings;

class decode8b10bSimulationDataGenerator
{
public:
	decode8b10bSimulationDataGenerator();
	~decode8b10bSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, decode8b10bAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	decode8b10bAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //DECODE8B10B_SIMULATION_DATA_GENERATOR