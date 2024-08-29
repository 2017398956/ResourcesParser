#include "ParseXML.h"
#include "ParseUtil.h"
#include <map>

using namespace std;

void readAttributes(FILE* xmlFile, ResStringPool_header globalStringPoolHeader, unsigned char* pGlobalStrings, vector<string> globalStringList, map<uint32_t, string> namespaceMap) {
	ResXMLTree_attrExt attrExt;
	fread(&attrExt, sizeof(ResXMLTree_attrExt), 1, xmlFile);
	printf("ResXMLTree_attrExt ns:0x%-8x,nsValue:%-7s, name:%s, start:%u, size:%u, count:%u, idIndex:%u, classIndex:%u, styleIndex:%u\n",
		attrExt.ns.index,
		namespaceMap[attrExt.ns.index].c_str(),
		globalStringList[attrExt.name.index].c_str(),
		attrExt.attributeStart,
		attrExt.attributeSize,
		attrExt.attributeCount,
		attrExt.idIndex,
		attrExt.classIndex,
		attrExt.styleIndex);
	fseek(xmlFile, attrExt.attributeStart - sizeof(ResXMLTree_attrExt), SEEK_CUR);
	ResXMLTree_attribute attribute;
	for (size_t i = 0; i < attrExt.attributeCount; i++)
	{
		fread(&attribute, attrExt.attributeSize, 1, xmlFile);
		printf("\tattribute ns:0x%-8x, nsValue:%-7s, name:%s, rawIndex:%u, value:",
			attribute.ns.index,
			namespaceMap[attribute.ns.index].c_str(),
			globalStringList[attribute.name.index].c_str(),
			attribute.rawValue.index);
		printValue(&attribute.typedValue, globalStringPoolHeader, pGlobalStrings);
	}
}

void readAttrExt() {

}

int parseXml(const char* xmlFilePath) {
	FILE* xmlFile;
	errno_t error = fopen_s(&xmlFile, xmlFilePath, "rb");
	if (error != 0)
	{
		printf("error_t:%d", error);
		return error;
	}
	// 1.头信息
	ResXMLTree_header xmlTreeHeader;
	fread(&xmlTreeHeader, sizeof(ResXMLTree_header), 1, xmlFile);
	if (xmlTreeHeader.header.type != RES_XML_TYPE)
	{
		logic_error e("这不是合法的 XML 文件");
		throw exception(e);
	}
	// 2.读取全局字符串资源池头信息
	ResStringPool_header globalStringPoolHeader;
	fread((void*)&globalStringPoolHeader, sizeof(ResStringPool_header), 1, xmlFile);
	if (globalStringPoolHeader.header.type != RES_STRING_POOL_TYPE)
	{
		logic_error e("全局字符串资源池解析失败！");
		throw exception(e);
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
	// 3.读取全局字符串资源池所有字符串及 style
	unsigned char* pGlobalStrings = readStringsDataFromStringPool(xmlFile, globalStringPoolHeader);
	vector<string> globalStringList;
	getStringsFromStringPoolData(globalStringPoolHeader, pGlobalStrings, true, globalStringList);
	// 4.读取所有 id 资源
	ResChunk_header idsHeader;
	fread(&idsHeader, sizeof(ResChunk_header), 1, xmlFile);
	if (idsHeader.type != RES_XML_RESOURCE_MAP_TYPE)
	{
		logic_error e("读取 id 资源失败！");
		throw exception(e);
	}
	size_t idsDataLength = idsHeader.size - idsHeader.headerSize;
	uint32_t* idsData = (uint32_t *) malloc(idsDataLength);
	if (idsData == 0)
	{
		logic_error e("不能为 idsData 分配内存！");
		throw exception(e);
	}
	fread(idsData, idsDataLength, 1, xmlFile);
	for (size_t i = 0; i < idsDataLength / sizeof(uint32_t); i++)
	{
		printf("i:%-3llu, code:0x%x , value:%s\n", i, *(idsData + i), globalStringList[i].c_str());
	}
	// 5.读取每个节点
	map<uint32_t, string> namespaceMap;
	while(true)
	{
		printf("elementHeader: ");
		ResXMLTree_node elementHeader;
		fread(&elementHeader, sizeof(ResXMLTree_node), 1, xmlFile);
		if (elementHeader.comment.index == 0xffffffff)
		{
			printf("header.type:0x%x, linenumber:%u, commentRef:None\n", elementHeader.header.type, elementHeader.lineNumber);
		}
		else {
			printf("header.type:0x%x, linenumber:%u, commentRef:0x%x, comment:%s\n", elementHeader.header.type, elementHeader.lineNumber, elementHeader.comment.index, globalStringList[elementHeader.comment.index].c_str());
		}
		if (elementHeader.header.type == RES_XML_START_NAMESPACE_TYPE)
		{
			printf("namespaceExt:\n");
			ResXMLTree_namespaceExt namespaceExt;
			fread(&namespaceExt, sizeof(ResXMLTree_namespaceExt), 1, xmlFile);
			namespaceMap[namespaceExt.uri.index] = globalStringList[namespaceExt.prefix.index];
			printf("prefixIndex:0x%x, prefix:%s, uriIndex:0x%x, uri:%s\n\n",
				namespaceExt.prefix.index,
				globalStringList[namespaceExt.prefix.index].c_str(),
				namespaceExt.uri.index,
				globalStringList[namespaceExt.uri.index].c_str());
		}
		else if (elementHeader.header.type == RES_XML_START_ELEMENT_TYPE)
		{
			readAttributes(xmlFile, globalStringPoolHeader, pGlobalStrings, globalStringList, namespaceMap);
			printf("\n");
		}
		else if (elementHeader.header.type == RES_XML_END_ELEMENT_TYPE)
		{
			ResXMLTree_endElementExt endElementExt;
			fread(&endElementExt, sizeof(ResXMLTree_endElementExt), 1, xmlFile);
			printf("ResXMLTree_endElementExt:\nns:0x%u, name:%s\n\n", endElementExt.ns.index, globalStringList[endElementExt.name.index].c_str());
		}
		else if (elementHeader.header.type == RES_XML_END_NAMESPACE_TYPE)
		{
			printf("namespace 读取结束\n");
			break;
		}
		else if (elementHeader.header.type == RES_XML_CDATA_TYPE)
		{
			ResXMLTree_cdataExt cdataExt;
			fread(&cdataExt, sizeof(ResXMLTree_cdataExt), 1, xmlFile);
			printf("cdataExt data:%s", globalStringList[cdataExt.data.index].c_str());
			printValue(&cdataExt.typedData, globalStringPoolHeader, pGlobalStrings);
			printf("\n");
		}
		else {
			logic_error e("xml element 读取失败！");
			throw exception(e);
		}
		
	}

	// 用于测试
	ResXMLTree_node temp;
	size_t readSize = fread(&temp, sizeof(ResXMLTree_node), 1, xmlFile);
	if (readSize == 1)
	{
		printf("\nelementHeader readSize:%llu, header.type:0x%x, linenumber:%u, value:0x%x\n", readSize, temp.header.type, temp.lineNumber, temp.comment.index);
	}
	else
	{
		printf("\n\n文件读取结束");
	}
	
	free(idsData);
	fclose(xmlFile);
	return 1;
}