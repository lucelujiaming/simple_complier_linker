// symbol_table.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
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
	printf("\t ss.size = %d \n", ss.size());
	// return &ss[0];
	if (ss.size() >= 1)
	{
		return &ss.back();
	}
	else
	{
		return NULL;
	}
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
		ps = sym_direct_push(local_sym_stack, v, type, c);
		print_all_stack("sym_push local_sym_stack");
	}
	else
	{
		ps = sym_direct_push(global_sym_stack, v, type, c);
		print_all_stack("sym_push global_sym_stack");
	}
	
	ps->r = r;
	
	// ��Ϊ����¼�ṹ���Ա����������
	// ����������߼���Ϊ�������֡�
	// ��һ��(v & SC_STRUCT)����v�������Ϊ�ṹ����š�
	// �ڶ���(v < SC_ANOM)����v������Ų����������ţ��ṹ��Ա����������������
	if((v & SC_STRUCT) || (v < SC_ANOM))
	{
		printf("\n tktable.size = %d \n", tktable.size());
		ts = &(TkWord)tktable[v & ~SC_STRUCT];
		printf("\n\t\t(v & ~SC_STRUCT) = (0x%03X,%d) and ts.tkcode = 0x%03X\n", 
			v & ~SC_STRUCT, v & ~SC_STRUCT, ts->tkcode);
		// ����ǽṹ����š�����sym_struct��Ա��
		if(v & SC_STRUCT)
			pps = &ts->sym_struct;
		// �������С��SC_ANOM�ķ��š�
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
	print_all_stack("sym_push global_sym_stack");
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
	TkWord * tp;
	Symbol *s;
	Type type;
	type.t = T_INT;
	tp = tkword_insert(sec); // , TK_CINT);
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
	
	s = &(pop->back());
	while(s != b)
	{
		v = s->v;
		// ����¼�ṹ���Ա����������
		if((v & SC_STRUCT) || v < SC_ANOM)
		{
			ts = &(TkWord)tktable[v & ~SC_STRUCT];
			if(v & SC_STRUCT)
				ps = &ts->sym_struct;
			else
				ps = &ts->sym_identifier;
			
			*ps = s->prev_tok ;
		}
		// pop->erase(pop->begin());
		pop->pop_back();
		if(pop->size() > 0)
			s = &(pop->back());
		else
			break;
	}
}

/***********************************************************
 *  ���ܣ����ҽṹ����
 *  v��   ���ű��
 **********************************************************/
Symbol * struct_search(int v)
{
	if(tktable.size() > v)
		return tktable[v].sym_struct;
	else
		return NULL;
}

/***********************************************************
 *  ���ܣ������뺯������
 *  l��   �洢���ͣ��ֲ��Ļ���ȫ�ֵ�
 **********************************************************/
Symbol * sym_search(int v)
{
	if(tktable.size() > v)
		return tktable[v].sym_identifier;
	else
		return NULL;
}

void mk_pointer(Type *t);

/***********************************************************
 * ����:	�ʷ�������ʼ��
 **********************************************************/
void init_lex()
{
	TkWord* tp;
	static TkWord keywords[] = {
	/* ��������ָ��� 1-25 */
	{TK_PLUS,		NULL,	  "+",	NULL,	NULL},
	{TK_MINUS,		NULL,	  "-",	NULL,	NULL},
	{TK_STAR,		NULL,	  "*",	NULL,	NULL},
	{TK_DIVIDE,		NULL,	  "/",	NULL,	NULL},	
	{TK_MOD,		NULL,	  "%",	NULL,	NULL},
	{TK_EQ,			NULL,	  "==",	NULL,	NULL},
	{TK_NEQ,		NULL,	  "!=",	NULL,	NULL},
	{TK_LT,			NULL,	  "<",	NULL,	NULL},
	{TK_LEQ,		NULL,	  "<=",	NULL,	NULL},
	{TK_GT,			NULL,	  ">",	NULL,	NULL},
	{TK_GEQ,		NULL,	  ">=",	NULL,	NULL},
	{TK_ASSIGN,		NULL,	  "=",	NULL,	NULL},
	{TK_POINTSTO,	NULL,	  "->",	NULL,	NULL},
	{TK_DOT,		NULL,	  ".",	NULL,	NULL},
	{TK_AND,		NULL,	  "&",	NULL,	NULL},
	{TK_OPENPA,		NULL,	  "(",	NULL,	NULL},
	{TK_CLOSEPA,	NULL,	  ")",	NULL,	NULL},
	{TK_OPENBR,		NULL,	  "[",	NULL,	NULL},
	{TK_CLOSEBR,	NULL,	  "]",	NULL,	NULL},	
	{TK_BEGIN,		NULL,	  "{",	NULL,	NULL},
	{TK_END,		NULL,	  "}",	NULL,	NULL},
	{TK_SEMICOLON,	NULL,	  ";",	NULL,	NULL},
	{TK_COMMA,		NULL,	  ",",	NULL,	NULL},
	{TK_ELLIPSIS,	NULL,	"...",	NULL,	NULL},
	{TK_EOF,		NULL,	 "End_Of_File",	NULL,	NULL},
                     	     	    
    /* ���� 26-28 */ 
	{TK_CINT,		NULL,	 	"���ͳ���",	NULL,	NULL},        
	{TK_CCHAR,		NULL,		"�ַ�����",	NULL,	NULL},        
	{TK_CSTR,		NULL,		"�ַ�������",	NULL,	NULL},	      

	/* �ؼ��� 29-41 */        	     	     	  	
	{KW_CHAR,		NULL,		"char",	NULL,	NULL},
	{KW_SHORT,		NULL,		"short",	NULL,	NULL},
	{KW_INT,		NULL,		"int",	NULL,	NULL},
	{KW_VOID,		NULL,		"void",	NULL,	NULL},	
	{KW_STRUCT,		NULL,		"struct",	NULL,	NULL},
                     	     	     	  	
	{KW_IF,			NULL,		"if"	,	NULL,	NULL},  
	{KW_ELSE,		NULL,		"else",	NULL,	NULL},  
	{KW_FOR,		NULL,		"for",	NULL,	NULL},  
	{KW_CONTINUE,	NULL,		"continue",	NULL,	NULL},
	{KW_BREAK,		NULL,		"break",	NULL,	NULL},    
	{KW_RETURN,		NULL,		"return",	NULL,	NULL},
	{KW_SIZEOF,		NULL,		"sizeof",	NULL,	NULL},
	
	/* �ؼ��� 42-44 */
	{KW_ALIGN,		NULL,		"__align",	NULL,	NULL},
	{KW_CDECL,		NULL,		"__cdecl",	NULL,	NULL},
	{KW_STDCALL,	NULL,		"__stdcall",	NULL,	NULL},
	{0,				NULL,	NULL,	NULL,		NULL}
	};

    for (tp = &keywords[0]; tp->spelling != NULL; tp++)
		tktable.push_back(*tp);
	printf("tktable.size = %d \n", tktable.size());
		
}

void init()
{
	global_sym_stack.reserve(1024);
	local_sym_stack.reserve(1024);
	init_lex();

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

