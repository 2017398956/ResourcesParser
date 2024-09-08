/**
 * 测试 arsc 文件解析
 */
#include "ParseArsc.h"
#include "ParseUtil.h"
#include <string.h>
#include "android/ResourceTypes.h"

using namespace std;



int parseArsc(const char* arscFile)
{
	const char* concernType = "style";
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
		printf("ResStringPool_header（资源类型字符串池）:\ntype:%u, headSize:%u, size:%u, stringCount:%u, styleCount:%u, flags:%u isUTF8 %d, stringStart:%u, styleStart:%u\n\n",
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
		// 7.读取资源项名称字符串池（即所有资源的 id 的名称）
		struct ResStringPool_header idStringPoolHeader;
		fread(&idStringPoolHeader, sizeof(ResStringPool_header), 1, pFile);
		if (typeStringPoolHeader.header.type != RES_STRING_POOL_TYPE)
		{
			printf("资源类型字符串池解析失败！");
			break;
		}
		printf("ResStringPool_header（资源名称字符串池）:\ntype:%u, headSize:%u, size:%u, stringCount:%u, styleCount:%u, flags:%u isUTF8 %d, stringStart:%u, styleStart:%u\n\n",
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
		vector<string> idNames;
		getStringsFromStringPoolData(idStringPoolHeader, idStringsData, false, idNames);
		// 8.读取 TYPE_SPEC + TYPE_TYPE 数组，每个 TYPE_SPEC 对应一种资源类型，如：mipmap，style 等
		struct ResTable_typeSpec resTypeSpecHeader;
		while (fread((void*)&resTypeSpecHeader, sizeof(struct ResTable_typeSpec), 1, pFile)) {
			if (resTypeSpecHeader.header.type != RES_TABLE_TYPE_SPEC_TYPE)
			{
				// TYPE_SPEC 读取完毕
				int backSize = sizeof(ResTable_typeSpec);
				fseek(pFile, -backSize, SEEK_CUR);
				break;
			}
			int printConcernType = isType(concernType, resTypeSpecHeader.id, resTypes);
			// printConcernType = 0;
			if (printConcernType)
			{
				if (resTypeSpecHeader.id >= 1 && resTypeSpecHeader.id <= resTypes.size())
				{
					printf("TYPE_SPEC: %-15sheader.size:%d, size:%d, entryCount:%d\n", resTypes[resTypeSpecHeader.id - 1].c_str(), resTypeSpecHeader.header.headerSize, resTypeSpecHeader.header.size, resTypeSpecHeader.entryCount);
				}
				else {
					printf("TYPE_SPEC: %-15dheader.size:%d, size:%d, entryCount:%d\n", resTypeSpecHeader.id, resTypeSpecHeader.header.headerSize, resTypeSpecHeader.header.size, resTypeSpecHeader.entryCount);
				}
			}
			// config 表示 resTypeSpecHeader.id 对应的属性（anim, color, string, layout 等）中的一项受哪个配置影响（其值对应在 configuration.h 中的值）。
			// 例如：config 为 0x80 时，对应 ACONFIGURATION_ORIENTATION 表示受屏幕方向影响。
			// resTypeSpecHeader.entryCount 表示该属性一共有多少个值，如 color 时，表示项目中一共配置了多少 color 属性。
			if (printConcernType)
			{
				printf("\t\t有几个 config，TYPE_TYPE 中就有几个 Entry\n");
			}
			uint32_t config;
			for (int i = 0; i < resTypeSpecHeader.entryCount; i++) {
				fread((void*)&config, sizeof(uint32_t), 1, pFile);
				// printf("    SPEC_TYPE config: 0x%x\n", config);
				if (printConcernType)
				{
					printf("\t\tconfig:0x%x\n", config);
				}
			}
			// 9.读取 TYPE_TYPE，TYPE SPEC 后面是 TYPE_TYPE 数组，所以这里循环到不是 TYPE_TYPE 为止。
			// 每个 ResTable_type.config 对应属性的一种配置下的值。当为空时表示是默认值。例如：当 id 为 drawable 的值时，
			// config 的值可能为 hdpi，xhdpi 等。
			ResTable_type resTypeHeader;
			while (fread((void*)&resTypeHeader, sizeof(ResTable_type), 1, pFile))
			{
				if (resTypeHeader.header.type != RES_TABLE_TYPE_TYPE)
				{
					// 当前 SPEC_TYPE 下的 TYPE_TYPE 数组读取完毕
					int backSize = sizeof(ResTable_type);
					fseek(pFile, -backSize, SEEK_CUR);
					break;
				}
				if (printConcernType)
				{
					if (resTypeHeader.id >= 1 && resTypeHeader.id <= resTypes.size())
					{
						printf("\tTYPE_TYPE: %s\theader.size:%d, size:%d, entryCount:%d, configs:%s\n", resTypes[resTypeHeader.id - 1].c_str(), resTypeHeader.header.headerSize, resTypeHeader.header.size, resTypeHeader.entryCount, resTypeHeader.config.toString().c_str());
					}
					else {
						printf("\tTYPE_TYPE: %d\theader.size:%d, size:%d, entryCount:%d, configs:%s\n", resTypeHeader.id, resTypeHeader.header.headerSize, resTypeHeader.header.size, resTypeHeader.entryCount, resTypeHeader.config.toString().c_str());
					}
				}
				// 10.读取 TYPE_TYPE 下的 entries 的偏移数组（类似全局字符串的偏移数组，用于定位下面的每个 Entry）。
				// 实际 struct ResTable_type 的大小可能不同 sdk 版本不一样,所以 typeHeader.header.headerSize 才是真正的头部大小
				// 跳转到 entries offset 的位置，和 ResTable_typeSpec 的头位置后面跟的的是 config 不同，这里是 ResTable_entry 在数据中的偏移量
				fseek(pFile, resTypeHeader.header.headerSize - sizeof(struct ResTable_type), SEEK_CUR);
				uint32_t* pOffsets = (uint32_t*)malloc(resTypeHeader.entryCount * sizeof(uint32_t));
				fread((void*)pOffsets, sizeof(uint32_t), resTypeHeader.entryCount, pFile);
				// 打印出 ResTable_type 中每个 entry 相对于数据区域的偏移量
				if (printConcernType)
				{
					//printf("\t\t\t有几个 offset 就有几个 Entry\n");
					for (int i = 0; i < resTypeHeader.entryCount; i++)
					{
						printf("\t\t\toffset:0x%x\n", *(pOffsets + i));
					}
				}
				// 11.读取 ResTable_type 中剩余的 entries 数据部分。到这里当前 ResTable_type 相关数据读取完毕。下面的逻辑都是对 pData 的处理。
				unsigned char* pData = (unsigned char*)malloc(resTypeHeader.header.size - resTypeHeader.entriesStart);
				fread((void*)pData, resTypeHeader.header.size - resTypeHeader.entriesStart, 1, pFile);

				for (int i = 0; i < resTypeHeader.entryCount; i++) {
					// 每个 entry 在数据区相对的偏移量
					uint32_t offset = *(pOffsets + i);
					if (offset == ResTable_type::NO_ENTRY) {
						continue;
					}
					// 12.读取每个 Entry
					struct ResTable_entry* pEntry = (struct ResTable_entry*)(pData + offset);
					if (printConcernType)
					{
						const char* entryName = getStringFromStringsPool(
							(uint32_t*)idStringsData,
							(char*)idStringsData + (idStringPoolHeader.stringsStart - sizeof(struct ResStringPool_header)),
							pEntry->key.index,
							idStringPoolHeader.flags & ResStringPool_header::UTF8_FLAG
						);
						// 获取 ResTable_entry 对应的 id 的名称，key.index 对应 id 区域中的位置
						printf("\t\tEntry: id value:0x%x, id name:%s\n", resTablePackage.id << 24 | resTypeHeader.id << 16 | i, entryName);
					}
					const char* entryName = getStringFromStringsPool(
							(uint32_t*)idStringsData,
							(char*)idStringsData + (idStringPoolHeader.stringsStart - sizeof(struct ResStringPool_header)),
							pEntry->key.index,
							idStringPoolHeader.flags & ResStringPool_header::UTF8_FLAG
						);
					// 13.根据 Entry 的 flags 判断 Entry 的实际数据类型
					if (pEntry->flags & ResTable_entry::FLAG_COMPLEX) {
						struct ResTable_map_entry* pMapEntry = (struct ResTable_map_entry*)(pData + offset);
						for (int i = 0; i < pMapEntry->count; i++) {
							struct ResTable_map* pMap =
								(struct ResTable_map*)(pData + offset + pMapEntry->size + i * sizeof(struct ResTable_map));
							if (printConcernType 
								// || strcmp(entryName, "Theme.Apk_jiagu") == 0
								)
							{
								printf("\t\t\tparentRef:0x%x, ", pMapEntry->parent.ident);
								printf("name:0x%08x, valueType:%u, value:0x%x, ", pMap->name.ident, pMap->value.dataType, pMap->value.data);
								printValue(&(pMap->value), globalStringPoolHeader, pGlobalStrings);
							}
						}
					}
					else {
						struct Res_value* pValue = (struct Res_value*)((unsigned char*)pEntry + sizeof(struct ResTable_entry));
						if (printConcernType 
							// || strcmp(entryName, "Theme.Apk_jiagu") == 0
							)
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
		// SPEC 读取完毕，下面还有数据的话，可能是另一个 RES_TABLE_PACKAGE_TYPE 也可能是 RES_TABLE_LIBRARY_TYPE ，
		// 但一般只有一个 package
		if (resTypeSpecHeader.header.type == RES_TABLE_LIBRARY_TYPE)
		{	
			// 每个资源的类型由 packId + resTypeId + 递增id 
			// 最高两个字节是packId,系统资源id是：0x01，普通应用资源id是：0x7F
			// 中间的两个字节表示 resTypeId，类型 id 即资源的类型（string、color等），这个值从0开始。（注意每个类型的id不是固定的）
			// 最低四个字节表示这个资源的顺序id，从 1 开始，逐渐累加 1
			// 而且资源 ID 的三个部分在 resources.arsc 文件中是分别存储的
			// https://blog.csdn.net/chzphoenix/article/details/80581838
			// https://blog.csdn.net/chzphoenix/article/details/80569567
			// 说明是修改了 pkgId
			ResTable_lib_header resLibHeader;
			fread(&resLibHeader, sizeof(ResTable_lib_header), 1, pFile);
			printf("lib count:%d\n", resLibHeader.count);
			ResTable_lib_entry libEntry;
			for (size_t i = 0; i < resLibHeader.count; i++)
			{
				fread(&libEntry, sizeof(ResTable_lib_entry), 1, pFile);
				printf("\tpackageId:0x%x, packageName:", libEntry.packageId);
				printUtf16String((char16_t*)libEntry.packageName);
			}
			printf("\n当前 Package 读取完毕。\n\n");
		}
		else if (resTypeSpecHeader.header.type == RES_TABLE_PACKAGE_TYPE)
		{
			// 另一个 Package，不用管，for 循环会继续读取
			printf("\n读取第 %lld 个 Package\n\n", packageIndex + 1);
		}
		else
		{
			printf("SPEC_TYPE 读取失败！type:0x%x", resTypeSpecHeader.header.type);
			break;
		}
		free(idStringsData);
	}

	free(pGlobalStrings);
	fclose(pFile);
	return 0;
}