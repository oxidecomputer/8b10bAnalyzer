#ifndef DECODE8B10B_SYMBOL_UTILS_H
#define DECODE8B10B_SYMBOL_UTILS_H

#include <AnalyzerTypes.h>
#include <tuple>

class decode8b10bSymbolUtils
{
public:
	enum class Disparity
	{
		Negative,
		Positive,
		Same
	};
	
	static const char* GetSymbolName(U16 decoded_octet);
	static std::tuple<U16, Disparity, bool> DecodeSymbol(U16 ten_bit_code);
};

#endif // DECODE8B10B_SYMBOL_UTILS_H