#ifndef DECODE8B10B_ANALYZER_SETTINGS
#define DECODE8B10B_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class decode8b10bAnalyzerSettings : public AnalyzerSettings
{
public:
	decode8b10bAnalyzerSettings();
	virtual ~decode8b10bAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	U32 mBitRate;

protected:
	AnalyzerSettingInterfaceChannel	mInputChannelInterface;
	AnalyzerSettingInterfaceInteger	mBitRateInterface;
};

#endif //DECODE8B10B_ANALYZER_SETTINGS
