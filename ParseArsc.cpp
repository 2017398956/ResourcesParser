/**
 * 测试 arsc 文件解析
 */
#include "ParseArsc.h"
#include "ParseUtil.h"
#include "android/ResourceTypes.h"

using namespace std;



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
	vector<string> tempList;
	getStringsFromStringPoolData(globalStringPoolHeader, pGlobalStrings, false, tempList);
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
		vector<string> resTypes;
		getStringsFromStringPoolData(typeStringPoolHeader, typeStringsData, false, resTypes);
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