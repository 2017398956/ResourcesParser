/**
 * 测试 arsc 文件解析
 */
#include "ParseArsc.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iosfwd>
#include <locale>
#include <codecvt>
#include <vector>
#include "ResourceTypes.h"

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
		throw exception("为字符串池分配内存出错！");
		return nullptr;
	}
	fread((void*)pData, size, 1, pFile);
	return pData;
}

static vector<string> getStringsFromStringPoolData(ResStringPool_header header, unsigned char* pData, bool print) {
	vector<string> allStrings;
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
			printf("length:%lld, %s\n", strTemp.length(), strTemp.c_str());
		}
	}

	return allStrings;
}

static int isType(const char* typeName, uint8_t id, vector<string> destTypesPool)
{
	return id >= 0 && id <= destTypesPool.size() && strcmp(typeName, destTypesPool[id - 1].c_str()) == 0;
}

int parseArsc(const char* arscFile)
{
	const char* concernType = "mipmap";
	struct ResChunk_header header;
	//FILE* pFile = fopen(fileName, "rb");
	FILE* pFile;
	errno_t error = fopen_s(&pFile, arscFile, "rb");
	if (error != 0)
	{
		printf("error_t:%d", error);
		return error;
	}
	// 1.头信息
	fread((void*)&header, sizeof(struct ResChunk_header), 1, pFile);
	if (header.type != RES_TABLE_TYPE)
	{
		printf("这不是正确格式的 arsc 文件。");
		return -1;
	}
	printf("type:%u, headSize:%u, size:%u\n\n", header.type, header.headerSize, header.size);
	// 2.读取 package 数量，一般为 1
	ResTable_header resTableHeader = { header };
	fread((void*)(&resTableHeader.packageCount), sizeof(ResTable_header) - sizeof(ResChunk_header), 1, pFile);
	printf("packageCount:%u\n\n", resTableHeader.packageCount);
	// 3.读取全局字符串资源池头信息
	ResStringPool_header globalStringPoolHeader;
	fread((void*)&globalStringPoolHeader, sizeof(ResStringPool_header), 1, pFile);
	if (globalStringPoolHeader.header.type != RES_STRING_POOL_TYPE)
	{
		printf("全局字符串资源池解析失败！");
		return -1;
	}
	printf("ResStringPool_header:\ntype:%u, headSize:%u, size:%u, stringCount:%u, styleCount:%u, flags:%u isUTF8 %d, stringStart:%u, styleStart:%u\n\n",
		globalStringPoolHeader.header.type,
		globalStringPoolHeader.header.headerSize,
		globalStringPoolHeader.header.size,
		globalStringPoolHeader.stringCount,
		globalStringPoolHeader.styleCount,
		globalStringPoolHeader.flags,
		globalStringPoolHeader.flags == ResStringPool_header::UTF8_FLAG,
		globalStringPoolHeader.stringsStart,
		globalStringPoolHeader.stylesStart);
	// 4.读取全局字符串资源池所有字符串及 style
	unsigned char* pGlobalStrings = readStringsDataFromStringPool(pFile, globalStringPoolHeader);
	vector<string> tempList = getStringsFromStringPoolData(globalStringPoolHeader, pGlobalStrings, false);
	// 5.读取 package
	ResTable_package resTablePackage;
	for (size_t packageIndex = 0; packageIndex < resTableHeader.packageCount; packageIndex++)
	{
		printf("========================== package %llu ==========================\n\n", packageIndex);
		if (fread((void*)&resTablePackage, sizeof(struct ResTable_package), 1, pFile) == 0
			|| resTablePackage.header.type != RES_TABLE_PACKAGE_TYPE) {
			printf("读取 package 部分失败！");
			break;
		}
		printf("ResTable_package:\ntype:%u, headSize:%u, size:%u, id:%x, packageName:",
			resTablePackage.header.type,
			resTablePackage.header.headerSize,
			resTablePackage.header.size,
			resTablePackage.id);
		printUtf16String((char16_t*)resTablePackage.name);
		printf("\n\n");
		// 6.读取资源类型字符串池
		struct ResStringPool_header typeStringPoolHeader;
		fread(&typeStringPoolHeader, sizeof(ResStringPool_header), 1, pFile);
		if (typeStringPoolHeader.header.type != RES_STRING_POOL_TYPE)
		{
			printf("资源类型字符串池解析失败！");
			break;
		}
		printf("ResStringPool_header:\ntype:%u, headSize:%u, size:%u, stringCount:%u, styleCount:%u, flags:%u isUTF8 %d, stringStart:%u, styleStart:%u\n\n",
			typeStringPoolHeader.header.type,
			typeStringPoolHeader.header.headerSize,
			typeStringPoolHeader.header.size,
			typeStringPoolHeader.stringCount,
			typeStringPoolHeader.styleCount,
			typeStringPoolHeader.flags,
			typeStringPoolHeader.flags == ResStringPool_header::UTF8_FLAG,
			typeStringPoolHeader.stringsStart,
			typeStringPoolHeader.stylesStart);
		unsigned char* typeStringsData = readStringsDataFromStringPool(pFile, typeStringPoolHeader);
		vector<string> resTypes = getStringsFromStringPoolData(typeStringPoolHeader, typeStringsData, false);
		free(typeStringsData);
		// 7.读取资源项名称字符串池（即所有资源的 id）
		struct ResStringPool_header idStringPoolHeader;
		fread(&idStringPoolHeader, sizeof(ResStringPool_header), 1, pFile);
		if (typeStringPoolHeader.header.type != RES_STRING_POOL_TYPE)
		{
			printf("资源类型字符串池解析失败！");
			break;
		}
		printf("ResStringPool_header:\ntype:%u, headSize:%u, size:%u, stringCount:%u, styleCount:%u, flags:%u isUTF8 %d, stringStart:%u, styleStart:%u\n\n",
			idStringPoolHeader.header.type,
			idStringPoolHeader.header.headerSize,
			idStringPoolHeader.header.size,
			idStringPoolHeader.stringCount,
			idStringPoolHeader.styleCount,
			idStringPoolHeader.flags,
			idStringPoolHeader.flags == ResStringPool_header::UTF8_FLAG,
			idStringPoolHeader.stringsStart,
			idStringPoolHeader.stylesStart);
		unsigned char* idStringsData = readStringsDataFromStringPool(pFile, idStringPoolHeader);
		//getStringsFromStringPoolData(idStringPoolHeader, idStringsData, false);
		// 8.读取资源 SPEC 数组
		struct ResTable_typeSpec resTypeSpecHeader;
		while (fread((void*)&resTypeSpecHeader, sizeof(struct ResTable_typeSpec), 1, pFile)) {
			if (resTypeSpecHeader.header.type != RES_TABLE_TYPE_SPEC_TYPE)
			{
				// 理论上，如果还有内容应该是下一个 package ，但一般只有一个 package
				if (resTypeSpecHeader.header.type != RES_TABLE_PACKAGE_TYPE)
				{
					printf("SPEC_TYPE 读取失败！type:%d", resTypeSpecHeader.header.type);
				}
				int backSize = sizeof(ResTable_typeSpec);
				fseek(pFile, -backSize, SEEK_CUR);
				break;
				
			}
			if (resTypeSpecHeader.id >= 1 && resTypeSpecHeader.id <= resTypes.size())
			{
				printf("%-15sheader.size:%d, size:%d, entryCount:%d\n", resTypes[resTypeSpecHeader.id - 1].c_str(), resTypeSpecHeader.header.headerSize, resTypeSpecHeader.header.size, resTypeSpecHeader.entryCount);
			}
			else {
				printf("%-15dheader.size:%d, size:%d, entryCount:%d\n", resTypeSpecHeader.id, resTypeSpecHeader.header.headerSize, resTypeSpecHeader.header.size, resTypeSpecHeader.entryCount);
			}
			// config 表示 resTypeSpecHeader.id 对应的属性（anim, color, string, layout 等）中的一项受哪个配置影响（其值对应在 configuration.h 中的值）。
			// 例如：config 为 0x80 时，对应 ACONFIGURATION_ORIENTATION 表示受屏幕方向影响。
			// resTypeSpecHeader.entryCount 表示该属性一共有多少个值，如 color 时，表示项目中一共配置了多少 color 属性。
			uint32_t config;
			for (int i = 0; i < resTypeSpecHeader.entryCount; i++) {
				fread((void*)&config, sizeof(uint32_t), 1, pFile);
				// printf("    SPEC_TYPE config: 0x%x\n", config);
				if (isType(concernType, resTypeSpecHeader.id, resTypes))
				{
					printf("\t\tconfig:0x%x\n", config);
				}
			}

			// 每个 ResTable_type.config 对应属性的一种配置下的值。当为空时表示是默认值。例如：当 id 为 drawable 的值时，
			// config 的值可能为 hdpi，xhdpi 等。
			ResTable_type resTypeHeader;
			while (fread((void*)&resTypeHeader, sizeof(ResTable_type), 1, pFile))
			{
				if (resTypeHeader.header.type != RES_TABLE_TYPE_TYPE)
				{
					if (resTypeHeader.header.type != RES_TABLE_TYPE_SPEC_TYPE && resTypeHeader.header.type != RES_TABLE_PACKAGE_TYPE)
					{
						printf("TYPE 解析错误! type:0x%x", resTypeHeader.header.type);
					}
					int backSize = sizeof(ResTable_type);
					fseek(pFile, -backSize, SEEK_CUR);
					break;
				}
				if (resTypeHeader.id >= 1 && resTypeHeader.id <= resTypes.size())
				{
					printf("\t%s          header.size:%d, size:%d, entryCount:%d, configs:%s\n", resTypes[resTypeHeader.id - 1].c_str(), resTypeHeader.header.headerSize, resTypeHeader.header.size, resTypeHeader.entryCount, resTypeHeader.config.toString().c_str());
				}
				else {
					printf("\t%d          header.size:%d, size:%d, entryCount:%d, configs:%s\n", resTypeHeader.id, resTypeHeader.header.headerSize, resTypeHeader.header.size, resTypeHeader.entryCount, resTypeHeader.config.toString().c_str());
				}
				// 实际 struct ResTable_type 的大小可能不同 sdk 版本不一样,所以 typeHeader.header.headerSize 才是真正的头部大小
				// 跳转到 entries offset 的位置，和 ResTable_typeSpec 的头位置后面跟的的是 config 不同，这里是 ResTable_entry 在数据中的偏移量
				fseek(pFile, resTypeHeader.header.headerSize - sizeof(struct ResTable_type), SEEK_CUR);
				uint32_t* pOffsets = (uint32_t*)malloc(resTypeHeader.entryCount * sizeof(uint32_t));
				fread((void*)pOffsets, sizeof(uint32_t), resTypeHeader.entryCount, pFile);
				// 打印出 ResTable_type 中每个 entry 相对于数据区域的偏移量
				if (isType(concernType, resTypeHeader.id, resTypes))
				{
					for (int i = 0; i < resTypeHeader.entryCount; i++)
					{
						printf("\t\t\toffset:0x%x\n", *(pOffsets + i));
					}
				}
				// 读取 ResTable_type 中剩余的 entries 数据部分。到这里当前 ResTable_type 相关数据读取完毕。下面的逻辑都是对 pData 的处理。
				unsigned char* pData = (unsigned char*)malloc(resTypeHeader.header.size - resTypeHeader.entriesStart);
				fread((void*)pData, resTypeHeader.header.size - resTypeHeader.entriesStart, 1, pFile);

				for (int i = 0; i < resTypeHeader.entryCount; i++) {
					// 每个 entry 在数据区相对的偏移量
					uint32_t offset = *(pOffsets + i);
					if (offset == ResTable_type::NO_ENTRY) {
						continue;
					}
					struct ResTable_entry* pEntry = (struct ResTable_entry*)(pData + offset);
					if (isType(concernType, resTypeHeader.id, resTypes))
					{
						// 获取 ResTable_entry 对应的 id 的名称，key.index 对应 id 区域中的位置
						printf("\t\tid name:");
						printStringFromStringsPool(
							(uint32_t*)idStringsData,
							(char*)idStringsData + (idStringPoolHeader.stringsStart - sizeof(struct ResStringPool_header)),
							pEntry->key.index,
							idStringPoolHeader.flags & ResStringPool_header::UTF8_FLAG
						);
					}


					if (pEntry->flags & ResTable_entry::FLAG_COMPLEX) {
						struct ResTable_map_entry* pMapEntry = (struct ResTable_map_entry*)(pData + offset);
						for (int i = 0; i < pMapEntry->count; i++) {
							struct ResTable_map* pMap =
								(struct ResTable_map*)(pData + offset + pMapEntry->size + i * sizeof(struct ResTable_map));
							if (isType(concernType, resTypeHeader.id, resTypes))
							{
								printf("\t\t\tname:0x%08x, valueType:%u, value:%u\n", pMap->name.ident, pMap->value.dataType, pMap->value.data);
							}
						}
					}
					else {
						struct Res_value* pValue = (struct Res_value*)((unsigned char*)pEntry + sizeof(struct ResTable_entry));
						if (isType(concernType, resTypeHeader.id, resTypes))
						{
							printf("\t\t\tvalue.data:0x%x, value :", pValue->data);
							printValue(pValue, globalStringPoolHeader, pGlobalStrings);
							printf("\n");
						}
					}
				}
				free(pOffsets);
				free(pData);
			}
		}

		free(idStringsData);
	}

	free(pGlobalStrings);
	fclose(pFile);
	return 0;
}