#ifndef TRANSLATION_UNIT_H
#define TRANSLATION_UNIT_H

#include "get_token.h"

/* �﷨״̬ */
enum e_SynTaxState
{
	SNTX_NUL,       // ��״̬��û���﷨��������
	SNTX_SP,		// �ո� int a; int __stdcall MessageBoxA(); return 1;
	SNTX_LF_HT,		// ���в�������ÿһ���������������塢��������Ҫ��Ϊ��״̬
	SNTX_DELAY      // �ӳ�ȡ����һ���ʺ�ȷ�������ʽ��ȡ����һ�����ʺ󣬸��ݵ������͵�������syntax_indentȷ����ʽ������� 
};

#endif