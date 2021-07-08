// coff_generator.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "windows.h"
#include <vector>

#define  MAXKEY    128

typedef struct Section_t{
	int data_offset;
	char * data;
	int data_allocated;
	char index;
	struct Section_t * link;
	int * hashtab;
	IMAGE_SECTION_HEADER sh;
} Section;

int nsec_image = 0;
std::vector<Section> vecSection;

Section * section_new(char * name, int iCharacteristics)
{
	Section * sec;
	int initSize = 8;
	sec = (Section *)malloc(sizeof(Section));
	memcpy(sec->sh.Name, name, strlen(name));
	sec->sh.Characteristics = iCharacteristics;
	// sec->index = sections.count + 1;  // Start from 1
	sec->data = (char *)malloc(sizeof(char) * initSize);
	sec->data_allocated = initSize;
	if(!(iCharacteristics & IMAGE_SCN_LNK_REMOVE))
		nsec_image++;
	vecSection.push_back(*sec);
	return sec;
}

void section_realloc(Section * sec, int new_size);
void * section_ptr_add(Section * sec, int increment)
{
	int offset, offsetOne;
	offset = sec->data_offset;
	offsetOne = offset + increment;
	if(offsetOne > sec->data_allocated)
		section_realloc(sec, offsetOne);
	sec->data_offset = offsetOne;
	return sec->data + offset;
}

void section_realloc(Section * sec, int new_size)
{
	int size;
	char * data ;
	size = sec->data_allocated;
	while(size < new_size)
		size = size * 2;
	data = (char *)realloc(sec->data, size);
	if(data == NULL)
		printf("realloc error");
	
	memset(data + sec->data_allocated, 0x00, size - sec->data_allocated);
	sec->data = data;
	sec->data_allocated = size;
}

Section * new_coffsym_section(char* symtable_name, int iCharacteristics, 
		char * strtable_name)
{
	Section * sec;
	sec = section_new(symtable_name, iCharacteristics);
	sec->link = section_new(strtable_name, iCharacteristics);
	sec->hashtab = (int *)malloc(sizeof(int) * MAXKEY);
	return sec;
}

int coffsym_add(Section * )
{
//	CoffSym * cfsym;
//	int cs, keyno;
//	char csname;
	return 1;
}

int main(int argc, char* argv[])
{
	printf("Hello World!\n");
	return 0;
}

