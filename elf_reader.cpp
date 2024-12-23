#include "elf_reader.h"
#include "elf.h"
#include <stdio.h>
#include <corecrt_malloc.h>
void readElfFile(const char* elfFilePath)
{
	FILE* elfFile;
	fopen_s(&elfFile, elfFilePath, "rb");
	if (elfFile == nullptr)
	{
		printf("Can not find %s\n", elfFilePath); 
		return;
	}
	char magicHeader[EI_NIDENT + 1] = { 0 };
	size_t readMagicResult = fread_s(&magicHeader, EI_NIDENT, EI_NIDENT, 1, elfFile);
	fseek(elfFile, -EI_NIDENT, SEEK_CUR);
	// 1.判断是否是合法的 elf 文件
	char tempMagic[5] = { 0 };
	strncpy_s(tempMagic, magicHeader, 4);
	tempMagic[4] = '\0';
	if (readMagicResult == 0 || strcmp(ElfMagic, tempMagic) != 0)
	{
		printf("This is not a elf file.\n");
		return;
	}
	// 2.判断大小端格式
	if (magicHeader[EI_DATA] == ELFDATA2LSB)
	{
		printf("This is Little-endian object file.\n");
	}
	else if (magicHeader[EI_DATA] == ELFDATA2MSB)
	{
		printf("This is Big-endian object file.\n");
	}
	else
	{
		printf("This is Invalid data encoding.\n");
	}
	// 3.获取 ABI 信息
	printf("ABI: %d, and ABI version: %d\n", magicHeader[EI_OSABI], magicHeader[EI_ABIVERSION]);
	// 判断是多少位的 elf 文件
	if (magicHeader[EI_CLASS] == ELFCLASS32)
	{
		printf("This is 32 bit elf file.\n");
	}
	else if (magicHeader[EI_CLASS] == ELFCLASS64) {
		printf("This is 64 bit elf file.\n");
		Elf64_Ehdr ehdr;
		fread_s(&ehdr, sizeof(Elf64_Ehdr), sizeof(Elf64_Ehdr), 1, elfFile);
		printf("elf head size: %zu, getSize: %hu\n", sizeof(ehdr), ehdr.e_ehsize);
		printf("e_phentsize: %hu, e_phnum: %hu, e_phoff: %llu\n", ehdr.e_phentsize, ehdr.e_phnum, ehdr.e_phoff);
		printf("e_shentsize: %hu, e_shnum: %hu, e_shoff: %llu\n", ehdr.e_shentsize, ehdr.e_shnum, ehdr.e_shoff);
		if (ehdr.e_type == ET_DYN)
		{
			// 说明是动态库
			// 4.读取各个程序头信息
			size_t phdrSize = sizeof(Elf64_Phdr);
			Elf64_Phdr *phdrListRef = (Elf64_Phdr *) malloc(phdrSize * ehdr.e_phnum);
			if (phdrListRef == nullptr)
			{
				printf("Can not alloc memory for phdrList.\n");
				return;
			}
			fseek(elfFile, ehdr.e_phoff, SEEK_SET);
			fread_s(phdrListRef, phdrSize * ehdr.e_phnum, phdrSize, ehdr.e_phnum, elfFile);
			Elf64_Phdr phdrTemp;
			printf("\n\n");
			for (size_t i = 0; i < ehdr.e_phnum; i++)
			{
				phdrTemp = *(phdrListRef + i);
				printf("p_type: %#10x, p_offset: %6llu, p_filesz: %6llu\n",
					phdrTemp.p_type, phdrTemp.p_offset, phdrTemp.p_filesz);
			}
			free(phdrListRef);
			// 5.读取节区信息
			size_t shdrSize = sizeof(Elf64_Shdr);
			Elf64_Shdr* shdrListRef = (Elf64_Shdr*)malloc(shdrSize * ehdr.e_shnum);
			if (shdrListRef == nullptr)
			{
				printf("Can not alloc memory for shdrList. \n");
				return;
			}
			fseek(elfFile, ehdr.e_shoff, SEEK_SET);
			fread_s(shdrListRef, shdrSize * ehdr.e_shnum, shdrSize, ehdr.e_shnum, elfFile);
			Elf64_Shdr shdrTemp;
			printf("\n\n");
			for (size_t i = 0; i < ehdr.e_shnum; i++)
			{
				shdrTemp = *(shdrListRef + i);
				printf("sh_type: %#10x, sh_name: %4d, sh_offset: %6llu, sh_size: %6llu\n",
					shdrTemp.sh_type, shdrTemp.sh_name, shdrTemp.sh_offset, shdrTemp.sh_size);
			}
			free(shdrListRef);
		}
	}
	else
	{
		printf("Can not get this elf file's class.\n");
	}
}
