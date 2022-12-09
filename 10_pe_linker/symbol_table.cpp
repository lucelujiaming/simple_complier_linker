// symbol_table.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pe_generator.h"
#include <vector>
#include "x86_generator.h"
#include "pe_generator.h"

extern std::vector<std::string> vecDllName;
extern std::vector<std::string> vecLib;

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;

std::vector<Symbol *> global_sym_stack;  //ȫ�ַ���ջ
std::vector<Symbol *> local_sym_stack;   //�ֲ�����ջ


extern int   g_output_type;
extern short g_subsystem;
extern std::vector<TkWord> tktable;
extern std::vector<char *> vecSrcFiles;

Type char_pointer_type,		// �ַ���ָ��
	 int_type,				// int����
	 default_func_type;		// ȱʡ��������

Symbol *sym_sec_rdata;			// ֻ���ڷ���

extern char *lib_path;

/***********************************************************
 *  ���ܣ������ŷ��ڷ���ջ��
 *  token_code��   ���ű��
 *  type��������������
 *  related_value��   ���Ź���ֵ
 **********************************************************/
Symbol * sym_direct_push(std::vector<Symbol *> &sym_stack, int token_code, Type * type, int related_value)
{
	Symbol * symElementPtr = (Symbol *)malloc(sizeof(Symbol)); //, *p;
    memset(symElementPtr, 0, sizeof(Symbol));
	symElementPtr->token_code =token_code;
	symElementPtr->typeSymbol.type = type->type;
	symElementPtr->typeSymbol.ref = type->ref;
	symElementPtr->related_value =related_value;
	symElementPtr->next = NULL;
	sym_stack.push_back(symElementPtr);
	// printf("\t ss.size = %d \n", ss.size());
	if (sym_stack.size() >= 1)
	{
		return sym_stack.back();
		// return &ss[0];
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
Symbol * sym_push(int token_code, Type * type, int storage_class, int related_value)
{
	Symbol *new_symbol_ptr, **tktable_member_pptr;
	TkWord *ts;
	// 1. �����ڷ���ջ�����һ�����š�
	if(local_sym_stack.size() == 0)
	{
		new_symbol_ptr = sym_direct_push(local_sym_stack, token_code, type, related_value);
		print_all_stack("sym_push local_sym_stack");
	}
	else
	{
		new_symbol_ptr = sym_direct_push(global_sym_stack, token_code, type, related_value);
		print_all_stack("sym_push global_sym_stack");
	}

	new_symbol_ptr->storage_class = storage_class;

	// ��Ϊ����¼�ṹ���Ա����������
	// ����������߼���Ϊ�������֡�
	// ��һ��(token_code & SC_STRUCT)����token_code�������Ϊ�ṹ����š�
	// �ڶ���(token_code < SC_ANOM)����token_code������Ų����������ţ��ṹ��Ա����������������
	if((token_code & SC_STRUCT) || (token_code < SC_ANOM))
	{
		// 2. ����token�ҵ���Ӧ�ĵ��ʽṹ��
		//    �ڽ����������֮ǰ�������Ѿ������﷨������
		//    ��ʱtktable�У�����sym_struct��Ա����sym_identifier��Ա������ֵ�ġ�
		//    ���ǵ�ʱ����ֵ�ǲ���ȫ������storage_class��related_value��Ա�ǲ�֪���ġ�
		//    ��token_codeҲ���ԡ�����ϣ���Щֵ����Ҫ
		ts = &(TkWord)tktable[token_code & ~SC_STRUCT];
		
		// ���漸�仰�Ľ�����ǣ�
		//    ts��sym_struct/sym_identifier��Աָ��new_symbol��
		//    ��new_symbol��prev_tok��Աָ��sym_struct/sym_identifier��Ա֮ǰ��ֵ��
		// �������ts��sym_struct/sym_identifier��Աָ��һ������
		// ��������϶���ͬһ�����ֵ�Symbol��
		// Ҳ�͵��ڣ���ts��sym_struct/sym_identifier������ͷ�ϲ��뵱ǰ����ӵķ��š�
		// ��Ҫ���������Ҫ��ϸ�Ķ�sym_pop���߼���

		// ����ǽṹ����š�����sym_struct��Ա��
		if(token_code & SC_STRUCT)
			tktable_member_pptr = &ts->sym_struct;
		// �������С��SC_ANOM�ķ��š�����sym_identifier��Ա��
		else
			tktable_member_pptr = &ts->sym_identifier;
		// ����ӵķ���ָ�򵥴ʱ��еı�ʶ�����š�
		new_symbol_ptr->prev_tok = *tktable_member_pptr;
		// tktable�ķ��ų�Աָ������ӵķ��š�
		*tktable_member_pptr = new_symbol_ptr;
		
		tktable[token_code & ~SC_STRUCT] = *ts;
		printf("sym_push:: (%s, %08X, %08X) \n", tktable[token_code & ~SC_STRUCT].spelling, 
			tktable[token_code & ~SC_STRUCT].sym_struct, tktable[token_code & ~SC_STRUCT].sym_identifier);
	}
	// ��������ӵķ���
	return new_symbol_ptr;
}

/***********************************************************
 *  ���ܣ����������ŷ���ȫ�ַ��ű���
 *  token_code��   ���ű��
 *  type��������������
 *  ����������ű�ʹ��func_sym_push������
 *  ���������֤�������Ŷ������ȫ�ַ���ջ��
 **********************************************************/
Symbol * func_sym_push(int token_code, Type * type)
{
	Symbol *new_func_symbol, **tktable_member_pptr;
	new_func_symbol = sym_direct_push(global_sym_stack, token_code, type, 0);
	print_all_stack("sym_push global_sym_stack");
	printf("tktable[%d].spelling = %s \n", token_code, tktable[token_code].spelling);
	// ps = &((TkWord *)&tktable[v])->sym_identifier;

	tktable_member_pptr = &(tktable[token_code].sym_identifier);
	// ��sym_identifier�������ϲ��롣
	while(*tktable_member_pptr != NULL)
		tktable_member_pptr = &(* tktable_member_pptr)->prev_tok;
	new_func_symbol->prev_tok = NULL;
	*tktable_member_pptr = new_func_symbol;
	return new_func_symbol;
}

/***********************************************************
 *  ����������ű�ͨ��var_sym_put������
 *  �����������ݱ����Ǿֲ���������ȫ�ֱ�����������Ӧ�ķ���ջ�С�
 **********************************************************/
Symbol * var_sym_put(Type * type, int storage_class, int token_code, int addr)
{
	Symbol *sym = NULL;
	if((storage_class & SC_VALMASK) == SC_LOCAL)
	{
		sym = sym_push(token_code, type, storage_class, addr);
	}
	else if((storage_class & SC_VALMASK) == SC_GLOBAL	// ��ȫ�ֱ�����
		 && (token_code != TK_CSTR))					// �������ַ�������
	{
		sym = sym_search(token_code);
		if(sym)
			printf("Error dual defined");
		else
			sym = sym_push(token_code, type, storage_class | SC_SYM, 0);
	}
	return sym;
}

/***********************************************************
 *  ���ܣ��������Ʒ���ȫ�ַ��ű�
 *  sec�� ������
 *  c��   ���Ź���ֵ
 **********************************************************/
Symbol * sec_sym_put(char * sec, int related_value)
{
	TkWord * tp;
	Symbol *sym;
	Type typeCurrent;
	typeCurrent.type = T_INT;
	tp = tkword_insert(sec); // , TK_CINT);
	// token_type = tp->tkcode ;
	set_current_token_type(tp->tkcode);
	// s = sym_push(token_type, &typeCurrent, SC_LOCAL, c);
	sym = sym_push(get_current_token_type(), 
					&typeCurrent, SC_LOCAL, related_value);
	return sym;
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
void sym_pop(std::vector<Symbol *> * pop, Symbol *new_top)
{
	Symbol *sym, **symTktablePtr;
	TkWord * ts;
	int token_code;

	// s = &(pop->back());
	sym = *(local_sym_stack.end() - 1);
	while(sym != new_top)
	{
		token_code = sym->token_code;
		// ����¼�ṹ���Ա����������
		if((token_code & SC_STRUCT) || token_code < SC_ANOM)
		{
			ts = &(TkWord)tktable[token_code & ~SC_STRUCT];
			if(token_code & SC_STRUCT)
				symTktablePtr = &ts->sym_struct;
			else
				symTktablePtr = &ts->sym_identifier;

			*symTktablePtr = sym->prev_tok ;
			tktable[token_code & ~SC_STRUCT] = *ts;
			printf("sym_pop:: (%s, %08X, %08X) \n", tktable[token_code & ~SC_STRUCT].spelling, 
				tktable[token_code & ~SC_STRUCT].sym_struct, tktable[token_code & ~SC_STRUCT].sym_identifier);
		}
		// pop->erase(pop->begin());
		free(sym);
		pop->pop_back();
		if(pop->size() > 0)
		{
			// s = &(pop->back());
			sym = *(local_sym_stack.end() - 1);
		}
		else
			break;
	}
}

/***********************************************************
 *  ���ܣ����ҽṹ����
 *  v��   ���ű��
 **********************************************************/
Symbol * struct_search(int token_code)
{
#if 0
	printf("\n -- struct_search -- ");
	for (int idx = 42; idx < tktable.size(); idx++)
	{
		printf(" (%s, %08X, %08X) ", 
			tktable[idx].spelling, tktable[idx].sym_struct, 
			tktable[idx].sym_identifier);
	}
#endif
	printf(" ---- \n");
	if(tktable.size() > token_code)
		return tktable[token_code].sym_struct;
	else
		return NULL;
}

/***********************************************************
 * ����:	���ҽṹ���� 
 * v:		���ű��
 **********************************************************/
Symbol * sym_search(int token_code)
{
	if(tktable.size() > token_code)
	{
		TkWord tmpTkWord = tktable[token_code];
		return tmpTkWord.sym_identifier;
	}
	else
		return NULL;
}


/************************************************************************/
/* ����:		�������ͳ���                                            */
/* typeCal:		��������ָ��                                            */
/* align:		����ֵ                                                  */
/* ����ֵ:	                                                            */
/*     ָ�����ͷ���-1���������ͻ�����ʵ�ʵĳ��ȡ�                     */
/************************************************************************/
int type_size(Type * typeCal, int * align)
{
	Symbol *sym;
	int bt;
	int PTR_SIZE = 4;
	bt = typeCal->type & T_BTYPE;
	switch(bt)
	{
	case T_STRUCT:
		sym = typeCal->ref;
		*align = sym->storage_class;
		return sym->related_value ;
	case T_PTR:
		// �����ָ�����顣��������������
		// 1. һ�������������������int AA[717]��
		//    ��������£����ȵ���typeSymbol�Ĵ�С����related_value��
		//    ����int AA[717]�ĳ��Ⱦ���4 * 717 = 2868��
		// 2. һ�������鳣����������"Hello world!"�����ַ�����
        //    ��������£�related_value����-1��ֱ�ӷ��ظ��������ϲ㴦��
		//    ��Ϊ��������£����ȿ���ͨ��tokenֱ�Ӽ��㡣
		if(typeCal->type & T_ARRAY)
		{
			sym = typeCal->ref;
			return type_size(&sym->typeSymbol, align) * sym->related_value;
		}
		// ����ָ��ĳ���Ϊ32λ��Ҳ����4��
		else
		{
			*align = PTR_SIZE;
			return PTR_SIZE;
		}
	case T_INT:
		*align = 4;
		return 4 ;
	case T_SHORT:
		*align = 2;
		return 2 ;
	default:			// char, void, function
		*align = 1;
		return 1 ;
	}
}

/***********************************************************
 * ����:	����t��ָ�����������
 * t:		ָ������
 **********************************************************/
Type *pointed_type(Type *typePointer)
{
    return &typePointer->ref->typeSymbol;
}

/***********************************************************
 * ����:	����t��ָ����������ͳߴ�
 * t:		ָ������
 **********************************************************/
int pointed_size(Type *typePointer)
{
    int align;
    return type_size(pointed_type(typePointer), &align);
}

/*********************************************************** 
 * ����:	�����ֽڶ���λ��
 * n:		δ����ǰֵ
 * align:   ��������
 **********************************************************/
int calc_align(int value , int align)
{
	return (value + align -1) & (~(align -1));
}

/*********************************************************** 
 * ����:	����ָ������
 * t:		ԭ��������
 **********************************************************/
void mk_pointer(Type *typePointer)
{
	Symbol *sym;
    sym = sym_push(SC_ANOM, typePointer, 0, -1);
    typePointer->type = T_PTR ;
    typePointer->ref = sym;
}

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
	// ��������һ�С���
	{KW_WHILE,		NULL,		"while",	NULL,	NULL},
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
	// printf("tktable.size = %d \n", tktable.size());

}

void print_all_TkWord()
{
	for (int idx = 0; idx < tktable.size(); idx++)
	{
		printf("tktable[%d].spelling = %s \n", idx, tktable[idx].spelling);
	}
}

void init()
{
	g_output_type = OUTPUT_EXE;
	g_subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

	vecSrcFiles.reserve(1);
	vecLib.reserve(4);
	vecDllName.reserve(4);
	operand_stack.reserve(OPSTACK_SIZE);
	// The begin is reserved
	operand_stack_top = operand_stack.begin();
	
	global_sym_stack.reserve(1024);
	local_sym_stack.reserve(1024);
	init_lex();

//	sym_sec_rdata = sec_sym_put(".rdata", 0);
	int_type.type = T_INT;
	char_pointer_type.type = T_CHAR;
	mk_pointer(&char_pointer_type);
	default_func_type.type = T_FUNC;
//	default_func_type.ref =

	init_coff();
	lib_path = get_lib_path();
}

void cleanup()
{
	int idx;
	sym_pop(&global_sym_stack, NULL);
	for(idx = 0; idx < global_sym_stack.size(); ++idx)
	{
		free(global_sym_stack[idx]);
	}
	global_sym_stack.clear();
	for(idx = 0; idx < local_sym_stack.size(); ++idx)
	{
		free(local_sym_stack[idx]);
	}
	local_sym_stack.clear();
	for(idx = 0; idx < vecSection.size(); ++idx)
	{
		free(vecSection[idx]);
	}
	vecSection.clear();
	
//	for(idx = 0; idx < global_sym_stack.size(); ++idx)
//	{
//
//	}
	tktable.clear();
	vecDllName.clear();
	vecSrcFiles.clear();
	vecLib.clear();
}

//	int main(int argc, char* argv[])
//	{
//		init();
//		token_init(argv[1]);
//		get_token();
//		translation_unit();
//		printf("Hello World!\n");
//		token_cleanup();
//		return 0;
//	}

