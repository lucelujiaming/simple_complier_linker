// translation_unit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include "get_token.h"
#include "x86_generator.h"

extern int loc;
e_SynTaxState syntax_state = SNTX_NUL;
int syntax_level;

extern Section *sec_text,	// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_idata,			// 导入表节
		*sec_rdata,			// 只读数据节
		*sec_rel,			// 重定位信息节
		*sec_symtab,		// 符号表节	
		*sec_dynsymtab;		// 链接库符号节

extern int rsym;						// 记录return指令位置
extern int ind ;						// 指令在代码节位置
extern int loc ;						// 局部变量在栈中位置
extern int func_begin_ind;				// 函数开始指令
extern int func_ret_sub;				// 函数返回释放栈空间大小
extern Symbol *sym_sec_rdata;			// 只读节符号

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;
extern std::vector<Operand>::iterator operand_stack_second;

extern std::vector<Symbol> global_sym_stack;  //全局符号栈
extern std::vector<Symbol> local_sym_stack;   //局部符号栈
extern std::vector<TkWord> tktable;

extern Type char_pointer_type,		// 字符串指针
	 int_type,				// int类型
	 default_func_type;		// 缺省函数类型

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;
extern std::vector<Operand>::iterator operand_stack_second;

int type_specifier(Type * type);
void declarator(Type * type, int * v, int * force_align);
void direct_declarator(Type * type, int * v, int func_call);
void direct_declarator_postfix(Type * type, int func_call);
void parameter_type_list(Type * type, int func_call); // (int func_call);

void compound_statement(int * bsym, int * csym);
void if_statement();
void break_statement();
void return_statement();
void continue_statement();
void for_statement();
void expression_statement();

void expression();
void assignment_expression();
void equality_expression();
void relational_expression();

void external_declaration(e_StorageClass iSaveType);

int calc_align(int n , int align);

Section * allocate_storage(Type * type, int r, int has_init, int v, int *addr);
void init_variable(Type * type, Section * sec, int c, int v);

void print_error(char * strErrInfo)
{
	printf("<ERROR> %s", strErrInfo);
	exit(1);
}

void print_tab(int nTimes)
{
	int n = 0;
	for (; n < nTimes; n++)
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
/*  "__cdecl",       KW_CDECL,		// __cdecl关键字 standard c call      */
/*	"__stdcall",     KW_STDCALL,    // __stdcall关键字 pascal c call      */
void function_calling_convention(int *fc)
{
	*fc = KW_CDECL;  // Set default value with __cdecl
	// 如果函数声明指定了调用方式，就进行处理。否则就不用处理。当前token不变。
	if(get_current_token_type() == KW_CDECL || get_current_token_type() == KW_STDCALL)
	{
		*fc = get_current_token_type();
		syntax_state = SNTX_SP;
		get_token();
	}
}

/* <struct_member_alignment> ::= <KW_ALIGN><TK_OPENPA><TK_INT><TK_CLOSEPA> */
/* Such as __align(4)                                                        */ 
/*   "__align",       KW_ALIGN,		// __align关键字	                   */
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

/*********************************************************** 
 * 功能:	生成指针类型
 * t:		原数据类型
 **********************************************************/
void mk_pointer(Type *t)
{
	Symbol *s;
    s = sym_push(SC_ANOM, t, 0, -1);
    t->type = T_PTR ;
    t->ref = s;
}

/* <declarator> ::= {<TK_STAR>}[<function_calling_convention>]               */
/*                    [<struct_member_alignment>]<direct_declarator>         */
/* Such as * __cdecl __align(4) function(int a, int b)                       */ 
void declarator(Type * type, int * v, int * force_align)
{
	int fc ;
	while(get_current_token_type() == TK_STAR)		// * 星号
	{
		mk_pointer(type);
		get_token();
	}
	// 这两步其实在SC程序代码中是选做。
	// 也就是一个变量声明和函数声明一般来说，极有可能是不包含函数调用约定和对齐的。
	// 当然编译程序必须处理。
	function_calling_convention(&fc);
	if(force_align)
	{
		struct_member_alignment(force_align);  // &fc
	}
	direct_declarator(type, v, fc);
}

/* <direct_declarator> ::= <IDENTIFIER><direct_declarator_postfix>              */
void direct_declarator(Type * type, int * v, int func_call)   // Not support argv[] usage
{
	if(get_current_token_type() >= TK_IDENT)  // 函数名或者是变量名，不可以是保留关键字。 
	{
		*v = get_current_token_type();
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
/*                         | <TK_OPENPA><parameter_type_list><TK_CLOSEPA>， (3)  */
/*                         | <TK_OPENPA><TK_CLOSEPA>}                       (4)  */
/* Such as var, var[5], var(), var(int a, int b), var[]                          */ 
void direct_declarator_postfix(Type * type, int func_call)
{
	int n;
	Symbol * s;
	if(get_current_token_type() == TK_OPENPA)         // <TK_OPENPA><parameter_type_list><TK_CLOSEPA> | <TK_OPENPA><TK_CLOSEPA>
	{
		parameter_type_list(type, func_call);
	}
	else if(get_current_token_type() == TK_OPENBR)   // <TK_OPENBR><TK_CINT><TK_CLOSEBR> | <TK_OPENBR><TK_CLOSEBR>
	{
		get_token();
		n = -1;
		if(get_current_token_type() == TK_CINT)
		{
			n = atoi(get_current_token());  // as a TK_CINT
			get_token();
			// n = tkvalue;  // ??
		}
		else
		{
			print_error("Need intergat\n");
		}
		skip_token(TK_CLOSEBR);
		direct_declarator_postfix(type, func_call);    // Nesting calling
		s = sym_push(SC_ANOM, type, 0, n);
		type->type = T_ARRAY | T_PTR;
		type->ref = s;
	}
}

/*  功能：解析形参类型表                                                  */
/*  func_call：函数调用约定                                               */
/*                                                                        */
/*  <parameter_type_list> ::= <parameter_list>                            */
/*             | <parameter_list><TK_COMMA><TK_ELLIPSIS>                  */
/*                 TK_ELLIPSIS is "...",   ... 省略号                     */
/*  <parameter_list> ::= <parameter_declaration>                          */
/*                      {<TK_COMMA><parameter_declaration>}               */
/*      Such as func(int a), func(int a, int b)                           */
/*  <parameter_declaration> ::= <type_specifier>{<declarator>}            */
/*      Such as  int a                                                    */
/*  等价转换后文法：                                                      */
/*  <parameter_type_list>::=<type_specifier>{<declarator>}                */
/*   {<TK_COMMA><type_specifier>{<declarator>} } <TK_COMMA><TK_ELLIPSIS>  */
void parameter_type_list(Type * type, int func_call) // (int func_call)
{
	int n;
	Symbol **plast, *s, *first;
	Type typeCurrent ;
	get_token();
	first = NULL;
	plast = &first;
	
	while(get_current_token_type() != TK_CLOSEPA)   // get_token until meet ) 右圆括号
	{
		if(!type_specifier(&typeCurrent))
		{
			print_error("Invalid type_specifier\n");
		}
		declarator(&typeCurrent, &n, NULL);			// Translate one parameter declaration
		s = sym_push(n|SC_PARAMS, &typeCurrent, 0, 0);
		*plast = s;
		plast  = &s->next;
		
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
	
	s = sym_push(SC_ANOM, type, func_call, 0);
	s->next = first;
	type->type = T_FUNC;
	type->ref = s;
}

// ------------------ func_body 
/************************************************
 *  <func_body> ::= <compound_statement>
 *  因为只要见到<{ 左大括号>就是复合语句，所以<复合语句>的概念比<函数体>概念要大，
 *  <函数体>属于一类比较特殊的<复合语句>。
 ***********************/
void func_body(Symbol * sym)
{
	// 1. 增加或更新COFF符号
	ind = sec_text->data_offset;
	coffsym_add_update(sym, ind, sec_text->index, CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
	
	/* 2. 放一个匿名符号到局部符号表中 */
	sym_direct_push(local_sym_stack, SC_ANOM, &int_type, 0);

	// 3. 生成函数开头代码。
	gen_prolog(&(sym->typeSymbol));
	rsym = 0;

	// 4. 调用复合语句处理函数处理函数体中的语句。
	// print_all_stack("sym_push local_sym_stack");
	compound_statement(NULL, NULL);
	// print_all_stack("Left local_sym_stack");

	// 5. 函数返回后，回填返回地址。
	backpatch(rsym, ind);
	
	// 6. 生成函数结尾代码。
	gen_epilog();
	sec_text->data_offset = ind;

	/* 7. 清空局部符号表栈 */
	if(local_sym_stack.size() > 0)
	{
		sym_pop(&local_sym_stack, NULL);
	}
	// print_all_stack("Clean local_sym_stack");
}

void print_all_stack(char* strPrompt)
{
	int i;
	return;
	printf("\n-------------------%s---------------------------\n", strPrompt);
	for(i = 0; i < global_sym_stack.size(); ++i)
	{
		printf("\t global_sym_stack[%d].v = %08X c = %d type = %d\n", i, 
			global_sym_stack[i].token_code, 
			global_sym_stack[i].related_value, 
			global_sym_stack[i].typeSymbol);
	}
	for(i = 0; i < local_sym_stack.size(); ++i)
	{
		printf("\t  local_sym_stack[%d].v = %08X c = %d type = %d\n", i, 
			local_sym_stack[i].token_code, 
			local_sym_stack[i].related_value, 
			local_sym_stack[i].typeSymbol);
	}
	printf("\t tktable.size = %d \n --- ", tktable.size());
	for (i = 40; i < tktable.size(); i++)
	{
		printf(" %s ", tktable[i].spelling);
	}
	printf(" --- \n----------------------------------------------\n");
}

// 初值符的代码如下所示。
/***********************************************************
* <initializer> ::= <assignment_expression>
*********************************************/
void initializer(Type * typeToken, int c, Section * sec)
{
	if((typeToken->type & T_ARRAY) && sec)
	{
		memcpy(sec->data + c, get_current_token(), strlen(get_current_token()));
		get_token();
	}
	else
	{
		assignment_expression();
		init_variable(typeToken, sec, c, 0);
	}
}

/************************************************************************/
/* 功能：变量初始化                                                     */
/* type：变量类型                                                       */
/* sec：变量所在节                                                      */
/* c：变量相关值                                                        */
/* v：变量符号编号                                                      */
/************************************************************************/
void init_variable(Type * type, Section * sec, int c, int v)
{
	// 2022/11/14
	int bt;
	void * ptr;
	if (sec)
	{
		if(operand_stack_top)
		{
			if ((operand_stack_top->r &(SC_VALMASK | SC_LVAL)) != SC_GLOBAL)
			{
				print_error("Use constant to initialize the global variable");
			}
		}
		bt = type->type & T_BTYPE;
		ptr = sec->data + c;
		
		switch(bt) {
		case T_CHAR:
			*(char *)ptr = operand_stack_top->value;
			break;
		case T_SHORT:
			*(short *)ptr = operand_stack_top->value;
			break;
		default:
			if(operand_stack_top)
			{
				if(operand_stack_top->r & SC_SYM)
				{
					coffreloc_add(sec, operand_stack_top->sym,c, IMAGE_REL_I386_DIR32);
				}
			}
			*(int *)ptr = operand_stack_top->value;
			break;
		}
		operand_pop();
	}
	else
	{
		if (type->type & T_ARRAY)
		{
			operand_push(type, SC_LOCAL | SC_LVAL, c);
			operand_swap();
			spill_reg(REG_ECX);

			gen_opcodeOne(0xB8 + REG_ECX);
			gen_dword(operand_stack_top->type.ref->related_value);
			gen_opcodeOne(0xB8 + REG_ESI);
			gen_addr32(operand_stack_top->r, operand_stack_top->sym, operand_stack_top->value);
			operand_swap();
			
			gen_opcodeOne(0x8D);
			gen_modrm(ADDR_OTHER, REG_EDI, SC_LOCAL, operand_stack_top->sym, operand_stack_top->value);

			gen_prefix((char)0xF3);
			gen_opcodeOne((char)0xA4);
			operand_stack_top -= 2;
		}
		else
		{
			operand_push(type, SC_LOCAL | SC_LVAL, c);
			operand_swap();
			store_one();
			operand_pop();
		}
	}
}

// ------------------ 语句：statement
/***********************************************************
 * 功能:	解析语句
 * bsym:	break跳转位置。暂时没有用。
 * csym:	continue跳转位置。暂时没有用。
 *
 * <statement>::=<compound_statement>    
 *             | <if_statement>          
 *             | <return_statement>      
 *             | <break_statement>       
 *             | <continue_statement>    
 *             | <for_statement>         
 *             | <expression_statement>  
 **********************************************************/
void statement()
{
	switch(get_current_token_type())
	{
	case TK_BEGIN:
		compound_statement(NULL, NULL);			// 复合语句
		break;
	case KW_IF:
		if_statement();
		break;
	case KW_RETURN:
		return_statement();
		break;
	case KW_BREAK:
		break_statement();
		break;
	case KW_CONTINUE:
		continue_statement();
		break;
	case KW_FOR:
		for_statement();
		break;
	default:
		expression_statement();
		break;
	}
}

/***********************************************************
 * <compound_statement> ::= <TK_BEGIN>{<declaration>} {<statement>} <TK_END>
 **********************************************************/
int is_type_specifier(int token_type)
{
	switch(token_type)
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

void compound_statement(int * bsym, int * csym)
{
	Symbol *s;
	// s = &(local_sym_stack[0]);
	s = local_sym_stack.end() - 1;
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
		statement();
	}
	syntax_state = SNTX_LF_HT;
	printf("\t local_sym_stack.size = %d \n", local_sym_stack.size());
	sym_pop(&local_sym_stack, s);
	get_token();
}

/***********************************************************
 * <expression_statement> ::= <TK_SEMICOLON>|<expression><TK_SEMICOLON>
 *   TK_SEMICOLON 是 ; 分号
 **********************************************************/
void expression_statement()
{
	if(get_current_token_type() != TK_SEMICOLON)
	{
		expression();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);

}

/***********************************************************
 * <if_statement> ::= <KW_IF><TK_OPENPA><expression>
 *          <TK_CLOSEPA><statement>[<KW_ELSE><statement>]
 **********************************************************/
void if_statement()
{
	syntax_state = SNTX_SP;

	get_token();
	skip_token(TK_OPENPA);
	expression();

	syntax_state = SNTX_LF_HT;
	skip_token(TK_CLOSEPA);
	statement();

	if(get_current_token_type() == KW_ELSE)
	{
		syntax_state = SNTX_LF_HT;
		get_token();
		statement();
	}
}	

/***********************************************************
 *  <for_statement> ::= <KW_FOR><TK_OPENPA><expression_statement>
 *        <expression_statement><expression><TK_CLOSEPA><statement>
 **********************************************************/
void for_statement()
{
	get_token();
	skip_token(TK_OPENPA);

	// Chapter I
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		skip_token(TK_SEMICOLON);
	}
	else 						// Such as for(; n < 100; n++)
		skip_token(TK_SEMICOLON);

	// Chapter II
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		skip_token(TK_SEMICOLON);
	}
	else 						// Such as for(n = 0; ; n++)
		skip_token(TK_SEMICOLON);

	// Chapter III
	if(get_current_token_type() != TK_CLOSEPA)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	else 						// Such as for(n = 0; n < 100;)
	{
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	// Deal with the body of for_statement 
	statement(); 
}

/***********************************************************
 *  <while_statement> ::= <KW_WHILE><TK_OPENPA><expression><TK_CLOSEPA><statement>
 **********************************************************/
void while_statement()
{
	get_token();
	skip_token(TK_OPENPA);
	// Chapter I
	expression();
	skip_token(TK_CLOSEPA);
	syntax_state = SNTX_LF_HT;
	// Deal with the body of while_statement 
	statement(); 
}

/***********************************************************
 *  <continue_statement> ::= <KW_CONTINUE><TK_SEMICOLON>
 **********************************************************/
void continue_statement()
{
	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/***********************************************************
 *  <break_statement> ::= <KW_CONTINUE><TK_SEMICOLON>
 **********************************************************/
void break_statement()
{
	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/***********************************************************
 *  <return_statement> ::= <KW_RETURN><TK_SEMICOLON>
 *                       | <KW_RETURN><expression><TK_SEMICOLON>
 **********************************************************/
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
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}


/***********************************************************
 *  <expression> ::= <assignment_expression>
 *        {<TK_COMMA><assignment_expression>}
 **********************************************************/
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
		get_token();
	}
}

/***********************************************************
 *  <assignment_expression> ::= <equality_expression>
 *           |<unary_expression><TK_ASSIGN><assignment_expression>：
 *  非等价变换后文法：
 *  <assignment_expression> ::= <equality_expression>
 *                  {<TK ASSIGN><assignment expression>} 
 **********************************************************/
void assignment_expression()
{
	equality_expression();
	if (get_current_token_type() == TK_ASSIGN)
	{
		get_token();
		assignment_expression();
	}
}

/***********************************************************
 *  <equality expression> ::= <relational_expression>
 *                    {<TK_EQ><relational_expression>
 *                    |<TK_NEQ><relational_expression>}
 **********************************************************/
void equality_expression()
{
	relational_expression();
//	if (get_current_token_type() == TK_EQ 
//		|| get_current_token_type() == TK_NEQ)
	while (get_current_token_type() == TK_EQ 
		|| get_current_token_type() == TK_NEQ)
	{
		get_token();
		relational_expression();
	}
}

void additive_expression();
void multiplicative_expression();
void unary_expression();
/***********************************************************
 *  <relational_expression> ::= <additive_expression>{
 *       <TK_LT><additive_expression>    // < 小于号      TK_LT, "<", 
 *     | <TK_GT><additive_expression>    // <= 小于等于号 TK_LEQ, "<=",
 *     | <TK_LEQ><additive_expression>   // > 大于号      TK_GT, ">", 
 *     | <TK_GEQ><additive_expression>}  // >= 大于等于号 TK_GEQ, ">=",
 **********************************************************/
void relational_expression()
{
	additive_expression();
	while(get_current_token_type() == TK_LT || get_current_token_type() == TK_LEQ 
		|| get_current_token_type() == TK_GT || get_current_token_type() == TK_GEQ)
	{
		get_token();
		additive_expression();
	}
}
 
/***********************************************************
 * <additive_expression> ::= <multiplicative_expression>
 *                {<TK_PLUS><multiplicative_expression>
 *                 <TK_MINUS><multiplicative_expression>)
 **********************************************************/
void additive_expression()
{
	multiplicative_expression();
	while(get_current_token_type() == TK_PLUS || get_current_token_type() == TK_MINUS)
	{
		get_token();
		multiplicative_expression();
	}
}

/***********************************************************
* <multiplicative_expression> ::= <unary_expression>
*                   {<TK_STAR><unary_expression>
*                   |<TK_DIVIDE><unary_expression>
*                   |<TK_MOD><unary_expression>}
**********************************************************/
void multiplicative_expression()
{
	unary_expression();
	while(get_current_token_type() == TK_STAR || get_current_token_type() == TK_DIVIDE || get_current_token_type() == TK_MOD)
	{
		get_token();
		unary_expression();
	}
}

void primary_expression();
void postfix_expression();
void sizeof_expression();
void argument_expression_list();
/***********************************************************
* <unary_expression> ::= <postfix_expression>
*                 | <TK_AND><unary_expression>
*                 | <TK_STAR><unary_expression>
*                 | <TK_PLUS><unary_expression>
*                 | <TK_MINUS><unary_expression>
*                 | <sizeof_expression>
**********************************************************/
void unary_expression()
{
	switch(get_current_token_type())
	{
	case TK_AND:
	case TK_STAR:
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

int type_size(Type * type, int * align);
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
}

int type_size(Type * typeCal, int * align)
{
	Symbol *s;
	int bt;
	int PTR_SIZE = 4;
	bt = typeCal->type & T_BTYPE;
	switch(bt)
	{
	case T_STRUCT:
		s = typeCal->ref;
		*align = s->storage_class;
		return s->related_value ;
	case T_PTR:
		// 如果是指针数组。这里分两种情况。
		// 1. 一个是数组变量，类似于int AA[717]。
		//    这种情况下，长度等于typeSymbol的大小乘以related_value，
		//    例如int AA[717]的长度就是4 * 717 = 2868。
		// 2. 一个是数组常量，类似于"Hello world!"这种字符串。
        //    这种情况下，related_value等于-1。直接返回负数。由上层处理。
		//    因为这种情况下，长度可以通过token直接计算。
		if(typeCal->type & T_ARRAY)
		{
			s = typeCal->ref;
			return type_size(&s->typeSymbol, align) * s->related_value;
		}
		// 否则指针的长度为32位，也就是4。
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
 *  <postfix_expression> ::= <primary_expression>
 *    { <TK_OPENBR><expression><TK_CLOSEBR>
 *    | <TK_OPENPA><TK_CLOSEPA>
 *    | <TK_OPENPA><argument_expression_list><TK_CLOSEPA>
 *    | <TK_DOT><IDENTIFIER>
 *    | <TK POINTS TO><IDENTIFIER>}
 **********************************************************/
void postfix_expression()
{
	primary_expression();
	while (1)
	{
		if (get_current_token_type() == TK_DOT               // | <TK_DOT><IDENTIFIER>
			|| get_current_token_type() == TK_POINTSTO)      // | <TK_POINTSTO><IDENTIFIER>
		{
			get_token();
			// token |= SC_MEMBER;
			get_token();
		}
		else if (get_current_token_type() == TK_OPENBR)      // <TK_OPENBR><expression><TK_CLOSEBR>
		{
			get_token();
			expression();
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

/***********************************************************
 *  <primary_expression> ::= <IDENTIFIER>
 *     | <TK_CINT>
 *     | <TK_CCHAR>
 *     | <TK_CSTR>
 *     | <TK_OPENPA><expression><TK_CLOSEPA>
 **********************************************************/
void primary_expression()
{
	// 这里的addr暂时没有用到。这个是用法是首先调用allocate_storage函数，从这个函数中获得。
	// 这个函数主要用于在空间不够的时候，分配节的空间，同时返回当前符号在节中的存储位置。
	// 这里的节是一个可执行程序中的概念。例如代码节，数据节，符号表节。
	int t , r, addr = 0;
	Type typeExpression;
	Symbol *s ;
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
		t = T_CHAR;
		typeExpression.type = t;
		mk_pointer(&typeExpression);
		typeExpression.type |= T_ARRAY;
		// 字符串常量在.rdata节分配存储空间
        sec = allocate_storage(&typeExpression,SC_GLOBAL, 2, 0, &addr);
		var_sym_put(&typeExpression, SC_GLOBAL, 0, addr);
		initializer(&typeExpression, addr, sec);
		break;
	case TK_OPENPA:
		get_token();
		expression();
		skip_token(TK_CLOSEPA);
		break;
	default:
		char tkStr[128];
		t = get_current_token_type();
		strcpy(tkStr, get_current_token());
		get_token();
		if (t < TK_IDENT) // The problem of String at 07/11, We need the parse_string function
		{
			// print_error("Need variable or constant\n");
			printf("Need variable or constant\n");
		}
		s = sym_search(t);
		if(!s)
		{
			if (get_current_token_type() != TK_OPENPA)
			{
				// char tmpStr[128];
				// sprintf(tmpStr, "%s does not declare\n", get_current_token());
				printf("%s does not declare\n", tkStr);
				// print_error(testStr);
			}
			s = func_sym_push(t, &default_func_type);
			s->storage_class = SC_GLOBAL | SC_SYM;
		}
		r = s->storage_class;
		operand_push(&s->typeSymbol, r, s->related_value);
        /* 符号引用，操作数必须记录符号地址 */	
		if(operand_stack_top)
        if (operand_stack_top->r & SC_SYM) 
		{   
			operand_stack_top->sym = s;      
            operand_stack_top->value = 0;  //用于函数调用，及全局变量引用 printf("g_cc=%c\n",g_cc);
        }
		break;
	}
}

/***********************************************************
 *  <argument_expression_list> ::= <assignment_expression>
 *                      {<TK_COMMA><assignment_expression>}
 **********************************************************/
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
 *	等价转换后文法：：
 *	<struct_member_declaration> ::= 
 *	<type_specifier><declarator>{<TK_COMMA><declarator>}
 *	<TK_SEMICOLON>
 ***********************************************************************/
void struct_member_declaration(int * maxalign, int * offset, Symbol *** ps)
{
	int v, size, align;
	Symbol * ss;
	Type typeOne, bType ;
	int force_align;
	
	type_specifier(&bType);
	while (1)
	{
		v = 0 ;
		typeOne = bType;
		declarator(&typeOne, &v, &force_align);
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
		ss = sym_push(v | SC_MEMBER, &typeOne, 0, *offset);
		*offset += size;
		**ps = ss;
		*ps = &ss->next;
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

int calc_align(int n , int align)
{
	return (n + align -1) & (~(align -1));
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
	Symbol *s, **ps;
	get_token();
	// Adding Symbol operation
	s = type->ref;
	if (s->related_value != -1)
	{
		print_error("Has defined");
	}
	maxalign = 1;
	ps = &s->next;
	offset = 0 ;
	// end of Adding Symbol operation
	while (get_current_token_type() != TK_END)  // } 右大括号
	{
		struct_member_declaration(&maxalign, &offset, &ps);
	}
	skip_token(TK_END);
	// syntax_state = SNTX_LF_HT;
	s->related_value = calc_align(offset, maxalign);
	s->storage_class = maxalign;
}

/************************************************************************/
/*  <struct_specifier> ::= <KW_STRUCT><IDENTIFIER><TK_BEGIN>            */
/*                         <struct_declaration_list><TK_END>            */
/*                       | <KW_STRUCT><IDENTIFIER>                      */
/************************************************************************/
void struct_specifier(Type * typeStruct)
{
	int token_code ;
	Symbol * s;
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
	s = struct_search(token_code);
	if (!s)
	{
		typeOne.type = T_STRUCT;
		s = sym_push(token_code | SC_STRUCT, &typeOne, 0, -1);
		s->storage_class = 0;
	}
	typeStruct->type = T_STRUCT;
	typeStruct->ref = s;
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
/*  功能：解析外部声明                                                  */
/*  iSaveType e_StorageClass  存储类型                                  */
/************************************************************************/
// 这里涉及到这个系统中比较精妙的一个部分。
// 就是我们认为结构体声明，全局变量声明和函数（声明和定义）都是外部声明。
// 这样，我们就把包含多个函数，多个全局变量声明和多个结构体声明的代码看成是多个外部声明。
// 之后语法解析就变成解析一个一个的外部声明。
// 同时为了简化设计，结构体声明，必须集中放在文件开头。
// 后面跟着若干个函数声明，函数定义和全局变量声明。
// 而函数external_declaration的作用就是处理一个外部声明。一旦处理成功就返回。
void external_declaration(e_StorageClass iSaveType)
{
	Type bTypeCurrent, typeCurrent ;
	// v:	符号编号。类型为e_TokenCode。这里改为symbol_index。
	// int v = -1; // , has_init, addr;
	int symbol_index = -1; 
	
	// 这里的addr暂时没有用到。这个是用法是首先调用allocate_storage函数，从这个函数中获得。
	// 这个函数主要用于在空间不够的时候，分配节的空间，同时返回当前符号在节中的存储位置。
	// 这里的节是一个可执行程序中的概念。例如代码节，数据节，符号表节。
	// int has_init, r, addr = 0;
	int has_init, storage_class, addr = 0;
	Symbol * sym;
	Section * sec = NULL;
	print_all_stack("Starting external_declaration");
	if (!type_specifier(&bTypeCurrent))
	{
		print_error("Need type token\n");
	}
	// 因此上，如果前面type_specifier有处理结构体，这时bTypeCurrent.t就会等于T_STRUCT。
	// 这说明这一次，我们处理完了一个外部声明。我们就返回。
	if (bTypeCurrent.type == T_STRUCT && get_current_token_type() == TK_SEMICOLON)
	{
		print_all_stack("End external_declaration");
		get_token();
		return;
	}
	// 代码执行到这里，说明我们已经处理完成了所有的结构体声明。
	// 下面处理混合排列的函数声明，函数定义和全局变量声明。
	while (1)
	{
		typeCurrent = bTypeCurrent;
		declarator(&typeCurrent, &symbol_index, NULL);
		// 函数定义
		if (get_current_token_type() == TK_BEGIN)
		{
			// 函数不能是本地的。只能是全局的。
			if (iSaveType == SC_LOCAL)
			{
				print_error("Not nesting function\n");
			}
			// 声明后面跟着括号。必须是函数。否则就报错。
			if((typeCurrent.type & T_BTYPE) != T_FUNC)
			{
				print_error("Needing function defination\n");
			}
			// 如果查找到了这个符号。说明前面进行了函数定义。
			sym = sym_search(symbol_index);
			if(sym)
			{
				// 如果发现前面定义的不是函数定义。而是一个变量定义，就报错退出。
				if((sym->typeSymbol.type & T_BTYPE) != T_FUNC)
				{
					char tmpStr[128];
					sprintf(tmpStr, "Function %s redefination\n", get_current_token());
					print_error(tmpStr);
				}
				sym->typeSymbol = typeCurrent;
			}
			// 如果没有找到，就直接添加。
			else
			{
				sym = func_sym_push(symbol_index, &typeCurrent);
			}
			// 全局普通符号。
			sym->storage_class = SC_SYM | SC_GLOBAL;
			print_all_stack("Enter funtion body");
			func_body(sym);
			break;
		}
		else
		{
			// 函数声明
			if((typeCurrent.type & T_BTYPE) == T_FUNC)
			{
				if (sym_search(symbol_index) == NULL)
				{
					sym = sym_push(symbol_index, &typeCurrent, SC_GLOBAL | SC_SYM, 0);
				}
			}
			// 变量声明
			else
			{
			    storage_class = 0;
				// 如果是数组。就只能是左值。
				if(!(typeCurrent.type & T_ARRAY))
				{
					storage_class |= SC_LVAL;
				}
				storage_class |= iSaveType;
				// 如果变量声明的时候同时做了赋初值。执行赋初值操作。
				// if (get_current_token_type() == TK_ASSIGN)  // int a = 5 ;
				has_init = (get_current_token_type() == TK_ASSIGN);
				// 这里的这句if(has_init)其实可以等价于：
				//     if (get_current_token_type() == TK_ASSIGN)
				// 但是这个判断用的太多了。于是这里提出来。
				// 同时，这个判断是不受get_token影响的。
				if(has_init)
				{
					get_token();
				//	initializer(&typeCurrent);
				}
				// 这里主要做了3项工作：
				//   (1) 为变量分配存储空间，局部变量存放在栈中，
				//       全局变量分两类，声明时进行赋值的存放在.data数据节中，
				//       声明时不进行赋值的存放在.bss节中。
				// 首先为符号分配空间，
			    sec = allocate_storage(&typeCurrent, storage_class, has_init, symbol_index, &addr);
				// 之后添加符号。
				sym = var_sym_put(&typeCurrent, storage_class, symbol_index, addr);
				//   (2) 将全局变量放人COFF符号表。
			    if (iSaveType == SC_GLOBAL)
			    {
				    coffsym_add_update(sym, addr, sec->index, 0, IMAGE_SYM_CLASS_EXTERNAL);
			    }
				// (3) 对声明时进行赋值的变量进行初始化。
				//     如果上一个token是等号，使用当前token进行赋值。
			    if (has_init)  // int a = 5 ;
			    {
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
/* 功能：分配存储空间                                                   */
/* type：变量类型                                                       */
/* r：变量存储类型                                                      */
/* has_in it：是否需要进行初始化                                        */
/* v：变量符号编号                                                      */
/* addr(输出) ：变量存储地址                                            */
/* 返回值：变量存储节                                                   */
/************************************************************************/
Section * allocate_storage(Type * typeCurrent, int storage_class, int has_init, int token_code, int *addr)
{
	int size, align;
	Section * sec= NULL;
	size = type_size(typeCurrent, &align);

	if (size < 0)
	{
		if (typeCurrent->type & T_ARRAY 
			&& typeCurrent->ref->typeSymbol.type == T_CHAR)
		{
			typeCurrent->ref->related_value = strlen((char *)get_current_token()) + 1;
			size = type_size(typeCurrent, &align);
		}
		else
			print_error("Unknown size of type");
	}

	// 局部变量在栈中分配存储空间
	if ((storage_class & SC_VALMASK) == SC_LOCAL)
	{
		loc = calc_align(loc - size, align);
		*addr = loc;
	}
	else
	{
		// 初始化的全局变量在.data节分配存储空间
		// 通过判断(get_current_token_type() == TK_ASSIGN)得到。
		if (has_init == SEC_DATA_STORAGE)
		{
			sec = sec_data;
		}
		// 字符串常量在.rdata节分配存储空间
		// 代码位于primary_expression的TK_CSTR分支。
		else if (has_init == SEC_RDATA_STORAGE)	
		{
			sec = sec_rdata;
		}
		// 未初始化的全局变量在.bss节分配存储空间
		// 通过判断(get_current_token_type() == TK_ASSIGN)得到。
		// (has_init == SEC_BSS_STORAGE)
		else if (has_init == SEC_BSS_STORAGE)	
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
		
		// 为需要初始化的数据在节中分配存储空间
		if (sec->sh.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA
			&& sec->data_offset > sec->data_allocated)
		{
			section_realloc(sec, sec->data_offset);
		}
		// 常量字符串
		if (token_code == 0)
		{
			operand_push(typeCurrent, SC_GLOBAL | SC_SYM, *addr);
		//	operand_stack_top->sym = sec_rdata;
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

