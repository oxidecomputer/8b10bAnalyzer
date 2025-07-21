#ifndef DECODE8B10B_SYMBOL_UTILS_H
#define DECODE8B10B_SYMBOL_UTILS_H

#include <AnalyzerTypes.h>

class decode8b10bSymbolUtils
{
public:
	static const char* GetSymbolName(U16 decoded_octet);
};

#endif // DECODE8B10B_SYMBOL_UTILS_H