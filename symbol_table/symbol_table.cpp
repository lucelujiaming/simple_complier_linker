// symbol_table.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
#include "symbol_table.h"
#include <vector>


std::vector<Symbol> global_sym_stack;  //ȫ�ַ���ջ
std::vector<Symbol> local_sym_stack;   //�ֲ�����ջ


extern std::vector<TkWord> tktable;
extern int token_type;

extern Type char_pointer_type,		// �ַ���ָ��
			 int_type,				// int����
			 default_func_type;		// ȱʡ��������

/***********************************************************
 *  ���ܣ������ŷ��ڷ���ջ��
 *  v��   ���ű��
 *  type��������������
 *  c��   ���Ź���ֵ
 **********************************************************/
Symbol * sym_direct_push(std::vector<Symbol> &ss, int v, Type * type, int c)
{
	Symbol s; //, *p;
	s.v =v;
	s.type.t = type->t;
	s.type.ref = type->ref;
	s.c =c;
	s.next = NULL;
	ss.push_back(s);
	return &ss[0];
}

/***********************************************************
 *  ���ܣ������ŷ��ڷ���ջ�У���̬�ж���
 *        ����ȫ�ַ���ջ���Ǿֲ�����ջ
 *  v��   ���ű��
 *  type��������������
 *  r��   ���Ŵ洢����
 *  c��   ���Ź���ֵ
 **********************************************************/
Symbol * sym_push(int v, Type * type, int r, int c)
{
	Symbol *ps, **pps;
	TkWord *ts;
	// std::vector<Symbol> * ss;
	
	if(local_sym_stack.size() == 0)
	{
		 ps = sym_direct_push(local_sym_stack, c, type, c);
	}
	else
	{
		 ps = sym_direct_push(global_sym_stack, c, type, c);
	}
	
	ps->r = r;
	
	if((v & SC_STRUCT) || v < SC_ANOM)
	{
		ts = &(TkWord)tktable[v & ~SC_STRUCT];
		if(v & SC_STRUCT)
			pps = &ts->sym_struct;
		else
			pps = &ts->sym_identifier;
		
		ps->prev_tok = *pps;
		*pps = ps;
	}
	return ps;
}

/***********************************************************
 *  ���ܣ����������ŷ���ȫ�ַ��ű���
 *  v��   ���ű��
 *  type��������������
 *  ����������ű�ʹ��func_sym_push������
 *  ���������֤�������Ŷ������ȫ�ַ���ջ�� 
 **********************************************************/
Symbol * func_sym_push(int v, Type * type)
{
	Symbol *s, **ps;
	s = sym_direct_push(global_sym_stack, v, type, 0);
	ps = &((TkWord *)&tktable[v])->sym_identifier;
	while(*ps != NULL)
		ps = &(* ps)->prev_tok;
	s->prev_tok = NULL;
	*ps = s;
	return s;
}

/***********************************************************
 *  ����������ű�ͨ��var_sym_put������
 *  �����������ݱ����Ǿֲ���������ȫ�ֱ�����������Ӧ�ķ���ջ�С�
 **********************************************************/
Symbol * var_sym_put(Type * type, int r, int v, int addr)
{
	Symbol *sym = NULL;
	if((r & SC_VALMASK) == SC_LOCAL)
	{
		sym = sym_push(v, type, r, addr);
	}
	else if((r & SC_VALMASK) == SC_GLOBAL)
	{
		sym = sym_search(v);
		if(sym)
			printf("Error dual defined");
		else
			sym = sym_push(v, type, r | SC_SYM, 0);
	}
	return sym;
}

/***********************************************************
 *  ���ܣ��������Ʒ���ȫ�ַ��ű�
 *  sec�� ������
 *  c��   ���Ź���ֵ
 **********************************************************/
Symbol * sec_sym_put(char * sec, int c)
{
	// TkWord * tp;
	Symbol *s;
	Type type;
	type.t = T_INT;
	push_token(sec, TK_CINT);
	token_type = tktable[tktable.size() - 1].tkcode ;
	s = sym_push(token_type, &type, SC_LOCAL, c);
	return s;
}

//���ʱ���
//ָ���ϣ��ͻ����������
//�����ַ���
//ָ�򵥴�����ʾ�Ľṹ����
//ָ�򵥴�����ʾ�ı�ʶ��


/***********************************************************
 *  ���ܣ� ����ջ�з���ֱ��ջ������Ϊ'b
 *  p_top������ջջ��
 *  b��    ����ָ��
 **********************************************************/
void sym_pop(std::vector<Symbol> * pop, Symbol *b)
{
	Symbol *s, **ps;
	TkWord * ts;
	int v;
	
	s = &(pop->at(0));
	while(s != b)
	{
		v = s->v;
		
		if((v & SC_STRUCT) || v < SC_ANOM)
		{
			ts = &(TkWord)tktable[v & ~SC_STRUCT];
			if(v & SC_STRUCT)
				ps = &ts->sym_struct;
			else
				ps = &ts->sym_identifier;
			
			*ps = s->prev_tok ;
		}
		pop->erase(pop->begin());
		s = &(pop->at(0));
	}
}

/***********************************************************
 *  ���ܣ����ҽṹ����
 *  v��   ���ű��
 **********************************************************/
Symbol * struct_search(int v)
{
	return tktable[v].sym_struct;
}

/***********************************************************
 *  ���ܣ������뺯������
 *  l��   �洢���ͣ��ֲ��Ļ���ȫ�ֵ�
 **********************************************************/
Symbol * sym_search(int v)
{
	return tktable[v].sym_identifier;
}

void mk_pointer(Type *t);
void init()
{
//	sym_sec_rdata = sec_sym_put(".rdata", 0);
	int_type.t = T_INT;
	char_pointer_type.t = T_CHAR;
	mk_pointer(&char_pointer_type);
	default_func_type.t = T_FUNC;
//	default_func_type.ref = 
}

int main(int argc, char* argv[])
{
	init();
	token_init(argv[1]);
	get_token();
	translation_unit();
	printf("Hello World!\n");
	token_cleanup();
	return 0;
}

