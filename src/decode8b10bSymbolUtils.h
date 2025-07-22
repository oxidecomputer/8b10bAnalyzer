#ifndef DECODE8B10B_SYMBOL_UTILS_H
#define DECODE8B10B_SYMBOL_UTILS_H

#include <AnalyzerTypes.h>
#include <tuple>

class decode8b10bSymbolUtils
{
public:
	static const char* GetSymbolName(U16 decoded_octet);
	static std::tuple<U16, bool, bool> DecodeSymbol(U16 ten_bit_code);
};

#endif // DECODE8B10B_SYMBOL_UTILS_H