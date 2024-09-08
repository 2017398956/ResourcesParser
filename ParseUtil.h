#ifndef PARSE_UTIL_H
#define PARSE_UTIL_H

#include <stdlib.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <iosfwd>
#include <locale>
#include <codecvt>
#include <vector>
#include "android/ResourceTypes.h"

using namespace std;

void printUtf16String(char16_t* str16);

/**
* @param pData 除去 header 后的数据区域（包括 字符串偏移数组，style 偏移数组，字符串区域，style 区域）
* @param pStringsStart 字符串区域 的开始地址
* @param stringIndex 字符串 在字符串区域中的位置
* @param isUtf8 是否是 UTF8 格式，目前只有 UTF8 和 UTF16
*/
char* printStringFromStringsPool(uint32_t* pData, char* pStringsStart, uint32_t stringIndex, uint32_t isUtf8);

/**
* @param pData 除去 header 后的数据区域（包括 字符串偏移数组，style 偏移数组，字符串区域，style 区域）
* @param pStringsStart 字符串区域 的开始地址
* @param stringIndex 字符串 在字符串区域中的位置
* @param isUtf8 是否是 UTF8 格式，目前只有 UTF8 和 UTF16
*/
const char* getStringFromStringsPool(uint32_t* pData, char* pStringsStart, uint32_t stringIndex, uint32_t isUtf8);

void printStringOfComplex(uint32_t complex, bool isFraction);

void printValue(const Res_value* value, struct ResStringPool_header globalStringPoolHeader, unsigned char* pGlobalStrings);

unsigned char* readStringsDataFromStringPool(FILE* pFile, ResStringPool_header header);

void getStringsFromStringPoolData(ResStringPool_header header, unsigned char* pData, bool print, vector<string>& allStrings);

int isType(const char* typeName, uint8_t id, vector<string> destTypesPool);

#endif // !PARSE_UTIL_H
