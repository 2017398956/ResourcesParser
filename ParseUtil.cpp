#include "ParseUtil.h"
#include <string.h>
using namespace std;

void printUtf16String(char16_t* str16) {
	printf("%s\n", std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(str16).c_str());
}

/**
* @param pData 除去 header 后的数据区域（包括 字符串偏移数组，style 偏移数组，字符串区域，style 区域）
* @param pStringsStart 字符串区域 的开始地址
* @param stringIndex 字符串 在字符串区域中的位置
* @param isUtf8 是否是 UTF8 格式，目前只有 UTF8 和 UTF16
*/
void printStringFromStringsPool(uint32_t* pData, char* pStringsStart, uint32_t stringIndex, uint32_t isUtf8) {
	char* str = pStringsStart + *(pData + stringIndex) + 2;
	if (isUtf8) {
		printf("%s\n", str);
	}
	else {
		printUtf16String((char16_t*)str);
	}
}

void printStringOfComplex(uint32_t complex, bool isFraction) {
	const float MANTISSA_MULT =
		1.0f / (1 << Res_value::COMPLEX_MANTISSA_SHIFT);
	const float RADIX_MULTS[] = {
		1.0f * MANTISSA_MULT, 1.0f / (1 << 7) * MANTISSA_MULT,
		1.0f / (1 << 15) * MANTISSA_MULT, 1.0f / (1 << 23) * MANTISSA_MULT
	};

	float value = (complex & (Res_value::COMPLEX_MANTISSA_MASK
		<< Res_value::COMPLEX_MANTISSA_SHIFT))
		* RADIX_MULTS[(complex >> Res_value::COMPLEX_RADIX_SHIFT)
		& Res_value::COMPLEX_RADIX_MASK];
	printf("%f", value);

	if (!isFraction) {
		switch ((complex >> Res_value::COMPLEX_UNIT_SHIFT) & Res_value::COMPLEX_UNIT_MASK) {
		case Res_value::COMPLEX_UNIT_PX: printf("px\n"); break;
		case Res_value::COMPLEX_UNIT_DIP: printf("dp\n"); break;
		case Res_value::COMPLEX_UNIT_SP: printf("sp\n"); break;
		case Res_value::COMPLEX_UNIT_PT: printf("pt\n"); break;
		case Res_value::COMPLEX_UNIT_IN: printf("in\n"); break;
		case Res_value::COMPLEX_UNIT_MM: printf("mm\n"); break;
		default: printf("(unknown unit)\n"); break;
		}
	}
	else {
		switch ((complex >> Res_value::COMPLEX_UNIT_SHIFT) & Res_value::COMPLEX_UNIT_MASK) {
		case Res_value::COMPLEX_UNIT_FRACTION: printf("%%\n"); break;
		case Res_value::COMPLEX_UNIT_FRACTION_PARENT: printf("%%p\n"); break;
		default: printf("(unknown unit)\n"); break;
		}
	}
}

void printValue(const Res_value* value, struct ResStringPool_header globalStringPoolHeader, unsigned char* pGlobalStrings) {
	if (value == nullptr)
	{
		printf("(value is null)\n");
		return;
	}
	if (value->dataType == Res_value::TYPE_NULL) {
		printf("(null)\n");
	}
	else if (value->dataType == Res_value::TYPE_REFERENCE) {
		printf("(reference) 0x%08x\n", value->data);
	}
	else if (value->dataType == Res_value::TYPE_ATTRIBUTE) {
		printf("(attribute) 0x%08x\n", value->data);
	}
	else if (value->dataType == Res_value::TYPE_STRING) {
		printf("(string) ");
		printStringFromStringsPool(
			(uint32_t*)pGlobalStrings,
			(char*)pGlobalStrings + (globalStringPoolHeader.stringsStart - sizeof(struct ResStringPool_header)),
			value->data,
			globalStringPoolHeader.flags & ResStringPool_header::UTF8_FLAG
		);
	}
	else if (value->dataType == Res_value::TYPE_FLOAT) {
		printf("(float) %f\n", *(const float*)&value->data);
	}
	else if (value->dataType == Res_value::TYPE_DIMENSION) {
		printf("(dimension) ");
		printStringOfComplex(value->data, false);
	}
	else if (value->dataType == Res_value::TYPE_FRACTION) {
		printf("(fraction) ");
		printStringOfComplex(value->data, true);
	}
	else if (value->dataType >= Res_value::TYPE_FIRST_COLOR_INT
		&& value->dataType <= Res_value::TYPE_LAST_COLOR_INT) {
		printf("(color) #%08x\n", value->data);
	}
	else if (value->dataType == Res_value::TYPE_INT_BOOLEAN) {
		printf("(boolean) %s\n", value->data ? "true" : "false");
	}
	else if (value->dataType >= Res_value::TYPE_FIRST_INT
		&& value->dataType <= Res_value::TYPE_LAST_INT) {
		printf("(int) %d or %u\n", value->data, value->data);
	}
	else {
		printf("(unknown type)\n");
	}
}

unsigned char* readStringsDataFromStringPool(FILE* pFile, ResStringPool_header header) {
	uint32_t size = header.header.size - sizeof(struct ResStringPool_header);
	unsigned char* pData = (unsigned char*)malloc(size);
	if (pData == 0)
	{
		logic_error e("为字符串池分配内存出错！");
		throw exception(e);
		return nullptr;
	}
	fread((void*)pData, size, 1, pFile);
	return pData;
}

void getStringsFromStringPoolData(ResStringPool_header header, unsigned char* pData, bool print, vector<string>& allStrings) {
	uint32_t* pOffsets = (uint32_t*)pData;
	// header.stringsStart 指的是 header 的起始地址到 字符串区域 起始地址的距离，
	// 那么，header.stringsStart - sizeof(struct ResStringPool_header) 就是字符串偏移数组和 style 偏移数组所占空间的大小，
	// 其值理论上等于 header.stringCount * 4 + header.styleCount * 4 。为了兼容以后的版本，新版本有可能会添加额外的数据，    
	// 所以还是通过 header.stringsStart - sizeof(struct ResStringPool_header) 的方式计算 字符串区域 的开始地址。             
	// pData 已经是 header 末尾的地址了，所以要加上后面计算的结果                    
	unsigned char* pStringsStart = pData + (header.stringsStart - sizeof(struct ResStringPool_header));
	// 这里的 pStringsStart 应该等于 predictedStringsStart
	// unsigned char* predictedStringsStart = pData + header.stringCount * 4 + header.styleCount * 4;

	for (int i = 0; i < header.stringCount; i++) {
		// *(pOffsets + i) 获取 字符串区域 中第 i 个字符串相对于 pStringStart 的偏移量，*(pOffsets + 0) 理论上等于 0。
		// 每条 字符串 前面有两个字节表示该字符串的长度,要跳过                                          
		unsigned char* str = pStringsStart + *(pOffsets + i) + 2;
		string strTemp;
		if (header.flags & ResStringPool_header::UTF8_FLAG) {
			strTemp = string((char*)str);
		}
		else {
			strTemp = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes((char16_t*)str);
		}
		allStrings.push_back(strTemp);
		if (print)
		{
			printf("i:%-3d, %s\n", i, strTemp.c_str());
		}
	}
}

int isType(const char* typeName, uint8_t id, vector<string> destTypesPool)
{
	return id >= 0 && id <= destTypesPool.size() && strcmp(typeName, destTypesPool[id - 1].c_str()) == 0;
}
