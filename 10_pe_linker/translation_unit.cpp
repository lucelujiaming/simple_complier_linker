// translation_unit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include "get_token.h"
#include "reg_manager.h"
#include "instruction_operator.h"

e_SynTaxState syntax_state = SNTX_NUL;
int syntax_level;

int return_symbol_pos;			// ��¼returnָ��λ��

extern Section *sec_text,	// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// �������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű���	
		*sec_dynsymtab;		// ���ӿ���Ž�

extern int sec_text_opcode_offset ;	 	// ָ���ڴ����λ��
extern int function_stack_loc ;			// �ֲ�������ջ��λ��
extern Symbol *sym_sec_rdata;			// ֻ���ڷ���

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;
extern std::vector<Operand>::iterator operand_stack_last_top;

extern std::vector<Symbol> global_sym_stack;  //ȫ�ַ���ջ
extern std::vector<Symbol> local_sym_stack;   //�ֲ�����ջ
extern std::vector<TkWord> tktable;

extern Type char_pointer_type,		// �ַ���ָ��
	 int_type,				// int����
	 default_func_type;		// ȱʡ��������

int type_specifier(Type * type);
void declarator(Type * type, int * iTokenTypePtr, int * force_align);
void direct_declarator(Type * type, int * iTokenTypePtr, int func_call);
void direct_declarator_postfix(Type * type, int func_call);
void parameter_type_list(Type * type, int func_call); // (int func_call);

void compound_statement(int * break_address, int * continue_address);
void if_statement(int *break_address, int *continue_address);
void break_statement(int *break_address);
void return_statement();
void continue_statement(int *continue_address);
void for_statement(int *break_address, int *continue_address);
void while_statement(int *break_address, int *continue_address);
void expression_statement();

void expression();
void assignment_expression();
void equality_expression();
void relational_expression();

void external_declaration(e_StorageClass iSaveType);

Section * allocate_storage(Type * typeCurrent, int storage_class, 
						   e_Sec_Storage sec_area, int token_code, int *addr);
void init_variable(Type * type, Section * sec, int c); // , int v);

void print_error(char * strErrInfo)
{
	printf("<ERROR> %s", strErrInfo);
	exit(1);
}

void print_tab(int nTimes)
{
	int seq = 0;
	for (; seq < nTimes; seq++)
	{
		printf("\t");
	}
}

void syntax_indent()
{
	switch(syntax_state) {
	case SNTX_NUL:
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
		break;
	case SNTX_SP:
		printf(" ");
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
		break;
	case SNTX_LF_HT:
		{
			if (get_current_token_type() == TK_END)
			{
				syntax_level--;
			}
			printf("\n");
			print_tab(syntax_level);
		}
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
		break;
	case SNTX_DELAY:
		break;
	default:
		break;
	}
	syntax_state = SNTX_NUL;
}

/* <function_calling_convention> ::= <KW_CDECL> | <KW_STDCALL>            */
/* Such as __stdcall__  | __cdecl                                         */ 
/*  "__cdecl",       KW_CDECL,		// __cdecl�ؼ��� standard c call      */
/*	"__stdcall",     KW_STDCALL,    // __stdcall�ؼ��� pascal c call      */
void function_calling_convention(int *fc)
{
	*fc = KW_CDECL;  // Set default value with __cdecl
	// �����������ָ���˵��÷�ʽ���ͽ��д���������Ͳ��ô�������ǰtoken���䡣
	if(get_current_token_type() == KW_CDECL || get_current_token_type() == KW_STDCALL)
	{
		*fc = get_current_token_type();
		syntax_state = SNTX_SP;
		get_token();
	}
}

/* <struct_member_alignment> ::= <KW_ALIGN><TK_OPENPA><TK_INT><TK_CLOSEPA> */
/* Such as __align(4)                                                        */ 
/*   "__align",       KW_ALIGN,		// __align�ؼ���	                   */
void struct_member_alignment(int * force_align) // (int *fc)
{
	int align = 1;  // Default value of __align is one.
	// Do not need indent or not
	if(get_current_token_type() == KW_ALIGN)
	{
		get_token();
		skip_token(TK_OPENPA);
		if(get_current_token_type() == TK_CINT)
		{
			get_token();
			// ex. : Get 4 for __align(4)
			align = atoi(get_current_token());  // as a TK_CINT
		}
		else
		{
			print_error("Need intergat\n");
		}
		// *fc = token;
		skip_token(TK_CLOSEPA); 
		// Calculate and set force_align.
		if(align !=1 && align !=2 && align !=4)
			align = 1;  // Only support 1, 2, 4.
		align |= ALIGN_SET;
		*force_align = align;
	}
	else
		*force_align = 1; // Default is one.
}

/* <declarator> ::= {<TK_STAR>}[<function_calling_convention>]               */
/*                    [<struct_member_alignment>]<direct_declarator>         */
/* Such as * __cdecl __align(4) function(int a, int b)                       */ 
void declarator(Type * type, int * iTokenTypePtr, int * force_align)
{
	int fc ;
	while(get_current_token_type() == TK_STAR)		// * �Ǻ�
	{
		mk_pointer(type);
		get_token();
	}
	// ��������ʵ��SC�����������ѡ����
	// Ҳ����һ�����������ͺ�������һ����˵�����п����ǲ�������������Լ���Ͷ���ġ�
	// ��Ȼ���������봦����
	function_calling_convention(&fc);
	if(force_align)
	{
		struct_member_alignment(force_align);  // &fc
	}
	direct_declarator(type, iTokenTypePtr, fc);
}

/* <direct_declarator> ::= <IDENTIFIER><direct_declarator_postfix>              */
void direct_declarator(Type * type, int * iTokenTypePtr, int func_call)   // Not support argv[] usage
{
	if(get_current_token_type() >= TK_IDENT)  // �����������Ǳ��������������Ǳ����ؼ��֡� 
	{
		*iTokenTypePtr = get_current_token_type();
		get_token();
	}
	else
	{
		print_error("direct_declarator can not be TK_IDENT\n");
	}
	direct_declarator_postfix(type, func_call);
}

/* <direct_declarator_postfix> :: = {<TK_OPENBR><TK_CINT><TK_CLOSEBR>       (1)  */
/*                         | <TK_OPENBR><TK_CLOSEBR>                        (2)  */
/*                         | <TK_OPENPA><parameter_type_list><TK_CLOSEPA>�� (3)  */
/*                         | <TK_OPENPA><TK_CLOSEPA>}                       (4)  */
/* Such as var, var[5], var(), var(int a, int b), var[]                          */ 
void direct_declarator_postfix(Type * type, int func_call)
{
	int array_size;
	Symbol * symArrayPtr;
	// <TK_OPENPA><parameter_type_list><TK_CLOSEPA> | <TK_OPENPA><TK_CLOSEPA>
	// Such as : var(), var(int a, int b)
	if(get_current_token_type() == TK_OPENPA)         
	{
		parameter_type_list(type, func_call);
	}
	// <TK_OPENBR><TK_CINT><TK_CLOSEBR> | <TK_OPENBR><TK_CLOSEBR>
	// Such as : var[5], var[] 
	else if(get_current_token_type() == TK_OPENBR)   
	{
		get_token();
		array_size = -1;
		if(get_current_token_type() == TK_CINT)
		{
			array_size = atoi(get_current_token());  // as a TK_CINT
			get_token();
			// n = tkvalue;  // ??
		}
		else
		{
			print_error("Need inter\n");
		}
		skip_token(TK_CLOSEBR);
		direct_declarator_postfix(type, func_call);    // Nesting calling
		// �������С���浽���ű��У���Ϊһ���������š�
		symArrayPtr = sym_push(SC_ANOM, type, 0, array_size);
		type->type = T_ARRAY | T_PTR;
		type->ref = symArrayPtr;
	}
}

/**************************************************************************/
/*  ���ܣ������β����ͱ�                                                  */
/*  func_call����������Լ��                                               */
/*                                                                        */
/*  <parameter_type_list> ::= <parameter_list>                            */
/*             | <parameter_list><TK_COMMA><TK_ELLIPSIS>                  */
/*                 TK_ELLIPSIS is "...",   ... ʡ�Ժ�                     */
/*  <parameter_list> ::= <parameter_declaration>                          */
/*                      {<TK_COMMA><parameter_declaration>}               */
/*      Such as func(int a), func(int a, int b)                           */
/*  <parameter_declaration> ::= <type_specifier>{<declarator>}            */
/*      Such as  int a                                                    */
/*  �ȼ�ת�����ķ���                                                      */
/*  <parameter_type_list>::=<type_specifier>{<declarator>}                */
/*   {<TK_COMMA><type_specifier>{<declarator>} } <TK_COMMA><TK_ELLIPSIS>  */
/**************************************************************************/
void parameter_type_list(Type * type, int func_call) // (int func_call)
{
	int iTokenType;
	Symbol **lastSymPtr, *sym, *firstSym;
	Type typeCurrent ;
	get_token();
	firstSym = NULL;
	lastSymPtr = &firstSym;
	
	while(get_current_token_type() != TK_CLOSEPA)   // get_token until meet ) ��Բ����
	{
		if(!type_specifier(&typeCurrent))
		{
			print_error("Invalid type_specifier\n");
		}
		// Translate one parameter declaration
		declarator(&typeCurrent, &iTokenType, NULL);			
		sym = sym_push(iTokenType|SC_PARAMS, &typeCurrent, 0, 0);
		*lastSymPtr = sym;
		lastSymPtr  = &sym->next;
		
		if(get_current_token_type() == TK_CLOSEPA) // We encounter the ) after one parameter
		{
			break;
		}
		skip_token(TK_COMMA);
		if(get_current_token_type() == TK_ELLIPSIS) // We encounter ... 
		{
			// func_call = KW_CDECL ; // record the function_calling_convention
			get_token();
			break;
		}
	}
	syntax_state = SNTX_DELAY;
	skip_token(TK_COMMA);
	if(get_current_token_type() == TK_BEGIN)            // func(int a) {
		syntax_state = SNTX_LF_HT;
	else                             // func(int a);
		syntax_state = SNTX_NUL;
	syntax_indent();
	
	sym = sym_push(SC_ANOM, type, func_call, 0);
	sym->next = firstSym;
	type->type = T_FUNC;
	type->ref = sym;
}

// ------------------ func_body 
/************************************************
 *  <func_body> ::= <compound_statement>
 *  ��ΪֻҪ����<{ �������>���Ǹ�����䣬����<�������>�ĸ����<������>����Ҫ��
 *  <������>����һ��Ƚ������<�������>��
 ***********************/
void func_body(Symbol * sym)
{
	// 1. ���ӻ����COFF����
	sec_text_opcode_offset = sec_text->data_offset;
	//    ����sym->related_valueΪ����COFF���ű�����š�
	coffsym_add_update(sym, sec_text_opcode_offset, 
		sec_text->index, CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
	
	/* 2. ��һ���������ŵ��ֲ����ű��� */
	sym_direct_push(local_sym_stack, SC_ANOM, &int_type, 0);

	// 3. ���ɺ�����ͷ���롣
	gen_prologue(&(sym->typeSymbol));
	return_symbol_pos = 0;

	// 4. ���ø�����䴦�����������������е���䡣
	// print_all_stack("sym_push local_sym_stack");
	compound_statement(NULL, NULL);
	// print_all_stack("Left local_sym_stack");

	// 5. �������غ󣬻���ص�ַ��
	jmpaddr_backstuff(return_symbol_pos, sec_text_opcode_offset);
	
	// 6. ���ɺ�����β���롣
	gen_epilogue();
	sec_text->data_offset = sec_text_opcode_offset;

	/* 7. ��վֲ����ű�ջ */
	if(local_sym_stack.size() > 0)
	{
		sym_pop(&local_sym_stack, NULL);
	}
	// print_all_stack("Clean local_sym_stack");
}

void print_all_stack(char* strPrompt)
{
	int idx;
	return;
	printf("\n-------------------%s---------------------------\n", strPrompt);
	for(idx = 0; idx < global_sym_stack.size(); ++idx)
	{
		printf("\t global_sym_stack[%d].TokenType = %08X value = %d type = %d\n", idx, 
			global_sym_stack[idx].token_code, 
			global_sym_stack[idx].related_value, 
			global_sym_stack[idx].typeSymbol);
	}
	for(idx = 0; idx < local_sym_stack.size(); ++idx)
	{
		printf("\t  local_sym_stack[%d].TokenType = %08X value = %d type = %d\n", idx, 
			local_sym_stack[idx].token_code, 
			local_sym_stack[idx].related_value, 
			local_sym_stack[idx].typeSymbol);
	}
	printf("\t tktable.size = %d \n --- ", tktable.size());
	for (idx = 40; idx < tktable.size(); idx++)
	{
		printf(" %s ", tktable[idx].spelling);
	}
	printf(" --- \n----------------------------------------------\n");
}

// ��ֵ���Ĵ���������ʾ��
/************************************************************************/
/* <initializer> ::= <assignment_expression>                            */
/************************************************************************/
/* ���ܣ� ������ʼ��                                                    */
/* type�� �������͡�Ҳ������ֵ�����͡�                                  */
/* value���������ֵ������ȫ�ֱ������ַ�������Ϊ���ڲ�ƫ������          */
/* sec��  �������ڽڡ�����ȫ�ֱ������ַ�������Ϊ��Ӧ�ڵ�ַ��            */
/*                   ���ھֲ�����ΪNULL��                               */
/************************************************************************/
/* �����߼����£�                                                       */
/*   1. �������������֧��else���֡�                                    */
/*      1.1 ���ȵ���assignment_expression���б���ʽ������               */
/*          ��Ϊ��ֵ��Ҳ�п�����һ������ʽ��                            */
/*      1.2 ֮�����ǵ���init_variable��ɸ�ֵ������                   */
/*   2. ��������ĸ���֧��if���֡��ò��ֶ����ַ�����ֵ�������⴦����    */
/*      �����֧������������·����                                      */
/*      2.1 �ֲ��ַ���������char str1[] = "AAAAA"; ��������Ϊ��         */
/*	        �����ǵ���assignment_expression���б���ʽ������ʱ��       */
/*		    �����ֵ��һ���ַ����������primary_expression�д�����      */
/*		    ��TK_CSTR��֧��Ϊ��������ռ䣬�ѱ���������ű���           */
/*		    ֮��Ƕ�׵���initializer��ɱ�����ʼ����                     */
/*      2.2 ȫ���ַ���������char g_str1[] = "AAAAA"; ��������Ϊ��       */
/*          ��external_declaration�У�Ϊ��������ռ䣬�ѱ���������ű���*/
/*		    ֮��Ƕ�׵���initializer��ɱ�����ʼ����                     */
/*		��Ϊǰ�����allocate_storage����ռ��ʱ�򣬷���ֵsec���ǿա�   */
/*		ͬʱ����ǰ��ָ����T_ARRAY���ͱ�־��                             */
/*		�⵼�����ǽ���initializer������if���֡�                         */
/*		���ַ���memcpy���ڵ�ָ��λ�á�                                  */
/************************************************************************/
void initializer(Type * typeToken, int value, Section * sec)
{
	// ��������ݱ����������ʼ�����������Ƕ���������������
	// �����ǽ����ڶ���������ʱ�����Ǿͻ���������֧��
	//     char g_char = 'a';  
	//     char ss[5] = "SSSSS";
	if((typeToken->type & T_ARRAY) && sec)
	{
	    // ֻ��Ҫ����ֱֵ�Ӹ��Ƶ����ڲ����ɡ����ƺ�sec->data�����ݱ�ɣ�
		//   "aSSSSS"
		memcpy(sec->data + value, get_current_token(), strlen(get_current_token()));
		get_token();
	}
	else
	{
		// ����ȫ�ֱ������ַ��������;ֲ���������ֵ��
		assignment_expression();
		init_variable(typeToken, sec, value); // , 0);
		// get_token can not move here. It only work when token == TK_CSTR
		// get_token();
	}
}

/************************************************************************/
/* ���ܣ� ������ʼ��                                                    */
/* type�� �������͡�Ҳ������ֵ�����͡�                                  */
/* sec��  �������ڽڡ�����ȫ�ֱ������ַ�������Ϊ��Ӧ�ڵ�ַ��            */
/*                   ���ھֲ�����ΪNULL��                               */
/* value���������ֵ������ȫ�ֱ������ַ�������Ϊ���ڲ�ƫ������          */
/* idx��  �������ű�š����ֵ����û���á�                              */
/************************************************************************/
/* ���������initializer�У���assignment_expression�����󣬱����á�     */
/* �����漰������������⸳ֵ������                                     */
/* ���ڼ������˵����ֵ�������ı��������ڴ��е�һ����ַ��               */ 
/* ����ϣ�����ȫ�ֱ������ַ���������������һ���ڵ�ַ��                 */
/*       �����ڶ��ھֲ�������������һ��operand_stackԪ�ء�              */
/************************************************************************/
/*   1. ��assignment_expression�����У�ȫ�ֱ������ַ��������;ֲ������� */
/*      ��ֵ�Ѿ���primary_expression�����б�ѹջ��                      */
/*   2. ȫ�ֱ������ַ��������������                                    */
/*      ������������ݱ����ڽ��ϣ�����ֻ��Ҫд��Ӧ�Ľڼ��ɡ�            */
/*   3. �ֲ������������                                                */
/*      �����ǽ���init_variable������ʱ����ֵλ��ջ����               */
/*      ����ֻ��Ҫ����ֵ����ѹջ����ջ��Ԫ��Ϊ��ֵ����ջ��Ԫ��Ϊ��ֵ��  */
/*      ֮��ִ��operand_swap����ջ��Ԫ�ر�Ϊ��ֵ����ջ��Ԫ�ر�Ϊ��ֵ��  */
/*      ������store_zero_to_one��ջ����ֵ�����ջ����ֵ���ɡ�         */
/************************************************************************/
void init_variable(Type * type, Section * sec, int value) // , int idx)
{
	// 2022/11/14
	int bt;
	void * ptr;
	// ����ڵ�ַ��Ϊ�գ�˵����ʼ������ȫ�ֱ������ַ���������
	if (sec)
	{
		if(operand_stack_top)
		{
			if ((operand_stack_top->storage_class &(SC_VALMASK | SC_LVAL)) != SC_GLOBAL)
			{
				print_error("Use constant to initialize the global variable");
			}
		}
		// �õ��������͡�
		bt = type->type & T_BTYPE;
		// �õ�����ȫ�ֱ������ַ����������ڴ�λ�á�
		ptr = sec->data + value;
		// ���½��ж�Ӧ��λ�á�
		switch(bt) {
		case T_CHAR:
			*(char *)ptr = operand_stack_top->operand_value;
			break;
		case T_SHORT:
			*(short *)ptr = operand_stack_top->operand_value;
			break;
		default:
			if(operand_stack_top)
			{
				if(operand_stack_top->storage_class & SC_SYM)
				{
					coffreloc_add(sec, 
						operand_stack_top->sym, value, IMAGE_REL_I386_DIR32);
				}
			}
			*(int *)ptr = operand_stack_top->operand_value;
			break;
		}
		operand_pop();
	}
	// ����ڵ�ַ��Ϊ�գ�˵����ʼ�����Ƿ���ջ�ϵľֲ�������
	else
	{
		// �����ֵ�����顣��ô���������ʼ�������磺
		//     char  str1[]  = "str 1";
		if (type->type & T_ARRAY)
		{
			operand_push(type, SC_LOCAL | SC_LVAL, value);
			operand_swap();
			array_initialize();
		}
		else
		{
			// ���ھֲ���������ֵ����һ��operand_stackԪ�ء�ֻ�����Ͳ����á�ֱ�Ӱ�����ѹջ���ɡ�
			// ��ѹջ֮ǰ��ջ��Ԫ��Ϊ��ֵ��ѹջ�Ժ�ջ��Ԫ��Ϊ��ֵ���ʹ�ջ��Ԫ��Ϊ��ֵ��
			operand_push(type, SC_LOCAL | SC_LVAL, value);
			// ����ջ��Ԫ�غʹ�ջ��Ԫ�ء�ջ��Ԫ�ر�Ϊ��ֵ����ջ��Ԫ�ر�Ϊ��ֵ��
			operand_swap();
			// ��ջ��������Ҳ������ֵ�������ջ����������Ҳ������ֵ�С�
			store_zero_to_one();
			// ��ֵ��ɺ�ջ��������Ҳ������ֵ��û���ˡ�ֱ�ӵ����������ɡ�
			// ���ﲹ��һ�£��������������ɻ����롣
			// �����ֵ�Ǳ���ʽ����ǰ���assignment_expression���������ɶ�Ӧ�Ļ����롣����ϲ��õ��ġ�
			operand_pop();
		}
	}
}

// ------------------ ��䣺statement
/************************************************************************/
/* ����:	         �������                                           */
/* break_address:	 break��תλ�á���ʱû���á�                        */
/* continue_address: continue��תλ�á���ʱû���á�                     */
/*                                                                      */
/* <statement>::=<compound_statement>                                   */
/*             | <if_statement>                                         */
/*             | <return_statement>                                     */
/*             | <break_statement>                                      */
/*             | <continue_statement>                                   */
/*             | <for_statement>                                        */
/*             | <expression_statement>                                 */
/************************************************************************/
void statement(int *break_address, int *continue_address)
{
	switch(get_current_token_type())
	{
	case TK_BEGIN:
		compound_statement(break_address, continue_address);			// �������
		break;
	case KW_IF:
		if_statement(break_address, continue_address);
		break;
	case KW_RETURN:
		return_statement();
		break;
	case KW_BREAK:
		break_statement(break_address);
		break;
	case KW_CONTINUE:
		continue_statement(continue_address);
		break;
	case KW_FOR:
		for_statement(break_address, continue_address);
		break;
	case KW_WHILE:
		while_statement(break_address, continue_address);
		break;
	default:
		expression_statement();
		break;
	}
}

/************************************************************************/
/* <compound_statement> ::= <TK_BEGIN>                                  */
/*                             {<declaration>} {<statement>}            */
/*                          <TK_END>                                    */
/************************************************************************/
int is_type_specifier(int token_code)
{
	switch(token_code)
	{
	case KW_CHAR:
	case KW_SHORT:
	case KW_INT:
	case KW_VOID:
	case KW_STRUCT:
		return 1;
	default:
		break;
	}
	return 0;
}

/************************************************************************/
/* ����:	         �����������                                       */
/* break_address:	 break��תλ��                                      */
/* continue_address: continue��תλ��                                   */
/*                                                                      */
/* <compound_statement>::=<TK_BEGIN>                                    */
/*                           {<declaration>}{<statement>}               */
/*                        <TK_END>                                      */
/************************************************************************/
void compound_statement(int * break_address, int * continue_address)
{
	Symbol *sym;
	// s = &(local_sym_stack[0]);
	sym = local_sym_stack.end() - 1;
	// local_sym_stack.erase(&local_sym_stack[0]);
	// local_sym_stack.erase(local_sym_stack.end() - 1);
	printf("\t local_sym_stack.size = %d \n", local_sym_stack.size());
	
	syntax_state = SNTX_LF_HT;
	syntax_level++;

	get_token();
	while(is_type_specifier(get_current_token_type()))
	{
		external_declaration(SC_LOCAL);
	}

	while(get_current_token_type() != TK_END)
	{
		if(get_current_token_type() == TK_EOF)
			break;
		char temp_str[128];
		sprintf(temp_str, "Execute %s statement", get_current_token());
		print_all_stack(temp_str);
		statement(break_address, continue_address);
	}
	syntax_state = SNTX_LF_HT;
	printf("\t local_sym_stack.size = %d \n", local_sym_stack.size());
	sym_pop(&local_sym_stack, sym);
	get_token();
}

/************************************************************************/
/* <expression_statement> ::= <TK_SEMICOLON>|<expression><TK_SEMICOLON> */
/*   TK_SEMICOLON �� ; �ֺ�                                             */
/************************************************************************/
void expression_statement()
{
	if(get_current_token_type() != TK_SEMICOLON)
	{
		expression();
		// ������һ������ʽ����һ��ջ��
		operand_pop();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);

}

/************************************************************************/
/* <if_statement> ::= <KW_IF><TK_OPENPA><expression>                    */
/*          <TK_CLOSEPA><statement>[<KW_ELSE><statement>]               */
/************************************************************************/
/* һ������ if(a > b) ����������ָ�                                */
/*   MOV EAX, DWORD PTR SS: [EBP-8]                                     */
/*   MOV ECX, DWORD PTR SS: [EBP-4]                                     */
/*   CMP ECX, EAX                                                       */
/*   JLE scc_anal.0040123F                                              */
/* ���Կ���������Ӧ��ϵ���Ƿǳ����Եġ������ǰѱ�������EAX��ECX��       */
/* ֮�����CMP���бȽϡ����ݱȽϽ�������a������b������ת��else��֧��  */
/* ��֮�����a����b��˵���������ϣ�ֱ�����к������伴�ɣ�������ת��   */
/*                                                                      */
/* һ�� else ������һ��ָ�                                         */
/*   JMP scc_anal. 00401245                                             */
/* ִ�е�else���˵����IF��������ִ�����ˣ�ֱ������ELSE���ּ��ɡ�     */
/************************************************************************/
void if_statement(int *break_address, int *continue_address)
{
	int if_jmp_addr, else_jmp_addr;
	syntax_state = SNTX_SP;

	get_token();
	skip_token(TK_OPENPA);
	expression();

	syntax_state = SNTX_LF_HT;
	skip_token(TK_CLOSEPA);
	// ΪIF�������������תָ�
	if_jmp_addr = gen_jcc(0);
	statement(break_address, continue_address);

	if(get_current_token_type() == KW_ELSE)
	{
		syntax_state = SNTX_LF_HT;
		get_token();
		// ΪELSE���������תָ�
		else_jmp_addr = gen_jmpforward(0);
		// ΪIF���������Ե�ַ��
		jmpaddr_backstuff(if_jmp_addr, sec_text_opcode_offset);
		statement(break_address, continue_address);
		// ΪELSE���������Ե�ַ��
		jmpaddr_backstuff(else_jmp_addr, sec_text_opcode_offset);
	}
	else
	{
		// ΪIF���������Ե�ַ��
		jmpaddr_backstuff(if_jmp_addr, sec_text_opcode_offset);
	}
}	

/************************************************************************/
/*  <for_statement> ::= <KW_FOR><TK_OPENPA><expression_statement>       */
/*        <expression_statement><expression><TK_CLOSEPA><statement>     */
/************************************************************************/
/* һ������ for(i = 0; i < 10; i++) ���������µ�ָ�                */
/* �����Ǹ���ֵ��i = 0��Ӧ��ָ������:                                   */
/*  1. MOV EAX, 0                                                       */
/*  2. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* ����һ���򵥵ĸ�ֵ������                                             */
/*  1. ��0����EAX��                                                     */
/*  2. ֮���������С�                                                 */
/*                                                                      */
/* Ȼ����i < 10��Ӧ��ָ������:                                          */
/*  3. MOV EAX, DWORD PTR SS��[EBP-4]                                   */
/*  4. CMP EAX, 0A                                                      */
/*  5. JGE scc_anal.0040128D                                            */
/*  6. JMP scc_anal.00401276                                            */
/* �����������£�                                                       */
/*  3 & 4. �ѱ�����10���бȽϡ�                                         */
/*  5. �����10�󣬾���ת������for�����Ľ�β��                        */
/*  6. ���򣬾���ת������for��������档                                */
/*                                                                      */
/* Ȼ����i++��Ӧ��ָ������:                                             */
/*  7. MOV EAX, DWORD PTR SS��[EBP-4]                                   */
/*  8. ADD EAX, 1                                                       */
/*  9. MOV DWORD PTR SS: [EBP-4], EAX                                  */
/* 10. JMP SHORT scc_anal.0040125A                                      */
/* �����������£�                                                       */
/* 7 & 8 & 9. �ѱ�����һ��                                              */
/* 10. Ȼ����ת��i < 10�ĵط���                                         */
/*                                                                      */
/* ������arr[i] = i��Ӧ��ָ��:                                          */
/* 11. MOV  EAX, 4                                                      */
/* 12. MOV  ECX, DWORD PTR SS: [EBP-4]                                  */
/* 13. IMUL ECX, EAX                                                    */
/* 14. LEA  EAX, DWORD PTR SS: [EBP-2C]                                 */
/* 15. ADD  EAX, ECX                                                    */
/* 16. MOV  ECX, DWORD PTR SS: [EBP-4]                                  */
/* 17. MOV  DWORD PTR DS: [EAX], ECX                                    */
/* 18. JMP  SHORT scc_anal.0040126B                                     */
/* ���Կ����߼�Ҳ�ǳ��򵥡�                                             */
/* 11. ����EAX����Ϊ4��                                                 */
/* 12. ֮��ȡ��i��ֵ����ECX��                                           */
/* 13. ֮���ECX��EAX��ˣ�����ECX��                                    */
/* 14. ֮��ȡ�������׵�ַ����EAX��                                      */
/* 15. ֮�����ECX���õ���r[i]�ĵ�ַ���������EAX��                     */
/* 16. ȡ��i��ֵ����ECX��                                               */
/* 17. ���arr[i] = i�ĸ�ֵ������                                       */
/* 18. ���ѭ���壬��ת��i++ָ���λ�á�                                */
/************************************************************************/
void for_statement(int *break_address, int *continue_address)
{
	// int b;
	int for_end_address, for_inc_address,for_exit_cond_address,for_body_address;
	get_token();
	skip_token(TK_OPENPA);

	// Chapter I
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		operand_pop();
		skip_token(TK_SEMICOLON);
	}
	else 											// Such as for(; n < 100; n++)
	{
		skip_token(TK_SEMICOLON);
	}
	// ��������λ��for���ĵ�һ���ֺ�λ�á����ȼ�¼��ѭ��ͷ���˳�������λ�á�
	for_exit_cond_address = sec_text_opcode_offset;
	// ��ʼ��ѭ��ͷ���ۼ�������λ�á���Ϊѭ���ۼ������п��ܲ����ڡ�
	for_inc_address = sec_text_opcode_offset;
	// ��ʼ��ѭ�������λ�á�
	for_end_address = 0;
	// b = 0;
	// Chapter II
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		// ����ѭ��ͷ���˳�����ת��䡣���磺5. JGE scc_anal.0040128D
		// ��ת��ַ�ڴ�����for����Ժ���
		for_end_address = gen_jcc(0);
		skip_token(TK_SEMICOLON);
	}
	else 						// Such as for(n = 0; ; n++)
		skip_token(TK_SEMICOLON);

	// Chapter III
	if(get_current_token_type() != TK_CLOSEPA)   // Such as for(n = 0; n < 100; n++)
	{
		// ����ѭ��ͷ�Ĳ��˳�����ת��䡣���磺6. JMP scc_anal. 00401276
		for_body_address = gen_jmpforward(0);
		// ����ִ�е���������������ѭ��ͷ���ۼӲ��֡���¼�����λ�á�
		for_inc_address = sec_text_opcode_offset;
		// ����ѭ��ͷ���ۼӲ��֡�
		expression();
		operand_pop();
		// �ۼӴ��������Ժ���ת��ѭ��ͷ���˳��жϵĵط���
		// ���磺10. JMP SHORT scc_anal.0040125A
		gen_jmpbackward(for_exit_cond_address);
		// ִ�е����������������ѭ��ѭ���塣���ǿ������ѭ�������ʼ��ַ�ˡ�
		jmpaddr_backstuff(for_body_address, sec_text_opcode_offset);
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	else 						// Such as for(n = 0; n < 100;)
	{
		// ���û���ۼӲ��֣���ʲô����������
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	// Deal with the body of for_statement 
	// ֻ�д˴��õ�break,��continue,һ��ѭ���п����ж��break,����continue,����Ҫ�����Ա�����
	statement(&for_end_address, &for_inc_address); 
	// ѭ���崦�������Ժ���ת��ѭ��ͷ�ۼӵĵط���
	// ���磺18. JMP  SHORT scc_anal.0040126B
	gen_jmpbackward(for_inc_address);
	// ִ�е���������ǿ������ѭ����Ľ�����ַ�ˡ�
	jmpaddr_backstuff(for_end_address, sec_text_opcode_offset);
	// ��仰�����Ƕ���ġ���Ϊbһֱ���㡣�����������0��jmpaddr_backstuffʲô����������
	// jmpaddr_backstuff(for_inc_address, for_exit_cond_address);
}

/************************************************************************/
/*  <while_statement> ::= <KW_WHILE>                                    */
/*                             <TK_OPENPA><expression><TK_CLOSEPA>      */
/*                             <statement>                              */
/************************************************************************/
void while_statement(int *break_address, int *continue_address)
{
	int while_end_address, while_exit_cond_address;
	get_token();
	skip_token(TK_OPENPA);
	// ��������λ��while����������λ�á����ȼ�¼��whileѭ���˳�������λ�á�
	while_exit_cond_address = sec_text_opcode_offset;
	// ��ʼ��ѭ�������λ�á�
	while_end_address = 0;
	// Chapter I
	expression();
	// ����ѭ��ͷ���˳�����ת��䡣���磺5. JGE scc_anal.0040128D
	// ��ת��ַ�ڴ�����for����Ժ���
	while_end_address = gen_jcc(0);
	skip_token(TK_CLOSEPA);
	// ���ﲻ��Ҫ����ѭ��ͷ�Ĳ��˳�����ת��䡣��Ϊ�������ѭ���塣
	// while_body_address = gen_jmpforward(0);

	syntax_state = SNTX_LF_HT;
	// Deal with the body of while_statement 
	statement(&while_end_address, &while_exit_cond_address); 
	
	// ѭ���崦�������Ժ���ת��ѭ��ͷ��
	// ���磺18. JMP  SHORT scc_anal.0040126B
	gen_jmpbackward(while_exit_cond_address);
	// ѭ���崦�������Ժ���ת��ѭ��ͷ��
	// ִ�е���������ǿ������ѭ����Ľ�����ַ�ˡ�
	jmpaddr_backstuff(while_end_address, sec_text_opcode_offset);
}

/************************************************************************/
/*  <continue_statement> ::= <KW_CONTINUE><TK_SEMICOLON>                */
/************************************************************************/
/* һ������ if(i==2) continue; �����������µ�ָ�                   */
/* ������ if(i==2) ��Ӧ��ָ������:                                      */
/*  1. MOV EAX, DWORD PTR SS: [EBP-4]                                   */
/*  2. CMP EAX, 2                                                       */
/*  3. JNZ scc_anal.004012CF                                            */
/* ����һ���򵥵ıȽ���ת������                                         */
/*  1. ��i��ֵ����EAX��                                                 */
/*  2. ֮���2�Ƚϡ�                                                    */
/*  3. ��������ڣ�����ת��if�����Ľ�β����                           */
/* ���� continue; ��Ӧ��ָ������:                                       */
/*  4. JMP scc_anal.004012B3                                            */
/* ����ֱ����ת�����for�����ۼӴ���                                  */
/************************************************************************/
void continue_statement(int *continue_address)
{
	if (!continue_address)
	{
		print_error("Can not use continue");
	}
	// ������ת�����for����ۼӴ��Ļ����롣
	* continue_address = gen_jmpforward(*continue_address);
	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/************************************************************************/
/*  <break_statement> ::= <KW_CONTINUE><TK_SEMICOLON>                   */
/************************************************************************/
/* һ������ if(i==6) break; �����������µ�ָ�                      */
/* ������ if(i==6) ��Ӧ��ָ������:                                      */
/*  1. MOV EAX, DWORD PTR SS: [EBP-4]                                   */
/*  2. CMP EAX, 6                                                       */
/*  3. JNZ scc_anal.004012E0                                            */
/* ����һ���򵥵ıȽ���ת������                                         */
/*  1. ��i��ֵ����EAX��                                                 */
/*  2. ֮���6�Ƚϡ�                                                    */
/*  3. ��������ڣ�����ת��if�����Ľ�β����                           */
/* ���� break; ��Ӧ��ָ������:                                          */
/*  4. JMP scc_anal.0040130D                                            */
/* ����ֱ����ת�����for����ѭ�����β����                            */
/************************************************************************/
void break_statement(int *break_address)
{
	if (!break_address)
	{
		print_error("Can not use break");
	}
	// ������ת�����for����ѭ�����β���Ļ����롣
	* break_address = gen_jmpforward(*break_address);

	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/************************************************************************/
/*  <return_statement> ::= <KW_RETURN><TK_SEMICOLON>                    */
/*                       | <KW_RETURN><expression><TK_SEMICOLON>        */
/************************************************************************/
/* һ������ return 1; �����������µ�ָ�                            */
/*  1. MOV EAX, 1                                                       */
/*  2. JMP scc_anal.00401317                                            */
/* ����һ���򵥵ĸ�ֵ��ת������                                         */
/*  1. �ѷ���ֵ1����EAX��                                               */
/*  2. ֮����ת���������ĩβ������������gen_epilog��ɡ�               */
/************************************************************************/
void return_statement()
{
	syntax_state = SNTX_DELAY;
	get_token();
	if(get_current_token_type() == TK_SEMICOLON)   // 
	{
		syntax_state = SNTX_NUL;
	}
	else 						// 
	{
		syntax_state = SNTX_SP;
	}
	syntax_indent();
	if(get_current_token_type() != TK_SEMICOLON)   // 
	{
		expression();
		// ��ջ����������Ҳ���Ƿ���ֵ�����ص�'rc'��Ĵ����С�
		// ���磺 1. MOV EAX, 1
		load_one(REG_IRET, operand_stack_top);
		operand_pop();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
	// ������תָ��Ļ����롣
	return_symbol_pos = gen_jmpforward(return_symbol_pos);
}


/************************************************************************/
/*  <expression> ::= <assignment_expression>                            */
/*        {<TK_COMMA><assignment_expression>}                           */
/************************************************************************/
void expression()
{
	while (1)
	{
		assignment_expression();
		// Exit except this kind of code : int a = 1, b = 2; 
		if (get_current_token_type() != TK_COMMA)   
		{
			break;
		}
		operand_pop();
		get_token();
	}
}

/************************************************************************/
/*  <assignment_expression> ::= <equality_expression>                   */
/*           |<unary_expression><TK_ASSIGN><assignment_expression>      */
/*  ��������ݹ飬������ȡ������                                        */
/*  �ǵȼ۱任���ķ���                                                  */
/*  <assignment_expression> ::= <equality_expression>                   */
/*                  {<TK ASSIGN><assignment expression>}                */
/************************************************************************/
void assignment_expression()
{
	equality_expression();
	// ���ﴦ��������ֵ����������磺
	//     char a, b;
    //	   char a = b = 'A';
	if (get_current_token_type() == TK_ASSIGN)
	{
		check_leftvalue();
		get_token();
		assignment_expression();
		store_zero_to_one();
	}
}

/************************************************************************/
/*  <equality expression> ::= <relational_expression>                   */
/*                    {<TK_EQ><relational_expression>                   */
/*                    |<TK_NEQ><relational_expression>}                 */
/************************************************************************/
/* һ������ c = a == b; �����������µ�ָ�                          */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETE AL                                                          */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* һ������ c = a != b; �����������µ�ָ�                          */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETNE AL                                                         */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/************************************************************************/
void equality_expression()
{
	int token_code;
	relational_expression();
//	if (get_current_token_type() == TK_EQ 
//		|| get_current_token_type() == TK_NEQ)
	while (get_current_token_type() == TK_EQ 
		|| get_current_token_type() == TK_NEQ)
	{
		// ��������
		token_code = get_current_token_type();
		get_token();
		relational_expression();
		gen_op(token_code);
	}
}

void additive_expression();
void multiplicative_expression();
void unary_expression();

/************************************************************************/
/*  <relational_expression> ::= <additive_expression>{                  */
/*       <TK_LT><additive_expression>    // < С�ں�      TK_LT, "<",   */
/*     | <TK_GT><additive_expression>    // <= С�ڵ��ں� TK_LEQ, "<=", */
/*     | <TK_LEQ><additive_expression>   // > ���ں�      TK_GT, ">",   */
/*     | <TK_GEQ><additive_expression>}  // >= ���ڵ��ں� TK_GEQ, ">=", */
/************************************************************************/
/* һ������ c = a > b; �����������µ�ָ�                           */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SET GAL                                                          */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* һ������ c = a >= b; �����������µ�ָ�                          */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETGE AL                                                         */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* һ������ c = a < b; �����������µ�ָ�                           */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETL AL                                                          */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* һ������ c = a <= b; �����������µ�ָ�                          */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETL EAL                                                         */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/************************************************************************/
void relational_expression()
{
	int token_code;
	additive_expression();
	while(get_current_token_type() == TK_LT || get_current_token_type() == TK_LEQ 
		|| get_current_token_type() == TK_GT || get_current_token_type() == TK_GEQ)
	{
		// ��������
		token_code = get_current_token_type();
		get_token();
		additive_expression();
		gen_op(token_code);
	}
}
 
/************************************************************************/
/* <additive_expression> ::= <multiplicative_expression>                */
/*                {<TK_PLUS><multiplicative_expression>                 */
/*                 <TK_MINUS><multiplicative_expression>)               */
/************************************************************************/
/* һ������ c = a + b; �����������µ�ָ�                           */
/*  1. MOV EAX,  DWORD PTR SS��[EBP-8]                                  */
/*  2. MOV ECX,  DWORD PTR SS��[EBP-4]                                  */
/*  3. ADD ECX,  EAX                                                    */
/*  4. MOV DWORD PTR SS: [EBP-C], ECX                                   */
/* һ������ d = a + 8; �����������µ�ָ�                           */
/*  1. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  2. ADD EAX,  8                                                      */
/*  3. MOV DWORD PTR SS: [EBP-10], EAX                                  */
/* һ������ e = 6 + 8; �����������µ�ָ�                           */
/*  1. MOV EAX,  6                                                      */
/*  2. ADD EAX,  8                                                      */
/*  3. MOV DWORD PTR SS: [EBP-14], EAX                                  */
/* һ������ c = a - b; �����������µ�ָ�                           */
/*  1. MOV EAX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV ECX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. SUB ECX,  EAX                                                    */
/*  4. MOV DWORD PTR SS: [EBP-C], ECX                                   */
/* һ������ d = a - 8; �����������µ�ָ�                           */
/*  1. MOV EAX, DWORD PTR SS: [EBP-4]                                   */
/*  2. SUB EAX, 8                                                       */
/*  3. MOV DWORD PTR SS: [EBP-10],  EAX                                 */
/************************************************************************/
void additive_expression()
{
	int token_code;
	multiplicative_expression();
	while(get_current_token_type() == TK_PLUS || get_current_token_type() == TK_MINUS)
	{
		// ��������
		token_code = get_current_token_type();
		get_token();
		multiplicative_expression();
		gen_op(token_code);
	}
}

/************************************************************************/
/* <multiplicative_expression> ::= <unary_expression>                   */
/*                   {<TK_STAR><unary_expression>                       */
/*                   |<TK_DIVIDE><unary_expression>                     */
/*                   |<TK_MOD><unary_expression>}                       */
/************************************************************************/
/* һ������ c = a * b; �����������µ�ָ�                           */
/*  1. MOV  EAX,  DWORD PTR SS: [EBP-8]                                 */
/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. MOV  DWORD PTR SS: [EBP-C], ECX                                  */
/* һ������ d = a * 8; �����������µ�ָ�                           */
/*  1. MOV  EAX,  8                                                     */
/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. MOV  DWORD PTR SS: [EBP-10], ECX                                 */
/* һ������ e = 6 * 8; �����������µ�ָ�                           */
/*  1. MOV  EAX,  8                                                     */
/*  2. MOV  ECX,  6                                                     */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. MOV DWORD PTR SS: [EBP-14], ECX                                  */
/* һ������ c = a / b; �����������µ�ָ�                           */
/*  1. MOV ECX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* һ������ d = a / 8; �����������µ�ָ�                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-10], EAX                                  */
/* һ������ e = 6 / 8; �����������µ�ָ�                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  6                                                      */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-14], EAX                                  */
/* һ������ c = a % b; �����������µ�ָ�                           */
/*  1. MOV ECX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-C], EDX                                   */
/* һ������ d = a % 8; �����������µ�ָ�                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-10], EDX                                  */
/* һ������ e = 6 % 8; �����������µ�ָ�                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  6                                                      */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-14], EDX                                  */
/************************************************************************/
void multiplicative_expression()
{
	int token_code;
	unary_expression();
	while(get_current_token_type() == TK_STAR
		|| get_current_token_type() == TK_DIVIDE
		|| get_current_token_type() == TK_MOD)
	{
		// ��������
		token_code = get_current_token_type();
		get_token();
		unary_expression();
		gen_op(token_code);
	}
}

void primary_expression();
void postfix_expression();
void sizeof_expression();
void argument_expression_list();

void indirection()
{
	// ���Ѱַ������ָ�롣�����������ָ�룬�ͱ�����
	// ���磺struct  point  pt; pt->x++;
	// ��ʱ��pt�ǽṹ�壬���͵���T_STRUCT��������T_PTR���ͻᱨ����
	// �����д�ɣ�struct  point * ptPtr; pt->x++;
	// ��ʱ��pt�ǽṹ��ָ�룬���͵���T_PTR���Ͳ��ᱨ����
	if ((operand_stack_top->type.type & T_BTYPE) != T_PTR)
	{
		// �����Ǻ�����
		if ((operand_stack_top->type.type & T_BTYPE) == T_FUNC)
		{
			return;
		}
		print_error("Need pointer");
	}
	// ����Ǳ��ر�������ñ���λ��ջ�ϡ�ֻ��Ҫ��
	if (operand_stack_top->storage_class & SC_LVAL)
	{
		load_one(REG_ANY, operand_stack_top);
	}
	operand_stack_top->type = *pointed_type(&operand_stack_top->type);

	// ������������뺯������������ֵ��־����Ϊ�����뺯������Ϊ��ֵ
	if ((operand_stack_top->type.type & T_BTYPE) != T_FUNC &&
		!(operand_stack_top->type.type & T_ARRAY))
	{
		operand_stack_top->storage_class |= SC_LVAL;
	}

}

/************************************************************************/
/* <unary_expression> ::= <postfix_expression>                          */
/*                 | <TK_AND><unary_expression>                         */
/*                 | <TK_STAR><unary_expression>                        */
/*                 | <TK_PLUS><unary_expression>                        */
/*                 | <TK_MINUS><unary_expression>                       */
/*                 | <sizeof_expression>                                */
/************************************************************************/
/* �����������£�                                                       */
/*     int a, *pa, n;                                                   */
/* һ������ a = +8; �����������µ�ָ�                              */
/*  1. MOV EAX,  8                                                      */
/*  2. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* һ������ a = -8; �����������µ�ָ�                              */
/*  1. MOV EAX,  0                                                      */
/*  2. SUB EAX,  8                                                      */
/*  3. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* һ������ a = *pa; �����������µ�ָ�                             */
/*  1. MOV EAX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV ECX,  DWORD PTR DS: [EAX]                                    */
/*  3. MOV DWORD PTR SS: [EBP-4], ECX                                   */
/* һ������ pa = &a; �����������µ�ָ�                             */
/*  1. LEA EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  2. MOV DWORD PTR SS: [EBP-8], EAX                                   */
/* һ������ n = sizeof(int); �����������µ�ָ�                     */
/*  1. MOV EAX,  4                                                      */
/*  2. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/************************************************************************/
void unary_expression()
{
	switch(get_current_token_type())
	{
	case TK_AND:
		get_token();
		unary_expression();
		// ������������뺯��
		if ((operand_stack_top->type.type & T_BTYPE) != T_FUNC &&
			!(operand_stack_top->type.type & T_ARRAY))
		{
			cancel_lvalue();
		}
		mk_pointer(&operand_stack_top->type);
		break;
	case TK_STAR:
		get_token();
		unary_expression();
		indirection();
		break;
	case TK_PLUS:
	case TK_MINUS:
		get_token();
		unary_expression();
		break;
	case KW_SIZEOF:
		sizeof_expression();
		break;
	default:
		postfix_expression();
		break;
	}
}

/***********************************************************
 *  <sizeof_expression> ::= 
 *     <KW_SIZEOF><TK_OPENPA><type_specifier><TK_CLOSEPA>
 **********************************************************/
void sizeof_expression()
{
	int align, size;
	Type typeCurrent ;
	
	get_token();
	skip_token(TK_OPENPA);
	type_specifier(&typeCurrent);
	skip_token(TK_CLOSEPA);
	
	size = type_size(&typeCurrent, &align);
	if(size < 0)
		print_error("sizeof failed.");
	operand_push(&int_type, SC_GLOBAL, size);
}

/************************************************************************/
/*  <postfix_expression> ::= <primary_expression>                       */
/*    { <TK_OPENBR><expression><TK_CLOSEBR>                             */
/*    | <TK_OPENPA><TK_CLOSEPA>                                         */
/*    | <TK_OPENPA><argument_expression_list><TK_CLOSEPA>               */
/*    | <TK_DOT><IDENTIFIER>                                            */
/*    | <TK POINTS TO><IDENTIFIER>}                                     */
/************************************************************************/
/* �����������£�                                                       */
/*     struct point                                                     */
/*     {                                                                */
/*         int m_x;                                                     */
/*         int m_y;                                                     */
/*     };                                                               */
/*                                                                      */
/*     int x��                                                          */
/*     struct point pt;                                                 */
/*     int arr[10];                                                     */
/* һ������ x = pt.m_x; �����������µ�ָ�                          */
/*  1. MOV  EAX,  1                                                     */
/*  2. MOV  ECX,  0                                                     */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. LEA  EAX,  DWORD PTR SS: [EBP-C]                                 */
/*  5. ADD  EAX,  ECX                                                   */
/*  6. MOV  ECX,  DWORD PTR DS: [EAX]                                   */
/*  7. MOV  DWORD PTR SS: [EBP-4], ECX                                  */
/* ��λ������߼�Ҳ�ܼ򵥡�����m_x��struct point pt�ĵ�0��Ԫ�ء�      */
/* ����ECX = 0��EAX��ʾ���ݴ�С����ECX��EAX��˵õ�ƫ������             */
/* ֮���ȡ[EBP-C]λ�õĵ�ַ��Ҳ����ջ��pt��λ�õ�ַ��֮�����ƫ������  */
/* �����ݼ��������ƫ������ȡ���ݵ�ECX��֮��ŵ���һ��ջλ���С�      */
/************************************************************************/
/* һ������ x = arr[1]; �����������µ�ָ�                          */
/*  1. MOV  EAX,  4                                                     */
/*  2. MOV  ECX,  1                                                     */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. LEA  EAX,  DWORD PTR SS: [EBP-34]                                */
/*  5. ADD  EAX,  ECX                                                   */
/*  6. MOV  ECX,  DWORD PTR DS: [EAX]                                   */
/*  7. MOV  DWORD PTR SS: [EBP-4], ECX                                  */
/************************************************************************/
void postfix_expression()
{
	Symbol * symbol;
	primary_expression();
	while (1)
	{
		// ����ǽṹ���Ա�����"->"��"."��
		if (get_current_token_type() == TK_DOT               // | <TK_DOT><IDENTIFIER>
			|| get_current_token_type() == TK_POINTSTO)      // | <TK_POINTSTO><IDENTIFIER>
		{
			if (get_current_token_type() == TK_POINTSTO)
			{
				indirection();
			}
			cancel_lvalue();

			get_token();

			if ((operand_stack_top->type.type & T_BTYPE) != T_STRUCT)
			{
				print_error("Need struct");
			}
			symbol = operand_stack_top->type.ref;

			// token |= SC_MEMBER;
			set_current_token_type(get_current_token_type() | SC_MEMBER);
			
			while ((symbol = symbol->next) != NULL)
			{
				if (symbol->related_value == get_current_token_type())
				{
					break;
				}
			}
			if (!symbol)
			{
				print_error("Not found");
			}
            /* ��Ա������ַ = �ṹ����ָ�� + ��Ա����ƫ�� */
			/* ��Ա������ƫ����ָ����ڽṹ���׵�ַ���ֽ�ƫ�ƣ�
			 * ���Ϊ�˼����ֽ�ƫ�ƣ��˴��任����Ϊ�ֽڱ���ָ�롣 */
			operand_stack_top->type = char_pointer_type;
			operand_push(&int_type, SC_GLOBAL, symbol->related_value);
			gen_op(TK_PLUS);  // ִ�к�optop->value�����˳�Ա��ַ
            /* �任����Ϊ��Ա������������ */
			operand_stack_top->type = symbol->typeSymbol;
            /* ����������ܳ䵱��ֵ */
			if (!(operand_stack_top->type.type & T_ARRAY))
			{
				operand_stack_top->storage_class |= SC_LVAL;
			}

			get_token();
		}
		// ����������±���� - ��������"["��
		else if (get_current_token_type() == TK_OPENBR)      // <TK_OPENBR><expression><TK_CLOSEBR>
		{
			// ��ȡ�����±�ĵ�һ��token��
			get_token();
			// ���������±ꡣ
			expression();
			// ��������ƫ�Ƶ�ַ��
			gen_op(TK_PLUS);
			// 
			indirection();
			skip_token(TK_CLOSEBR);
		}
		else if (get_current_token_type() == TK_OPENPA)      // <TK_OPENPA><argument_expression_list><TK_CLOSEPA>
		{
			argument_expression_list();
		}
		else
			break;
	}
}

/************************************************************************/
/*  <primary_expression> ::= <IDENTIFIER>                               */
/*     | <TK_CINT>                                                      */
/*     | <TK_CCHAR>                                                     */
/*     | <TK_CSTR>                                                      */
/*     | <TK_OPENPA><expression><TK_CLOSEPA>                            */
/************************************************************************/
/* ȫ�ֱ����������£�                                                   */
/*    char g_char = 'a';                                                */
/*    short g_short = 123;                                              */
/*    int g_int = 123456;                                               */
/*    char g_str1[] = "g_strl";                                         */
/*    char* g_str2 = "g_str2";                                          */                                       
/* һ������ char a = 'a'; �����������µ�ָ�                        */
/*    MOV EAX,  61                                                      */
/*    MOV BYTE PTR SS: [EBP-1], AL                                      */
/* һ������ short b = 8; �����������µ�ָ�                         */
/*    MOV EAX,  8                                                       */
/*    MOV WORD PTR SS: [EBP-2], AX                                      */
/* һ������ int c = 6; �����������µ�ָ�                           */
/*    MOV EAX,  6                                                       */
/*    MOV DWORD PTR SS: [EBP-4], EAX                                    */
/* һ������ char str1[]  = "str 1"; �����������µ�ָ�              */
/*    MOV ECX,  5                                                       */
/*    MOV ESI,  scc_anal.00403015; ASCII"strl"                          */
/*    LEA EDI,  DWORD PTR SS: [EBP-9]                                   */
/*    REP MOVS  BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]                  */
/* һ������ char* str2 = "str2"; �����������µ�ָ�                 */
/*    MOV EAX,  scc_anal.0040301A; ASCII"str2"                          */
/*    MOV DWORD PTR SS: [EBP-C], EAX                                    */
/* һ������ a = g_char; �����������µ�ָ�                          */
/*    ; MOVSX - ��������չ����ָ��                                      */
/*    MOVSX EAX,  BYTE PTR DS: [402000]                                 */
/*    MOV BYTE PTR SS: [EBP-1], AL                                      */
/* һ������ b = g_short; �����������µ�ָ�                         */
/*    MOVSX EAX,  WORD PTR DS: [402002]                                 */
/*    MOV WORD PTR SS: [EBP-2], AX                                      */
/* һ������ c = g_int; �����������µ�ָ�                           */
/*    MOV  EAX,  DWORD PTR DS: [402004]                                 */
/*    MOV  DWORD PTR SS: [EBP-4], EAX                                   */
/* һ������ printf(g_str1); �����������µ�ָ�                      */
/*    MOV  EAX,  scc_anal.00402008; ASCII��g_strl"                       */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvert.printf>                                         */
/*    ADD  ESP, 4                                                       */
/* һ������ printf(printf(g_str2);); �����������µ�ָ�             */
/*    MOV  EAX, DWORD PTR: [402010]: scc_anal.0040300E                  */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&-ms vert.printf>                                       */
/*    ADD  ESP,  4                                                      */
/* һ������ printf(str1); �����������µ�ָ�                        */
/*    LEA  EAX, DWORD PTR SS: [EBP-9]                                   */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvert.printf>                                         */
/*    ADD ESP. 4                                                        */
/* һ������ printf(str2); �����������µ�ָ�                        */
/*    MOV  EAX,  DWORD PTR SS: [EBP-C]                                  */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvert.printf>                                         */
/*    ADD ESP,  4                                                       */
/************************************************************************/
void primary_expression()
{
	// �����addr��ʱû���õ���������÷������ȵ���allocate_storage����������������л�á�
	// ���������Ҫ�����ڿռ䲻����ʱ�򣬷���ڵĿռ䣬ͬʱ���ص�ǰ�����ڽ��еĴ洢λ�á�
	// ����Ľ���һ����ִ�г����еĸ���������ڣ����ݽڣ����ű��ڡ�
	int token_code , storage_class, addr = 0;
	int iTokenCode;   // Used in TK_CSTR
	Type typeExpression;
	Symbol *token_symbol ;
	Section * sec = NULL;
	
	switch(get_current_token_type()) {
	case TK_CINT:
	case TK_CCHAR:
        // operand_push(&int_type, SC_GLOBAL, tkvalue);
        operand_push(&int_type, SC_GLOBAL, atoi(get_current_token()));
		get_token();
		break;
	case TK_CSTR:
		// get_token();
		// ����������Ϊ������
		//   1.1. ����һ���ַ���ָ����š�
		iTokenCode = T_CHAR;
		typeExpression.type = iTokenCode;
		mk_pointer(&typeExpression);
		typeExpression.type |= T_ARRAY;
		// �ַ���������.rdata�ڷ���洢�ռ�
        // sec = allocate_storage(&typeExpression, 
        //	      SC_GLOBAL, SEC_RDATA_STORAGE, 0, &addr);
		// var_sym_put(&typeExpression, SC_GLOBAL, 0, addr);

		// �ַ���������Ӧ����0 - TK_PLUS����Ӧ����TK_CSTR
		// 1.2. ����allocate_storage����ռ䡣
		//      ��Ϊ��typeExpression->ref->related_value = -1
		//      ��ˣ���ʹ�õ�ǰtoken�������ݳ��ȡ�
        sec = allocate_storage(&typeExpression, 
        	SC_GLOBAL, SEC_RDATA_STORAGE, TK_CSTR, &addr);
		// 1.3. ����var_sym_put�ѱ���������ű���
		var_sym_put(&typeExpression, SC_GLOBAL, TK_CSTR, addr);
		// 1.4. Ƕ�׵���initializer��ɱ�����ʼ����
		//      ��Ϊǰ�����ռ��ʱ���Ƿ����ȫ�ֿռ䣬��SC_GLOBAL��
	    //      ͬʱ��allocate_storage�ķ���ֵsec���ǿա�
	    //      ͬʱ����ǿ��ָ����T_ARRAY���ͱ�־��
		//      ���ǻ����initializer������if���֡�
	    //      ���ַ���memcpy���ڵ�ָ��λ�á�
		initializer(&typeExpression, addr, sec);
		break;
	case TK_OPENPA:
		get_token();
		expression();
		skip_token(TK_CLOSEPA);
		break;
	default:
		char tkStr[128];
		token_code = get_current_token_type();
		strcpy(tkStr, get_current_token());
		get_token();
		// The problem of String at 07/11, We need the parse_string function
		if (token_code < TK_IDENT) 
		{
			// print_error("Need variable or constant\n");
			printf("Need variable or constant\n");
		}
		// ����token_type���ҷ��š�
		token_symbol = sym_search(token_code);
		if(!token_symbol)
		{
			if (get_current_token_type() != TK_OPENPA)
			{
				// char tmpStr[128];
				// sprintf(tmpStr, "%s does not declare\n", get_current_token());
				printf("%s does not declare\n", tkStr);
				// print_error(testStr);
			}
			token_symbol = func_sym_push(token_code, &default_func_type);
			token_symbol->storage_class = SC_GLOBAL | SC_SYM;
		}
		// ȡ�����ŵļĴ����洢����
		storage_class = token_symbol->storage_class;
		operand_push(&token_symbol->typeSymbol, 
			storage_class, token_symbol->related_value);
        /* ���ȡ�����ķ�����һ���������ã�������һ���������� */
        if (operand_stack_top->storage_class & SC_SYM) 
		{   
			// �����������¼���ŵ�ַ
			operand_stack_top->sym = token_symbol;      
			// ��Ϊ���������һ���������ã�����ϣ�����ֵ��Ч��
            operand_stack_top->operand_value = 0;  
			// ���ں������ã���ȫ�ֱ������� printf("g_cc=%c\n",g_cc);
        }
		break;
	}
}

/************************************************************************/
/* ���ܣ�����ʵ�α���ʽ��                                               */
/* <argument_expression_list> ::= <assignment_expression>               */
/*                     {<TK_COMMA><assignment_expression>}              */
/************************************************************************/
/* �����������£�                                                       */
/*    int add(int x, int y)                                             */
/*    {                                                                 */
/*        return x+y;                                                   */
/*    }                                                                 */
/* �����������µ�ָ�                                                 */
/*    1. PUSH EBP                                                       */
/*    2. MOVE BP, ESP                                                   */
/*    3. SUB ESP, 0                                                     */
/*    4. MOV EAX,  DWORD PTR SS: [EBP+C]                                */
/*    5. MOV ECX,  DWORD PTR SS: [EBP+8]                                */
/*    6. ADD ECX,  EAX                                                  */
/*    7. JMP scc_anal.00401331                                          */
/*    8. MOV ESP, EBP                                                   */
/*    9. POP EBP                                                        */
/*   10. RETN                                                           */
/* ����1-3��Ϊ������ڡ�4-6��ִ��x+y��7��ִ��return��8-10��Ϊ�������ڡ� */
/* һ������ c = add(a, b); �����������µ�ָ�                       */
/*    MOV  EAX,  DWORD PTR SS: [EBP-8]  ; ��aȡ����ѹջ��               */
/*    PUSH EAX                                                          */
/*    MOV  EAX,  DWORD PTR SS: [EBP-4]  ; ��bȡ����ѹջ��               */
/*    PUSH EAX                                                          */
/*    CALL scc_anal.0040131B            ; ���ú�����                    */
/*    ADD  ESP, 8                       ; �ָ���ջ��                    */
/*                                      ; ��Ϊ����������������д8��     */
/*    MOV DWORD PTR SS: [EBP-C], EAX    ; �ѷ���ֵȡ�����ŵ�c���档     */
/* һ������ printf("Hello"); �����������µ�ָ�                     */
/*    MOV  EAX, scc_anal.00403008; ASCII"Hello"                         */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvcrt.printf>                                         */
/*    ADD ESP,  4                                                       */
/************************************************************************/
void argument_expression_list()
{
	get_token();
	if(get_current_token_type() != TK_OPENPA) 
	{
		for(;;)
		{
			assignment_expression();
			if(get_current_token_type() == TK_CLOSEPA)
				break;
			skip_token(TK_COMMA);
		}
	}
	skip_token(TK_CLOSEPA);
}

/************************************************************************
 *	<struct_member_declaration> ::= 
 *	<type_specifier><struct_declarator_list><TK_SEMICOLON>*
 *	<struct_declarator_list> ::= <declarator>{<TK_COMMA><declarator>}
 *	�ȼ�ת�����ķ�����
 *	<struct_member_declaration> ::= 
 *	<type_specifier><declarator>{<TK_COMMA><declarator>}
 *	<TK_SEMICOLON>
 ***********************************************************************/
void struct_member_declaration(int * maxalign, int * offset, Symbol *** ps)
{
	int token_code, size, align;
	Symbol * struct_symbol;
	Type typeOne, bType ;
	int force_align;
	
	type_specifier(&bType);
	while (1)
	{
		token_code = 0 ;
		typeOne = bType;
		declarator(&typeOne, &token_code, &force_align);
		// Adding Symbol operation
		size = type_size(&typeOne, &align);
		if (force_align & ALIGN_SET)
		{
			align = force_align & ~ALIGN_SET;
		}
		*offset = calc_align(*offset, align);
		if (align > *maxalign)
		{
			*maxalign = align;
		}
		struct_symbol = sym_push(token_code | SC_MEMBER, &typeOne, 0, *offset);
		*offset += size;
		**ps = struct_symbol;
		*ps = &struct_symbol->next;
		// end of Adding Symbol operation

		if (get_current_token_type() == TK_SEMICOLON)
		{
			break;
		}
		skip_token(TK_COMMA);
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/************************************************************************/
/*  <struct_declaration_list> ::= <struct_member_declaration>           */
/*                               {<struct_member_declaration>}          */
/************************************************************************/
void struct_declaration_list(Type * type)
{
	int maxalign, offset;
	// syntax_state = SNTX_LF_HT;
	// syntax_level++;
	Symbol *sym, **symPtr;
	get_token();
	// Adding Symbol operation
	sym = type->ref;
	if (sym->related_value != -1)
	{
		print_error("Has defined");
	}
	maxalign = 1;
	symPtr = &sym->next;
	offset = 0 ;
	// end of Adding Symbol operation
	while (get_current_token_type() != TK_END)  // } �Ҵ�����
	{
		struct_member_declaration(&maxalign, &offset, &symPtr);
	}
	skip_token(TK_END);
	// syntax_state = SNTX_LF_HT;
	sym->related_value = calc_align(offset, maxalign);
	sym->storage_class = maxalign;
}

/************************************************************************/
/*  <struct_specifier> ::= <KW_STRUCT><IDENTIFIER><TK_BEGIN>            */
/*                         <struct_declaration_list><TK_END>            */
/*                       | <KW_STRUCT><IDENTIFIER>                      */
/************************************************************************/
void struct_specifier(Type * typeStruct)
{
	int token_code ;
	Symbol * sym;
	Type typeOne;
	
	get_token();		// Get struct name <IDENTIFIER>
	token_code = get_current_token_type();
	syntax_state = SNTX_DELAY;
	// DELAY disable print in the get_token
	get_token(); // Get token after the struct name

	if(get_current_token_type() == TK_BEGIN) // Indent at {
		syntax_state = SNTX_LF_HT;
	else if(get_current_token_type() == TK_CLOSEPA) // sizeof(struct_name);
		syntax_state = SNTX_NUL;
	else
		syntax_state = SNTX_SP;

	syntax_indent();
	if (token_code < TK_IDENT)  // Key word is illegal
	{
		print_error("Need struct name\n");
	}
	// Adding Symbol operation
	sym = struct_search(token_code);
	if (!sym)
	{
		typeOne.type = T_STRUCT;
		sym = sym_push(token_code | SC_STRUCT, &typeOne, 0, -1);
		sym->storage_class = 0;
	}
	typeStruct->type = T_STRUCT;
	typeStruct->ref  = sym;
	// end of Adding Symbol operation
	if (get_current_token_type() == TK_BEGIN)
	{
		struct_declaration_list(typeStruct);
	}
}

/************************************************************************/
/* <type_specifier> ::= <KW_CHAR> | <KW_SHORT>                          */
/*                    | <KW_VOID> | <KW_INT> | <struct_specifier>       */
/************************************************************************/
int type_specifier(Type * typeSpec)
{
	e_TypeCode typeCode;
	int type_found = 0 ;
	Type typeStruct;
	typeCode = T_INT ;
	
	switch(get_current_token_type()) {
	case KW_CHAR:    // All of simple_type
		typeCode = T_CHAR;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_SHORT:
		typeCode = T_SHORT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_VOID:
		typeCode = T_VOID;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_INT:
		typeCode = T_INT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_STRUCT:
		typeCode = T_STRUCT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		struct_specifier(&typeStruct);
		typeSpec->ref = typeStruct.ref;
		break;
	default:
		break;
	}
	typeSpec->type = typeCode;
	return type_found ;
}

/************************************************************************/
/*  ���ܣ������ⲿ����                                                  */
/*  iSaveType e_StorageClass  �洢����                                  */
/************************************************************************/
// �����漰�����ϵͳ�бȽϾ����һ�����֡�
// ����������Ϊ�ṹ��������ȫ�ֱ��������ͺ����������Ͷ��壩�����ⲿ������
// ���������ǾͰѰ���������������ȫ�ֱ��������Ͷ���ṹ�������Ĵ��뿴���Ƕ���ⲿ������
// ֮���﷨�����ͱ�ɽ���һ��һ�����ⲿ������
// ͬʱΪ�˼���ƣ��ṹ�����������뼯�з����ļ���ͷ��
// ����������ɸ��������������������ȫ�ֱ���������
// ������external_declaration�����þ��Ǵ���һ���ⲿ������һ�������ɹ��ͷ��ء�
void external_declaration(e_StorageClass iSaveType)
{
	Type bTypeCurrent, typeCurrent ;
	// v:	���ű�š�����Ϊe_TokenCode�������Ϊtoken_code��
	// int v = -1; // , has_init, addr;
	int token_code = -1; 
	
	// �����addr��ʱû���õ���������÷������ȵ���allocate_storage����������������л�á�
	// ���������Ҫ�����ڿռ䲻����ʱ�򣬷���ڵĿռ䣬ͬʱ���ص�ǰ�����ڽ��еĴ洢λ�á�
	// ����Ľ���һ����ִ�г����еĸ���������ڣ����ݽڣ����ű��ڡ�
	// int has_init, r, addr = 0;
	e_Sec_Storage sec_area;
	int storage_class, addr = 0;
	Symbol * sym;
	Section * sec = NULL;
	print_all_stack("Starting external_declaration");
	// 1. ��ȡ���������͡�����������ǵ�һ��token��
	//    ������Ϳ�������Ϊ�������ͣ���Ϊ����������*�ͻ���ָ�����͡�
	if (!type_specifier(&bTypeCurrent))
	{
		print_error("Need type token\n");
	}
	// 2. ���ǰ��type_specifier�д����ṹ�壬��ʱbTypeCurrent.t�ͻ����T_STRUCT��
	//    ��˵����һ�Σ����Ǵ�������һ���ⲿ���������Ǿͷ��ء�
	if (bTypeCurrent.type == T_STRUCT && get_current_token_type() == TK_SEMICOLON)
	{
		print_all_stack("End external_declaration");
		get_token();
		return;
	}
	// 2. ����ִ�е����˵�������Ѿ�������������еĽṹ��������
	//    ���洦��������еĺ������������������ȫ�ֱ���������
	while (1)
	{
		typeCurrent = bTypeCurrent;
		// 3. ��������ǰ׺���������*��typeCurrent�ͻ���ָ�����͡�
		declarator(&typeCurrent, &token_code, NULL);
		// 4. ����Ǻ�������
		if (get_current_token_type() == TK_BEGIN)
		{
			// ���������Ǳ��صġ�ֻ����ȫ�ֵġ�
			if (iSaveType == SC_LOCAL)
			{
				print_error("Not nesting function\n");
			}
			// ��������������š������Ǻ���������ͱ�����
			if((typeCurrent.type & T_BTYPE) != T_FUNC)
			{
				print_error("Needing function defination\n");
			}
			// 5. ������ҵ���������š�˵��ǰ������˺������塣
			sym = sym_search(token_code);
			if(sym)
			{
				// �������ǰ�涨��Ĳ��Ǻ������塣����һ���������壬�ͱ����˳���
				if((sym->typeSymbol.type & T_BTYPE) != T_FUNC)
				{
					char tmpStr[128];
					sprintf(tmpStr, "Function %s redefination\n", get_current_token());
					print_error(tmpStr);
				}
				sym->typeSymbol = typeCurrent;
			}
			// 6. ���û���ҵ�����ֱ�����ӡ�
			else
			{
				sym = func_sym_push(token_code, &typeCurrent);
			}
			// ȫ����ͨ���š�
			sym->storage_class = SC_SYM | SC_GLOBAL;
			print_all_stack("Enter funtion body");
			// 7. ���������塣
			func_body(sym);
			break;
		}
		else
		{
			// 4. ��������
			if((typeCurrent.type & T_BTYPE) == T_FUNC)
			{
				if (sym_search(token_code) == NULL)
				{
					sym = sym_push(token_code, &typeCurrent, SC_GLOBAL | SC_SYM, 0);
				}
			}
			// 4. ��������
			else
			{
			    storage_class = 0;
				// ��������顣��ֻ������ֵ��
				if(!(typeCurrent.type & T_ARRAY))
				{
					storage_class |= SC_LVAL;
				}
				storage_class |= iSaveType;
				// �������������ʱ��ͬʱ���˸���ֵ��ִ�и���ֵ������
				// if (get_current_token_type() == TK_ASSIGN)  // int a = 5 ;
				if (get_current_token_type() == TK_ASSIGN)
				{
					sec_area = SEC_DATA_STORAGE;
				}
				else 
				{
					sec_area = SEC_BSS_STORAGE;
				}
				// ��������if(has_init)��ʵ���Եȼ��ڣ�
				//     if (get_current_token_type() == TK_ASSIGN)
				// ��������ж��õ�̫���ˡ����������������
				// ͬʱ������ж��ǲ���get_tokenӰ��ġ�
				// 5. ����ǵȺš�˵���Ǹ�ֵ��������Ҫ�ٶ�һ�����ţ�����ֵҲ��������
				if(sec_area == SEC_DATA_STORAGE)
				{
					// ȡ�õȺź������ֵ��
					get_token(); //���ܷŵ����棬
				//	initializer(&typeCurrent);
				}
				// ������Ҫ����3�����
				//   (1) Ϊ��������洢�ռ䣬�ֲ����������ջ�У�
				//       ȫ�ֱ��������࣬����ʱ���и�ֵ�Ĵ����.data���ݽ��У�
				//       ����ʱ�����и�ֵ�Ĵ����.bss���С�
				// ����ִ�е������ʱ��������ڸ�ֵ�������Ѿ�ȡ���˵Ⱥź������ֵ��
				// ֮����Ҫ������������Ϊ������Ҫ����char str[]="abc"�������
				// ͨ��allocate_storage���ַ������ȡ�

				// 6. ����Ϊ���ŷ���ռ䣬
			    sec = allocate_storage(&typeCurrent, storage_class,
			    							sec_area, token_code, &addr);
				// 7. ֮�����ӷ��š�
				sym = var_sym_put(&typeCurrent, storage_class, token_code, addr);
				//   (2) ��ȫ�ֱ�������COFF���ű���
			    if (iSaveType == SC_GLOBAL)
			    {
				    coffsym_add_update(sym, addr, sec->index, 0, IMAGE_SYM_CLASS_EXTERNAL);
			    }
				//   (3) ������ʱ���и�ֵ�ı������г�ʼ����
				//       �����һ��token�ǵȺţ�ʹ�õ�ǰtoken���и�ֵ��
			    if (sec_area == SEC_DATA_STORAGE)  // int a = 5 ;
			    {
					// 8. ��ֵ������
				    initializer(&typeCurrent, addr, sec);
			    }
			}
			
			if(get_current_token_type() ==TK_COMMA)  // int a, b ;
			{
				get_token();
			}
			else									// int a;
			{
				syntax_state = SNTX_LF_HT;
				skip_token(TK_SEMICOLON);
				break;
			}
		}
	}
}

/************************************************************************/
/* ���ܣ�����洢�ռ�                                                   */
/* typeCurrent��  ��������                                              */
/* storage_class�������洢����                                          */
/* sec_area��     �����Ƿ����ַ����������Ƿ���Ҫ���г�ʼ����            */
/*                ��������һ���ڷ���洢�ռ䡣                          */
/* token_code��   �������ű�š�����Ϊe_TokenCode��                     */
/*                ��ΪС��TK_IDENT�Ǳ���ֵ��                            */
/*                ����ϣ���ֵͨ������TK_IDENT��                        */
/*         ע�⣺ ���ڳ����ַ�����ʱ�����Ǵ�����TK_CSTR��             */
/*                ��Ȼ����ֻ��Ҫ����һ��С��TK_IDENT�ı���ֵ��          */
/*                ����֮ǰ�Ĵ��봫�����㡣                              */
/*                ����Ϊ�˱������⣬�����޸�Ϊ����TK_CSTR��             */
/*                ���ֵ��var_sym_put��ʱ�򣬻���������sym_search��
                               */
/* addr(���) ��  �����洢ƫ������ַ��                                  */
/*                ����ȫ�ֱ������ַ����������ؽ���ƫ������              */
/*                ���ھֲ���������ջ��ƫ������                          */
/* ����ֵ��       �����洢�ڡ�                                          */
/*                ����ȫ�ֱ������ַ����������ض�Ӧ�ڵĵ�ַ������ƫ������*/
/*                ���ھֲ���������NULL��                                */
/************************************************************************/
/* �����߼����£�                                                       */
/*   �������ǵ���type_size������Ҫ����Ĵ洢�ռ䡣                      */
/*   ������һ�����ɡ����ǣ�                                             */
/*       �����char * str1�����ĳ����ǲ��̶��ġ���������·���-1��      */
/*       �ܹ�����-1��ԭ����typeCurrent->related_value = -1��            */
/*   ���type_size����-1.�ͻ�ʹ��ǰ���������ֵ�������ݳ��ȡ�           */
/*   ֮�����ǾͿ�������ǰ���������ĳ��ȷ���ռ��ˡ�                   */
/*   �����ȫ�ֱ������ͷ����ڽ��ϡ�����Ǿֲ��������ͷ�����ջ�ϡ�       */
/************************************************************************/
Section * allocate_storage(Type * typeCurrent, int storage_class, 
						   e_Sec_Storage sec_area, int token_code, int *addr)
{
	int size, align;
	Section * sec= NULL;
	// ����type_size������Ҫ����Ĵ洢�ռ䣬
	size = type_size(typeCurrent, &align);
	// type_size����-1˵���ǲ�����ָ�����顣
	if (size < 0)
	{
		// ʹ��ǰ���������ֵ�������ݳ��ȡ�
		if (typeCurrent->type & T_ARRAY 
			&& typeCurrent->ref->typeSymbol.type == T_CHAR)
		{
			typeCurrent->ref->related_value = strlen((char *)get_current_token()) + 1;
			size = type_size(typeCurrent, &align);
		}
		else
			print_error("Unknown size of type");
	}

	// �ֲ�������ջ�з���洢�ռ�
	if ((storage_class & SC_VALMASK) == SC_LOCAL)
	{
		function_stack_loc = calc_align(function_stack_loc - size, align);
		*addr = function_stack_loc;
	}
	else
	{
		// ��ʼ����ȫ�ֱ�����.data�ڷ���洢�ռ�
		// ͨ���ж�(get_current_token_type() == TK_ASSIGN)�õ���
		if (sec_area == SEC_DATA_STORAGE)
		{
			sec = sec_data;
		}
		// �ַ���������.rdata�ڷ���洢�ռ�
		// ����λ��primary_expression��TK_CSTR��֧��
		else if (sec_area == SEC_RDATA_STORAGE)	
		{
			sec = sec_rdata;
		}
		// δ��ʼ����ȫ�ֱ�����.bss�ڷ���洢�ռ�
		// ͨ���ж�(get_current_token_type() == TK_ASSIGN)�õ���
		// (has_init == SEC_BSS_STORAGE)
		else if (sec_area == SEC_BSS_STORAGE)	
		{
			sec = sec_bss;
		}
		else
		{
			print_error("Illegal section defination");
		}
		sec->data_offset = calc_align(sec->data_offset, align);
		*addr = sec->data_offset;
		sec->data_offset += size;
		
		// Ϊ��Ҫ��ʼ���������ڽ��з���洢�ռ�
		if (sec->sh.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA
			&& sec->data_offset > sec->data_allocated)
		{
			section_realloc(sec, sec->data_offset);
		}
		// ��.rdata�ڷ���洢�ռ�ĳ����ַ�����
		// if (token_code == 0)
		if (token_code == TK_CSTR)
		{
			operand_push(typeCurrent, SC_GLOBAL | SC_SYM, *addr);
		//	operand_stack_top->sym = sym_sec_rdata;
		}
	}
	return sec;
}

/************************************************************************/
/* <translation_unit> ::= { <external_declaration> } | <TK_EOF>         */
/************************************************************************/
void translation_unit()
{
	while (get_current_token_type() != TK_EOF)
	{
		external_declaration(SC_GLOBAL);
	}
}
