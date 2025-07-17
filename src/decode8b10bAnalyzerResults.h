#ifndef DECODE8B10B_ANALYZER_RESULTS
#define DECODE8B10B_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class decode8b10bAnalyzer;
class decode8b10bAnalyzerSettings;

class decode8b10bAnalyzerResults : public AnalyzerResults
{
public:
	decode8b10bAnalyzerResults( decode8b10bAnalyzer* analyzer, decode8b10bAnalyzerSettings* settings );
	virtual ~decode8b10bAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	decode8b10bAnalyzerSettings* mSettings;
	decode8b10bAnalyzer* mAnalyzer;
};

#endif //DECODE8B10B_ANALYZER_RESULTS
