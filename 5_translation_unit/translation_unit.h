#ifndef TRANSLATION_UNIT_H
#define TRANSLATION_UNIT_H

/*******************************grammar.h begin****************************/
/* �﷨״̬ */
enum e_SynTaxState
{
	SNTX_NUL,       // ��״̬��û���﷨��������
	SNTX_SP,		// �ո� int a; int __stdcall MessageBoxA(); return 1;
	SNTX_LF_HT,		// ���в�������ÿһ���������������塢��������Ҫ��Ϊ��״̬
	SNTX_DELAY      // �ӳ�ȡ����һ���ʺ�ȷ�������ʽ��ȡ����һ�����ʺ󣬸��ݵ������͵�������syntax_indentȷ����ʽ������� 
};

/* �洢���� */
enum e_StorageClass
{
	SC_GLOBAL =   0x00f0,		// �������������ͳ������ַ��������ַ�������,ȫ�ֱ���,��������          
	SC_LOCAL  =   0x00f1,		// ջ�б���
	SC_LLOCAL =   0x00f2,       // �Ĵ���������ջ��
	SC_CMP    =   0x00f3,       // ʹ�ñ�־�Ĵ���
	SC_VALMASK=   0x00ff,       // �洢��������             
	SC_LVAL   =   0x0100,       // ��ֵ       
	SC_SYM    =   0x0200,       // ����	

	SC_ANOM	  = 0x10000000,     // ��������
	SC_STRUCT = 0x20000000,     // �ṹ�����
	SC_MEMBER = 0x40000000,     // �ṹ��Ա����
	SC_PARAMS = 0x80000000,     // ��������
};

/* ���ͱ��� */
enum e_TypeCode
{
	T_INT    =  0,			// ����                     
	T_CHAR   =  1,			// �ַ���                 
	T_SHORT  =  2,			// ������                       
	T_VOID   =  3,			// ������                        
	T_PTR    =  4,			// ָ��                          
	T_FUNC   =  5,			// ����                    
	T_STRUCT =  6,			// �ṹ�� 
	
	T_BTYPE  =  0x000f,		// ������������          
	T_ARRAY  =  0x0010,		// ����
};

#endif