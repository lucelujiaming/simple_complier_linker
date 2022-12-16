#ifndef COFF_GENERATOR_H
#define COFF_GENERATOR_H
#include <stdio.h>
#include <windows.h>

#include "get_token.h"

#define MAXKEY	1024				// ��ϣ������

enum e_Sec_Storage{
	SEC_BSS_STORAGE   = 0,
	SEC_DATA_STORAGE  = 1,
	SEC_RDATA_STORAGE = 2
};

// д��ǰ�档
// ����ṹ�������ǻ���������һ��ǰ�ᡣ
// ���Ƕ��ڲ�ͬ�Ľ���˵��data�б��������Ҳ��ͬ�����磺
//     ���ڴ������˵��  ����һ���ֽ�һ���ֽڵĻ����롣�ڱ������ɴ����ʱ��ͨ��gen_byteд�롣
//     �������ݽ���˵��  ����һ���ֽ�һ���ֽڵ����ݡ���initializer��init_variable��д�롣
//     ���ڷ��ű����˵������һ��һ����CoffSym�ṹ�塣�����ڱ�������У�
//                       ÿ������һ������������һ��ȫ�ֱ�����ʱ�����Ǿ���Ҫ���ӻ����COFF���š�
//     �������ӿ���Ž���˵��Ҳ��һ��һ����CoffSym�ṹ�塣
//     �����ַ�������˵������һ��һ�����ַ�����
//     �����ض�λ��Ϣ����˵������һ��һ����CoffReloc�ṹ�塣
// ����ϣ�������Ҫ��¼������д��λ�ã�����������������С��
typedef struct Section_t{
    int data_offset;                  // ��ǰ����ƫ��λ�á�Ҳ���ǻ�����д��λ�á�
    char * bufferSectionData;         // �����ݡ�Ҳ���ǻ�����
    int data_allocated;               // �����ڴ�ռ䡣Ҳ���ǻ�������С��
    char cSectionIndex;               // ����š�����м����ô���
	                                  // һ�����ڷ��ű���У���ʾ��������������ĸ��ڡ�
	                                  // ���������ض�λ��Ϣ���У���ʶ��Ҫ�ض�λ���ݵĽڱ�š�
    struct Section_t * linkSection;   // �����������ڣ����Žڹ����ַ�����
    int * sym_hashtab;                // ��ϣ��ֻ���ڴ洢���ű�
		  // ��Ϊ���ڷ��ű����˵���������һ��һ����CoffSym�ṹ�壬Ҳ����һ��COFF���š�
		  // �����źܶ��ʱ��Ϊ�˿����ҵ���Ӧ��CoffSym�ṹ�壬��������������ϣ��
		  // �Ա�����coffsym_search�и������ֵĹ�ϣֵ�����ҵ���Ӧ��CoffSym�ṹ�塣
		  // �����ϣ����ÿһ��Ԫ�ر������CoffSym���������±ꡣ
		  // ���磺sym_hashtab[422] = 5 �������ű��еĵ�5��COFF����(CoffSym�ṹ��)
		  // ������"printf"�Ĺ�ϣֵΪ422��
		  // �����������ǲ���"printf"������ŵ�ʱ�����ǾͿ����ȶ�"printf"���й�ϣ��
		  // �õ�����ַ����Ĺ�ϣֵΪ422��֮��ֱ�ӷ���sym_hashtab[422]���õ�5��
		  // ���ǾͿ���֪��"printf"��������Ƿ��ű���еĵ�5����
		  // �������ǻ�������ϣ��ͻ������������CoffSym�ṹ����nextCoffSymName��Ա�����á�
    IMAGE_SECTION_HEADER sh;          // ��ͷ
} Section;

// COFF���Žṹ���塣����μ���7.9 ���ű��¼��Ա����˵��
// ���ڷ��ű����˵�������ŵľ���һ������CoffSym�ṹ�塣
// ����Ķ���ο���IMAGE_SYMBOL�Ķ��塣
typedef struct CoffSym
{
    DWORD coffSymName;          // �������ƣ��ַ������е�һ��ƫ�Ƶ�ַ��
								//           ����Է��ű��¼��ʽ��һ��С�޸ġ�����COFF�ļ��淶��
								//           ��������С�ڳ���8ʱ����������ֱ�Ӵ���ShortName�У�
								//           ������8ʱ��Short�ֶ�Ϊ0��Long�ֶα�ʾ�˷����������ַ������е�ƫ��λ�á�
								//           ���ڸ�Ϊ���۷������Ƴ����Ƿ����8�����ű����ƶ������ַ������У�
								//           name�ֶα�ʾ�����������ַ������е�ƫ��λ�ã�
								//           �����ӵ�Next�ֶ����������ϣ��ͻ����
	DWORD nextCoffSymName;		// ���ڱ����ͻ����
    /*
    struct {
		DWORD   Short;			// if 0, use LongName
        DWORD   Long;			// offset into string table
    } name;
	*/
    DWORD   coff_sym_value;		// �������ص�ֵ�����ֵʹ��coffsym_add_update���á�
                                //                 ����ȫ�ֱ�����ʾ���ŵ�ַ��
                                //                 ���ں�����ʾ��ǰָ���ڴ����λ��
								//                 ������������Section Number��Storage Class��������
								//                 ��ͨ����ʾ���ض�λ�ĵ�ַ��
    short   shortSectionNumber;	// ����˷��ŵĽ���������������������ǽڱ������(��1��ʼ)�����Ա�ʶ����˷��ŵĽ�
								//                     С��1��ֵ������ĺ��壬��ϸ��Ϣ��ο���7.10��Section Number���ֵ��
    WORD    typeFunc;			// COFF�������ͣ�һ����ʾ���͵����֡�
	                            //               ����ֵĵ��ֽڶ��������������һ��ָ�룬�������ػ����������ַ��
	                            //               Microsoft���߽�ʹ�ô��ֶ���ָʾ�����Ƿ�Ϊ������
								//               ���ԣ�ֻ���������ֵ��0x0��0x20��
								//               ���ǣ��������߿���ʹ�ô��ֶ������������Ϣ��
	                            //               ���磬Visual C++ ������Ϣ����ָʾ���͡�
								//               ��ȷָ���������Էǳ���Ҫ�� ����������Ҫ����Ϣ��������������
	                            //               ����ĳЩ��ϵ�ṹ��������Ҫ��Щ��Ϣ������������Ŀ�ġ�
								// �μ���https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
    BYTE    storageClass;		// COFF���Ŵ洢�������һ����ʾ�洢����ö������ֵ��
	                            //                    Ҫ��ȡ������Ϣ����ο���7.11���洢���ȡֵ��
    BYTE    numberOfAuxSymbols;	// ���ڱ���¼����ĸ������ű���ĸ���
} CoffSym;

#define CST_FUNC    0x20  //Coff�������ͣ�����
#define CST_NOTFUNC 0     //Coff�������ͣ��Ǻ���

/* �ض�λ�ṹ���� */
/************************************************************************/
/* COFF�ض�λ��Ϣ��                                                     */
/* Ŀ���ļ��а���COFF�ض�λ��Ϣ��������ָ�������е����ݱ��Ž�ӳ���ļ�   */
/* �Լ����������ؽ��ڴ�ʱӦ����޸���Щ���ݡ�                           */
/* ӳ���ļ���û��COFF�ض�λ��Ϣ��������Ϊ���б����õķ��Ŷ��Ѿ��������� */
/* һ����ƽ̹�ڴ�ռ��еĵ�ַ��ӳ���ļ��а������ض�λ��Ϣ��λ��.reloc�� */
/* �еĻ�ַ�ض�λ��Ϣ(����ӳ����IMAGE_FILE_RELOCS_STRIPPED����)��       */
/* ����Ŀ���ļ��е�ÿ���ڣ�����һ���ɳ��ȹ̶��ļ�¼��ɵ�����������     */
/* �˽ڵ�COFF�ض�λ��Ϣ���������λ�úͳ����ڽ�ͷ��ָ����               */
/* �����ÿ��Ԫ�ض�����IMAGE_RELOCATION�С��򵥿������Ϊ��             */
/*		typedef struct _IMAGE_RELOCATION {                              */
/*			unsigned long ulAddr;    // ��λƫ��                        */
/*			unsigned long ulSymbol;  // ����                            */
/*			unsigned shortusType;    // ��λ����                        */
/*		} IMAGE_RELOCATION;                                             */
/*                                                                      */
/* һ��������Ա��                                                       */
/*     ulAddr��Ҫ��λ�������ڶ���ƫ�ơ����磺                           */
/*        һ�����ĶΣ���ʼλ��Ϊ0x010��ulAddr��ֵΪ0x05�������         */
/*        ��λ��Ϣ��Ҫд��0x15����������Ϣ�ĳ���Ҫ����Ĵ�������ͣ�    */
/*        32λ�Ĵ���Ҫд4���ֽڣ�16λ�ľ�ֻҪ��2���ֽڡ�                */
/*     ulSymbol�Ƿ�����������ָ����ű��е�һ����¼���Ƿ��ű��е�       */
/*        һ����¼�ļ�¼�š������Աָ�����ض�λ��Ϣ����ӳ�ķ��š�      */
/*     usType���ض�λ���͵ı�ʶ��32λ�����У�                           */
/*        ͨ��ֻ�����ֶ�λ��ʽ��һ�Ǿ��Զ�λ��������Զ�λ��            */
/************************************************************************/
typedef struct CoffReloc
{
    DWORD offsetAddr;	// ��Ҫ�����ض�λ�Ĵ�������ݵĵ�ַ��
	                    // ���Ǵӽڿ�ͷ�����ƫ�ƣ����Ͻڵ�RVA/Offset���ֵ��
    DWORD cfsymIndex;	// ���ű������(��0��ʼ)��������Ÿ����������ض�λ�ĵ�ַ��
                        // ������ָ�����ŵĴ洢���Ϊ�ڣ�
					    // ��ô���ĵ�ַ���ǵ�һ������ͬ���Ľڵĵ�ַ��
	// ������ض�λ��¼��ʽ��һ��С�޸ģ�
	// ��Type�����Ա�����������ֽڲ��Ϊ����������
	// һ���ֽڱ�����Ҫ�ض�λ���ݵĽڱ�ţ���һ���ֽڴ洢�ض�λ���͡�
	BYTE  sectionIndex; // ��Ҫ�ض�λ���ݵĽڱ�š�
    BYTE  typeReloc;    // �ض�λ���͡��Ϸ����ض�λ���������ڻ������ͣ�
	                    // ��ο���7.13���ض�λ����ָʾ��ȡֵ��
					    // ������ֻ�����������
					    //     IMAGE_REL_I386_DIR32 - 32λ���Զ�λ��
					    //     IMAGE_REL_I386_REL32 - 32λ��Զ�λ��
} CoffReloc;
// �μ�7.1.4�ڡ����磬 ����ڵĵ�һ���ֽڵĵ�ַ��0xl0����ô�������ֽڵĵ�ַ����0x12


void *mallocz(int size);

// �ڿռ����ͳ�ʼ��������
void init_all_sections_and_coffsyms();
// �ڿռ��ͷź�����
void free_sections();
// �����ݿռ䴦����
void * section_ptr_add(Section * sec, int increment);
void section_realloc(Section * sec, int new_size);

// ���ű�ںͶ�̬���ű�ڲ���������
int coffsym_add(Section * symtab, char * coffsym_name, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);
int coffsym_search(Section * symtab, char * coffsym_name);

// �ַ����ڲ���������
char * coffstr_add(Section * strtab, char * coffstr_name);
// �ض�λ�ڲ���������
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type_reloc);
// �ض�λ��Ϣ�ڲ���������
void coffreloc_redirect_add(int offset, int cfsym_index, char section_index, char type_reloc);


// ����ڵ�ַ�ռ䴦������
int  make_jmpaddr_list(int jmp_addr);
void jmpaddr_backstuff(int fill_offset, int jmp_addr);

// COFF�־û�������
int load_obj_file(char * file_name);
void output_obj_file(char * file_name);
void fpad(FILE * fp, int new_pos);

#endif
