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

int return_symbol_pos;			// 记录return指令位置

extern Section *sec_text,	// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_rdata;			// 只读数据节

extern int sec_text_opcode_offset ;	 	// 指令在代码节位置
extern int function_stack_loc ;			// 局部变量在栈中位置
extern Symbol *sym_sec_rdata;			// 只读节符号

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;
extern std::vector<Operand>::iterator operand_stack_last_top;

extern std::vector<Symbol *> global_sym_stack;  //全局符号栈
extern std::vector<Symbol *> local_sym_stack;   //局部符号栈
extern std::vector<TkWord> tktable;

extern Type char_pointer_type,		// 字符串指针
	 int_type,				// int类型
	 default_func_type;		// 缺省函数类型

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
void init_variable(Type * type, Section * sec, int valueAddr); // , int v);

void print_error(char * strErrInfo, char * subject_str)
{
	printf("<ERROR> '%s' %s", subject_str, strErrInfo);
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
			print_error("Need intergat\n", get_current_token());
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
	direct_declarator(type, iTokenTypePtr, fc);
}

/* <direct_declarator> ::= <IDENTIFIER><direct_declarator_postfix>              */
void direct_declarator(Type * type, int * iTokenTypePtr, int func_call)   // Not support argv[] usage
{
	if(get_current_token_type() >= TK_IDENT)  // 函数名或者是变量名，不可以是保留关键字。 
	{
		*iTokenTypePtr = get_current_token_type();
		get_token();
	}
	else
	{
		print_error("direct_declarator can not be TK_IDENT\n", "");
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
	int array_size;
	Symbol * symArrayPtr;
	// 函数参数：<TK_OPENPA><parameter_type_list><TK_CLOSEPA> | <TK_OPENPA><TK_CLOSEPA>
	// Such as : var(), var(int a, int b)
	if(get_current_token_type() == TK_OPENPA)         
	{
		parameter_type_list(type, func_call);
	}
	// 数组参数：<TK_OPENBR><TK_CINT><TK_CLOSEBR> | <TK_OPENBR><TK_CLOSEBR>
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
			print_error("Need inter\n", get_current_token());
		}
		skip_token(TK_CLOSEBR);
		direct_declarator_postfix(type, func_call);    // Nesting calling
		// 把数组大小保存到符号表中，作为一个匿名符号。
		// SC_ANOM不是e_StorageClass的一部分。而是e_TokenCode类型。
		// SC_ANOM只不过是被放在e_StorageClass里面而已。
		symArrayPtr = sym_push(SC_ANOM, type, 0, array_size);
		type->typeCode = T_ARRAY | T_PTR;
		type->ref = symArrayPtr;
	}
}

/**************************************************************************/
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
/**************************************************************************/
void parameter_type_list(Type * type, int func_call) // (int func_call)
{
	int iTokenType;
	Symbol **lastSymPtr, *sym, *firstSym;
	Type typeCurrent ;
	get_token();
	firstSym = NULL;
	lastSymPtr = &firstSym;
	
	while(get_current_token_type() != TK_CLOSEPA)   // get_token until meet ) 右圆括号
	{
		if(!type_specifier(&typeCurrent))
		{
			print_error("Invalid type_specifier\n", get_current_token());
		}
		// Translate one parameter declaration
		declarator(&typeCurrent, &iTokenType, NULL);
		// SC_PARAMS不是e_StorageClass的一部分。而是e_TokenCode类型。
		// SC_PARAMS只不过是被放在e_StorageClass里面而已。
		sym = sym_push(iTokenType|SC_PARAMS, &typeCurrent, 0, 0);
		*lastSymPtr = sym;
		lastSymPtr  = &sym->nextSymbol;
		
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
	// SC_ANOM不是e_StorageClass的一部分。而是e_TokenCode类型。
	// SC_ANOM只不过是被放在e_StorageClass里面而已。
	sym = sym_push(SC_ANOM, type, func_call, 0);
	sym->nextSymbol  = firstSym;
	type->typeCode   = T_FUNC;
	type->ref        = sym;
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
	sec_text_opcode_offset = sec_text->data_offset;
	//    更新sym->related_value为符号COFF符号表中序号。
	coffsym_add_update(sym, sec_text_opcode_offset, 
		sec_text->cSectionIndex, CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
	
	/* 2. 放一个匿名符号到局部符号表中 */
	// SC_ANOM不是e_StorageClass的一部分。而是e_TokenCode类型。
	// SC_ANOM只不过是被放在e_StorageClass里面而已。
	sym_direct_push(local_sym_stack, SC_ANOM, &int_type, 0);

	// 3. 生成函数开头代码。
	gen_prologue(&(sym->typeSymbol));
	return_symbol_pos = 0;

	// 4. 调用复合语句处理函数处理函数体中的语句。
	// print_all_stack("sym_push local_sym_stack");
	compound_statement(NULL, NULL);
	// print_all_stack("Left local_sym_stack");

	// 5. 函数返回后，回填返回地址。
	jmpaddr_backstuff(return_symbol_pos, sec_text_opcode_offset);
	
	// 6. 生成函数结尾代码。
	gen_epilogue();
	sec_text->data_offset = sec_text_opcode_offset;

	/* 7. 清空局部符号表栈 */
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
			global_sym_stack[idx]->token_code, 
			global_sym_stack[idx]->related_value, 
			global_sym_stack[idx]->typeSymbol);
	}
	for(idx = 0; idx < local_sym_stack.size(); ++idx)
	{
		printf("\t  local_sym_stack[%d].TokenType = %08X value = %d type = %d\n", idx, 
			local_sym_stack[idx]->token_code, 
			local_sym_stack[idx]->related_value, 
			local_sym_stack[idx]->typeSymbol);
	}
	printf("\t tktable.size = %d \n --- ", tktable.size());
	for (idx = 40; idx < tktable.size(); idx++)
	{
		printf(" %s ", tktable[idx].spelling);
	}
	printf(" --- \n----------------------------------------------\n");
}

// 初值符的代码如下所示。
/************************************************************************/
/* <initializer> ::= <assignment_expression>                            */
/************************************************************************/
/* 功能： 变量初始化                                                    */
/* type： 变量类型。也就是左值的类型。                                  */
/* value：变量相关值。对于全局变量和字符串常量为节内部偏移量。          */
/* sec：  变量所在节。对于全局变量和字符串常量为相应节地址。            */
/*                   对于局部变量为NULL。                               */
/************************************************************************/
/* 函数逻辑如下：                                                       */
/*   1. 这个函数的主分支是else部分。                                    */
/*      1.1 首先调用assignment_expression进行表达式分析。               */
/*          因为右值，也有可能是一个表达式。                            */
/*      1.2 之后，我们调用init_variable完成赋值操作。                   */
/*   2. 这个函数的副分支是if部分。该部分对于字符串赋值进行特殊处理。    */
/*      这个分支包括两个调用路径。                                      */
/*      2.1 局部字符串变量。char str1[] = "AAAAA"; 处理流程为：         */
/*	        在我们调用assignment_expression进行表达式分析的时候，       */
/*		    如果右值是一个字符串，会进入primary_expression中处理。      */
/*		    走TK_CSTR分支。为变量分配空间，把变量放入符号表。           */
/*		    之后嵌套调用initializer完成变量初始化。                     */
/*      2.2 全局字符串变量。char g_str1[] = "AAAAA"; 处理流程为：       */
/*          在external_declaration中，为变量分配空间，把变量放入符号表。*/
/*		    之后嵌套调用initializer完成变量初始化。                     */
/*		因为前面调用allocate_storage分配空间的时候，返回值sec不是空。   */
/*		同时我们前面指定了T_ARRAY类型标志。                             */
/*		这导致我们进入initializer函数的if部分。                         */
/*		把字符串memcpy到节的指定位置。                                  */
/************************************************************************/
void initializer(Type * typeToken, int valueAddr, Section * sec)
{
	// 如果是数据变量的数组初始化。例如我们定义了两个变量，
	// 当我们解析第二个变量的时候，我们就会进入这个分支：
	//     char g_char = 'a';  
	//     char ss[5] = "SSSSS";
	if((typeToken->typeCode & T_ARRAY) && sec)
	{
	    // 只需要把右值直接复制到节内部即可。复制后sec->data的内容变成：
		//   "aSSSSS"
		memcpy(sec->bufferSectionData + valueAddr, get_current_token(), strlen(get_current_token()));
		get_token();
	}
	else
	{
		// 解析全局变量，字符串常量和局部变量的右值。
		assignment_expression();
		init_variable(typeToken, sec, valueAddr); // , 0);
		// get_token can not move here. It only work when token == TK_CSTR
		// get_token();
	}
}

/************************************************************************/
/* 功能： 变量初始化                                                    */
/* type： 变量类型。也就是左值的类型。                                  */
/* sec：  变量所在节。对于全局变量和字符串常量为相应节地址。            */
/*                   对于局部变量为NULL。                               */
/* value：变量相关值。对于全局变量和字符串常量为节内部偏移量。          */
/* idx：  变量符号编号。这个值好像没有用。                              */
/************************************************************************/
/* 这个函数在initializer中，在assignment_expression函数后，被调用。     */
/* 这里涉及到我们如何理解赋值操作。                                     */
/* 对于计算机来说，左值所代表的变量就是内存中的一个地址。               */ 
/* 因此上，对于全局变量和字符串常量，他就是一个节地址。                 */
/*       而对于对于局部变量，他就是一个operand_stack元素。              */
/************************************************************************/
/*   1. 在assignment_expression函数中，全局变量，字符串常量和局部变量的 */
/*      右值已经在primary_expression函数中被压栈。                      */
/*   2. 全局变量和字符串常量的情况：                                    */
/*      这两种情况数据保存在节上，我们只需要写对应的节即可。            */
/*   3. 局部变量的情况：                                                */
/*      当我们进入init_variable函数的时候，右值位于栈顶。               */
/*      我们只需要把左值类型压栈，让栈顶元素为左值。次栈顶元素为右值。  */
/*      之后执行operand_swap。让栈顶元素变为右值。次栈顶元素变为左值。  */
/*      最后调用store_zero_to_one将栈顶右值存入次栈顶左值即可。         */
/************************************************************************/
void init_variable(Type * type, Section * sec, int valueAddr) // , int idx)
{
	// 2022/11/14
	int bt;
	void * ptr;
	// 如果节地址不为空，说明初始化的是全局变量和字符串常量。
	if (sec)
	{
		if(operand_stack_top)
		{
			if ((operand_stack_top->storage_class &(SC_VALMASK | SC_LVAL)) != SC_GLOBAL)
			{
				print_error("Use constant to initialize the global variable"
					, get_current_token());
			}
		}
		// 得到数据类型。
		bt = type->typeCode & T_BTYPE;
		// 得到保存全局变量和字符串常量的内存位置。
		ptr = sec->bufferSectionData + valueAddr;
		// 更新节中对应的位置。
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
						operand_stack_top->sym, valueAddr, IMAGE_REL_I386_DIR32);
				}
			}
			*(int *)ptr = operand_stack_top->operand_value;
			break;
		}
		operand_pop();
	}
	// 如果节地址不为空，说明初始化的是放在栈上的局部变量。
	else
	{
		// 如果左值是数组。那么就是数组初始化。例如：
		//     char  str1[]  = "str 1";
		if (type->typeCode & T_ARRAY)
		{
			operand_push(type, SC_LOCAL | SC_LVAL, valueAddr);
			operand_swap();
			array_initialize();
		}
		else
		{
			// 对于局部变量，左值就是一个operand_stack元素。只有类型才有用。直接把类型压栈即可。
			// 在压栈之前，栈顶元素为右值。压栈以后，栈顶元素为左值。和次栈顶元素为右值。
			operand_push(type, SC_LOCAL | SC_LVAL, valueAddr);
			// 交换栈顶元素和次栈顶元素。栈顶元素变为右值。次栈顶元素变为左值。
			operand_swap();
			// 将栈顶操作数也就是右值，存入次栈顶操作数，也就是左值中。
			store_zero_to_one();
			// 赋值完成后，栈顶操作数也就是右值就没用了。直接弹出舍弃即可。
			// 这里补充一下，我们现在是生成机器码。
			// 如果右值是表达式，在前面的assignment_expression函数中生成对应的机器码。因此上不用担心。
			operand_pop();
		}
	}
}

// ------------------ 语句：statement
/************************************************************************/
/* 功能:	         解析语句                                           */
/* break_address:	 break跳转位置。暂时没有用。                        */
/* continue_address: continue跳转位置。暂时没有用。                     */
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
		compound_statement(break_address, continue_address);			// 复合语句
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
/* 功能:	         解析复合语句                                       */
/* break_address:	 break跳转位置                                      */
/* continue_address: continue跳转位置                                   */
/*                                                                      */
/* <compound_statement>::=<TK_BEGIN>                                    */
/*                           {<declaration>}{<statement>}               */
/*                        <TK_END>                                      */
/************************************************************************/
void compound_statement(int * break_address, int * continue_address)
{
	Symbol *sym;
	// s = &(local_sym_stack[0]);
	sym = *(local_sym_stack.end() - 1);
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
/*   TK_SEMICOLON 是 ; 分号                                             */
/************************************************************************/
void expression_statement()
{
	if(get_current_token_type() != TK_SEMICOLON)
	{
		expression();
		// 计算完一个表达式，退一次栈。
		operand_pop();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);

}

/************************************************************************/
/* <if_statement> ::= <KW_IF><TK_OPENPA><expression>                    */
/*          <TK_CLOSEPA><statement>[<KW_ELSE><statement>]               */
/************************************************************************/
/* 一条形如 if(a > b) 语句包括四条指令。                                */
/*   MOV EAX, DWORD PTR SS: [EBP-8]                                     */
/*   MOV ECX, DWORD PTR SS: [EBP-4]                                     */
/*   CMP ECX, EAX                                                       */
/*   JLE scc_anal.0040123F                                              */
/* 可以看出来，对应关系还是非常明显的。首先是把变量放在EAX和ECX，       */
/* 之后调用CMP进行比较。根据比较结果，如果a不大于b，就跳转到else分支。  */
/* 反之，如果a大于b，说明条件符合，直接运行后面的语句即可，无需跳转。   */
/*                                                                      */
/* 一条 else 语句包括一条指令。                                         */
/*   JMP scc_anal. 00401245                                             */
/* 执行到else语句说明，IF后面的语句执行完了，直接跳过ELSE部分即可。     */
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
	// 为IF语句生成条件跳转指令。
	if_jmp_addr = gen_jcc(0);
	statement(break_address, continue_address);

	if(get_current_token_type() == KW_ELSE)
	{
		syntax_state = SNTX_LF_HT;
		get_token();
		// 为ELSE语句生成跳转指令。
		else_jmp_addr = gen_jmpforward(0);
		// 为IF语句填人相对地址。
		jmpaddr_backstuff(if_jmp_addr, sec_text_opcode_offset);
		statement(break_address, continue_address);
		// 为ELSE语句填人相对地址。
		jmpaddr_backstuff(else_jmp_addr, sec_text_opcode_offset);
	}
	else
	{
		// 为IF语句填人相对地址。
		jmpaddr_backstuff(if_jmp_addr, sec_text_opcode_offset);
	}
}	

/************************************************************************/
/*  <for_statement> ::= <KW_FOR><TK_OPENPA><expression_statement>       */
/*        <expression_statement><expression><TK_CLOSEPA><statement>     */
/************************************************************************/
/* 一条形如 for(i = 0; i < 10; i++) 语句包括如下的指令。                */
/* 首先是赋初值的i = 0对应的指令如下:                                   */
/*  1. MOV EAX, 0                                                       */
/*  2. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* 就是一个简单的赋值操作。                                             */
/*  1. 把0放入EAX。                                                     */
/*  2. 之后放入变量中。                                                 */
/*                                                                      */
/* 然后是i < 10对应的指令如下:                                          */
/*  3. MOV EAX, DWORD PTR SS：[EBP-4]                                   */
/*  4. CMP EAX, 0A                                                      */
/*  5. JGE scc_anal.0040128D                                            */
/*  6. JMP scc_anal.00401276                                            */
/* 操作步骤如下：                                                       */
/*  3 & 4. 把变量和10进行比较。                                         */
/*  5. 如果比10大，就跳转到整个for语句体的结尾。                        */
/*  6. 否则，就跳转到整个for语句体里面。                                */
/*                                                                      */
/* 然后是i++对应的指令如下:                                             */
/*  7. MOV EAX, DWORD PTR SS：[EBP-4]                                   */
/*  8. ADD EAX, 1                                                       */
/*  9. MOV DWORD PTR SS: [EBP-4], EAX                                  */
/* 10. JMP SHORT scc_anal.0040125A                                      */
/* 操作步骤如下：                                                       */
/* 7 & 8 & 9. 把变量加一。                                              */
/* 10. 然后跳转到i < 10的地方。                                         */
/*                                                                      */
/* 下面是arr[i] = i对应的指令:                                          */
/* 11. MOV  EAX, 4                                                      */
/* 12. MOV  ECX, DWORD PTR SS: [EBP-4]                                  */
/* 13. IMUL ECX, EAX                                                    */
/* 14. LEA  EAX, DWORD PTR SS: [EBP-2C]                                 */
/* 15. ADD  EAX, ECX                                                    */
/* 16. MOV  ECX, DWORD PTR SS: [EBP-4]                                  */
/* 17. MOV  DWORD PTR DS: [EAX], ECX                                    */
/* 18. JMP  SHORT scc_anal.0040126B                                     */
/* 可以看到逻辑也非常简单。                                             */
/* 11. 首先EAX设置为4，                                                 */
/* 12. 之后取出i的值放入ECX。                                           */
/* 13. 之后把ECX和EAX相乘，放入ECX。                                    */
/* 14. 之后取出数组首地址放入EAX。                                      */
/* 15. 之后加上ECX，得到了r[i]的地址。结果放入EAX。                     */
/* 16. 取出i的值放入ECX。                                               */
/* 17. 完成arr[i] = i的赋值操作。                                       */
/* 18. 完成循环体，跳转到i++指令的位置。                                */
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
	// 现在我们位于for语句的第一个分号位置。首先记录下循环头的退出条件的位置。
	for_exit_cond_address = sec_text_opcode_offset;
	// 初始化循环头的累加条件的位置。因为循环累加条件有可能不存在。
	for_inc_address = sec_text_opcode_offset;
	// 初始化循环体结束位置。
	for_end_address = 0;
	// b = 0;
	// Chapter II
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		// 生成循环头的退出的跳转语句。例如：5. JGE scc_anal.0040128D
		// 跳转地址在处理完for语句以后回填。
		for_end_address = gen_jcc(0);
		skip_token(TK_SEMICOLON);
	}
	else 						// Such as for(n = 0; ; n++)
		skip_token(TK_SEMICOLON);

	// Chapter III
	if(get_current_token_type() != TK_CLOSEPA)   // Such as for(n = 0; n < 100; n++)
	{
		// 生成循环头的不退出的跳转语句。例如：6. JMP scc_anal. 00401276
		for_body_address = gen_jmpforward(0);
		// 代码执行到这里我们来到了循环头的累加部分。记录下这个位置。
		for_inc_address = sec_text_opcode_offset;
		// 处理循环头的累加部分。
		expression();
		operand_pop();
		// 累加处理完了以后，跳转到循环头的退出判断的地方。
		// 例如：10. JMP SHORT scc_anal.0040125A
		gen_jmpbackward(for_exit_cond_address);
		// 执行到了这里，我们来到了循环循环体。我们可以填充循环体的起始地址了。
		jmpaddr_backstuff(for_body_address, sec_text_opcode_offset);
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	else 						// Such as for(n = 0; n < 100;)
	{
		// 如果没有累加部分，则什么都不用做。
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	// Deal with the body of for_statement 
	// 只有此处用到break,及continue,一个循环中可能有多个break,或多个continue,故需要拉链以备反填
	statement(&for_end_address, &for_inc_address); 
	// 循环体处理完了以后，跳转到循环头累加的地方。
	// 例如：18. JMP  SHORT scc_anal.0040126B
	gen_jmpbackward(for_inc_address);
	// 执行到了这里，我们可以填充循环体的结束地址了。
	jmpaddr_backstuff(for_end_address, sec_text_opcode_offset);
	// 这句话好像是多余的。因为b一直是零。而且如果传入0，jmpaddr_backstuff什么都不会做。
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
	// 现在我们位于while语句的左括号位置。首先记录下while循环退出条件的位置。
	while_exit_cond_address = sec_text_opcode_offset;
	// 初始化循环体结束位置。
	while_end_address = 0;
	// Chapter I
	expression();
	// 生成循环头的退出的跳转语句。例如：5. JGE scc_anal.0040128D
	// 跳转地址在处理完for语句以后回填。
	while_end_address = gen_jcc(0);
	skip_token(TK_CLOSEPA);
	// 这里不需要生成循环头的不退出的跳转语句。因为后面就是循环体。
	// while_body_address = gen_jmpforward(0);

	syntax_state = SNTX_LF_HT;
	// Deal with the body of while_statement 
	statement(&while_end_address, &while_exit_cond_address); 
	
	// 循环体处理完了以后，跳转到循环头。
	// 例如：18. JMP  SHORT scc_anal.0040126B
	gen_jmpbackward(while_exit_cond_address);
	// 循环体处理完了以后，跳转到循环头。
	// 执行到了这里，我们可以填充循环体的结束地址了。
	jmpaddr_backstuff(while_end_address, sec_text_opcode_offset);
}

/************************************************************************/
/*  <continue_statement> ::= <KW_CONTINUE><TK_SEMICOLON>                */
/************************************************************************/
/* 一条形如 if(i==2) continue; 的语句包括如下的指令。                   */
/* 首先是 if(i==2) 对应的指令如下:                                      */
/*  1. MOV EAX, DWORD PTR SS: [EBP-4]                                   */
/*  2. CMP EAX, 2                                                       */
/*  3. JNZ scc_anal.004012CF                                            */
/* 就是一个简单的比较跳转操作。                                         */
/*  1. 把i的值放入EAX。                                                 */
/*  2. 之后和2比较。                                                    */
/*  3. 如果不等于，就跳转到if语句体的结尾处。                           */
/* 接着 continue; 对应的指令如下:                                       */
/*  4. JMP scc_anal.004012B3                                            */
/* 就是直接跳转到外层for语句的累加处。                                  */
/************************************************************************/
void continue_statement(int *continue_address)
{
	if (!continue_address)
	{
		print_error("Can not use continue", "");
	}
	// 生成跳转到外层for语句累加处的汇编代码。
	* continue_address = gen_jmpforward(*continue_address);
	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/************************************************************************/
/*  <break_statement> ::= <KW_CONTINUE><TK_SEMICOLON>                   */
/************************************************************************/
/* 一条形如 if(i==6) break; 的语句包括如下的指令。                      */
/* 首先是 if(i==6) 对应的指令如下:                                      */
/*  1. MOV EAX, DWORD PTR SS: [EBP-4]                                   */
/*  2. CMP EAX, 6                                                       */
/*  3. JNZ scc_anal.004012E0                                            */
/* 就是一个简单的比较跳转操作。                                         */
/*  1. 把i的值放入EAX。                                                 */
/*  2. 之后和6比较。                                                    */
/*  3. 如果不等于，就跳转到if语句体的结尾处。                           */
/* 接着 break; 对应的指令如下:                                          */
/*  4. JMP scc_anal.0040130D                                            */
/* 就是直接跳转到外层for语句的循环体结尾处。                            */
/************************************************************************/
void break_statement(int *break_address)
{
	if (!break_address)
	{
		print_error("Can not use break", "");
	}
	// 生成跳转到外层for语句的循环体结尾处的汇编代码。
	* break_address = gen_jmpforward(*break_address);

	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/************************************************************************/
/*  <return_statement> ::= <KW_RETURN><TK_SEMICOLON>                    */
/*                       | <KW_RETURN><expression><TK_SEMICOLON>        */
/************************************************************************/
/* 一条形如 return 1; 的语句包括如下的指令。                            */
/*  1. MOV EAX, 1                                                       */
/*  2. JMP scc_anal.00401317                                            */
/* 就是一个简单的赋值跳转操作。                                         */
/*  1. 把返回值1放入EAX。                                               */
/*  2. 之后跳转到函数体的末尾。后续操作由gen_epilog完成。               */
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
		// 将栈顶操作数，也就是返回值，加载到'rc'类寄存器中。
		// 例如： 1. MOV EAX, 1
		load_one(REG_IRET, operand_stack_top);
		operand_pop();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
	// 生成跳转指令的汇编代码。
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
/*  这里有左递归，可以提取公因子                                        */
/*  非等价变换后文法：                                                  */
/*  <assignment_expression> ::= <equality_expression>                   */
/*                  {<TK ASSIGN><assignment expression>}                */
/************************************************************************/
void assignment_expression()
{
	equality_expression();
	// 这里处理连续赋值的情况。例如：
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
/* 一条形如 c = a == b; 的语句包括如下的指令。                          */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETE AL                                                          */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* 一条形如 c = a != b; 的语句包括如下的指令。                          */
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
		// 获得运算符
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
/*       <TK_LT><additive_expression>    // < 小于号      TK_LT, "<",   */
/*     | <TK_GT><additive_expression>    // <= 小于等于号 TK_LEQ, "<=", */
/*     | <TK_LEQ><additive_expression>   // > 大于号      TK_GT, ">",   */
/*     | <TK_GEQ><additive_expression>}  // >= 大于等于号 TK_GEQ, ">=", */
/************************************************************************/
/* 一条形如 c = a > b; 的语句包括如下的指令。                           */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SET GAL                                                          */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* 一条形如 c = a >= b; 的语句包括如下的指令。                          */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETGE AL                                                         */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* 一条形如 c = a < b; 的语句包括如下的指令。                           */
/*  1. MOV EAX, DWORD PTR SS: [EBP-8]                                   */
/*  2. MOV ECX, DWORD PTR SS: [EBP-4]                                   */
/*  3. CMP ECX, EAX                                                     */
/*  4. MOV EAX, 0                                                       */
/*  5. SETL AL                                                          */
/*  6. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* 一条形如 c = a <= b; 的语句包括如下的指令。                          */
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
		// 获得运算符
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
/* 一条形如 c = a + b; 的语句包括如下的指令。                           */
/*  1. MOV EAX,  DWORD PTR SS：[EBP-8]                                  */
/*  2. MOV ECX,  DWORD PTR SS：[EBP-4]                                  */
/*  3. ADD ECX,  EAX                                                    */
/*  4. MOV DWORD PTR SS: [EBP-C], ECX                                   */
/* 一条形如 d = a + 8; 的语句包括如下的指令。                           */
/*  1. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  2. ADD EAX,  8                                                      */
/*  3. MOV DWORD PTR SS: [EBP-10], EAX                                  */
/* 一条形如 e = 6 + 8; 的语句包括如下的指令。                           */
/*  1. MOV EAX,  6                                                      */
/*  2. ADD EAX,  8                                                      */
/*  3. MOV DWORD PTR SS: [EBP-14], EAX                                  */
/* 一条形如 c = a - b; 的语句包括如下的指令。                           */
/*  1. MOV EAX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV ECX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. SUB ECX,  EAX                                                    */
/*  4. MOV DWORD PTR SS: [EBP-C], ECX                                   */
/* 一条形如 d = a - 8; 的语句包括如下的指令。                           */
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
		// 获得运算符
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
/* 一条形如 c = a * b; 的语句包括如下的指令。                           */
/*  1. MOV  EAX,  DWORD PTR SS: [EBP-8]                                 */
/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. MOV  DWORD PTR SS: [EBP-C], ECX                                  */
/* 一条形如 d = a * 8; 的语句包括如下的指令。                           */
/*  1. MOV  EAX,  8                                                     */
/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. MOV  DWORD PTR SS: [EBP-10], ECX                                 */
/* 一条形如 e = 6 * 8; 的语句包括如下的指令。                           */
/*  1. MOV  EAX,  8                                                     */
/*  2. MOV  ECX,  6                                                     */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. MOV DWORD PTR SS: [EBP-14], ECX                                  */
/* 一条形如 c = a / b; 的语句包括如下的指令。                           */
/*  1. MOV ECX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/* 一条形如 d = a / 8; 的语句包括如下的指令。                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-10], EAX                                  */
/* 一条形如 e = 6 / 8; 的语句包括如下的指令。                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  6                                                      */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-14], EAX                                  */
/* 一条形如 c = a % b; 的语句包括如下的指令。                           */
/*  1. MOV ECX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-C], EDX                                   */
/* 一条形如 d = a % 8; 的语句包括如下的指令。                           */
/*  1. MOV ECX,  8                                                      */
/*  2. MOV EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  3. CDQ                                                              */
/*  4. IDIV ECX                                                         */
/*  5. MOV DWORD PTR SS: [EBP-10], EDX                                  */
/* 一条形如 e = 6 % 8; 的语句包括如下的指令。                           */
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
		// 获得运算符
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
	// 间接寻址必须是指针。如果变量不是指针，就报错。
	// 例如：struct  point  pt; pt->x++;
	// 此时，pt是结构体，类型等于T_STRUCT，而不是T_PTR，就会报错。
	// 而如果写成：struct  point * ptPtr; pt->x++;
	// 此时，pt是结构体指针，类型等于T_PTR，就不会报错。
	if ((operand_stack_top->typeOperand.typeCode & T_BTYPE) != T_PTR)
	{
		// 除非是函数。
		if ((operand_stack_top->typeOperand.typeCode & T_BTYPE) == T_FUNC)
		{
			return;
		}
		print_error("Need pointer", get_current_token());
	}
	// 如果是本地变量，则该变量位于栈上。只需要讲
	if (operand_stack_top->storage_class & SC_LVAL)
	{
		load_one(REG_ANY, operand_stack_top);
	}
	operand_stack_top->typeOperand = *pointed_type(&operand_stack_top->typeOperand);

	// 如果不是数组与函数。就设置左值标志。因为数组与函数不能为左值
	if ((operand_stack_top->typeOperand.typeCode & T_BTYPE) != T_FUNC &&
		!(operand_stack_top->typeOperand.typeCode & T_ARRAY))
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
/* 变量定义如下：                                                       */
/*     int a, *pa, n;                                                   */
/* 一条形如 a = +8; 的语句包括如下的指令。                              */
/*  1. MOV EAX,  8                                                      */
/*  2. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* 一条形如 a = -8; 的语句包括如下的指令。                              */
/*  1. MOV EAX,  0                                                      */
/*  2. SUB EAX,  8                                                      */
/*  3. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* 一条形如 a = *pa; 的语句包括如下的指令。                             */
/*  1. MOV EAX,  DWORD PTR SS: [EBP-8]                                  */
/*  2. MOV ECX,  DWORD PTR DS: [EAX]                                    */
/*  3. MOV DWORD PTR SS: [EBP-4], ECX                                   */
/* 一条形如 pa = &a; 的语句包括如下的指令。                             */
/*  1. LEA EAX,  DWORD PTR SS: [EBP-4]                                  */
/*  2. MOV DWORD PTR SS: [EBP-8], EAX                                   */
/* 一条形如 n = sizeof(int); 的语句包括如下的指令。                     */
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
		// 如果不是数组与函数
		if ((operand_stack_top->typeOperand.typeCode & T_BTYPE) != T_FUNC &&
			!(operand_stack_top->typeOperand.typeCode & T_ARRAY))
		{
			cancel_lvalue();
		}
		mk_pointer(&operand_stack_top->typeOperand);
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
		print_error("sizeof failed.", get_current_token());
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
/* 变量定义如下：                                                       */
/*     struct point                                                     */
/*     {                                                                */
/*         int m_x;                                                     */
/*         int m_y;                                                     */
/*     };                                                               */
/*                                                                      */
/*     int x；                                                          */
/*     struct point pt;                                                 */
/*     int arr[10];                                                     */
/* 一条形如 x = pt.m_x; 的语句包括如下的指令。                          */
/*  1. MOV  EAX,  1                                                     */
/*  2. MOV  ECX,  0                                                     */
/*  3. IMUL ECX,  EAX                                                   */
/*  4. LEA  EAX,  DWORD PTR SS: [EBP-C]                                 */
/*  5. ADD  EAX,  ECX                                                   */
/*  6. MOV  ECX,  DWORD PTR DS: [EAX]                                   */
/*  7. MOV  DWORD PTR SS: [EBP-4], ECX                                  */
/* 这段汇编码的逻辑也很简单。首先m_x是struct point pt的第0个元素。      */
/* 所以ECX = 0，EAX表示数据大小。将ECX和EAX相乘得到偏移量。             */
/* 之后读取[EBP-C]位置的地址，也就是栈中pt的位置地址。之后加上偏移量。  */
/* 最后根据计算出来的偏移量读取数据到ECX。之后放到另一个栈位置中。      */
/************************************************************************/
/* 一条形如 x = arr[1]; 的语句包括如下的指令。                          */
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
		// 如果是结构体成员运算符"->"或"."。
		if (get_current_token_type() == TK_DOT               // | <TK_DOT><IDENTIFIER>
			|| get_current_token_type() == TK_POINTSTO)      // | <TK_POINTSTO><IDENTIFIER>
		{
			if (get_current_token_type() == TK_POINTSTO)
			{
				indirection();
			}
			cancel_lvalue();

			get_token();

			if ((operand_stack_top->typeOperand.typeCode & T_BTYPE) != T_STRUCT)
			{
				print_error("Need struct", get_current_token());
			}
			symbol = operand_stack_top->typeOperand.ref;

			// token |= SC_MEMBER;
			// SC_MEMBER不是e_StorageClass的一部分。而是e_TokenCode类型。
			// SC_MEMBER只不过是被放在e_StorageClass里面而已。
			set_current_token_type(get_current_token_type() | SC_MEMBER);
			
			while ((symbol = symbol->nextSymbol) != NULL)
			{
				if (symbol->related_value == get_current_token_type())
				{
					break;
				}
			}
			if (!symbol)
			{
				print_error("Not found", get_current_token());
			}
            /* 成员变量地址 = 结构变量指针 + 成员变量偏移 */
			/* 成员变量的偏移是指相对于结构体首地址的字节偏移，
			 * 因此为了计算字节偏移，此处变换类型为字节变量指针。 */
			operand_stack_top->typeOperand = char_pointer_type;
			operand_push(&int_type, SC_GLOBAL, symbol->related_value);
			gen_op(TK_PLUS);  // 执行后optop->value记忆了成员地址
            /* 变换类型为成员变量数据类型 */
			operand_stack_top->typeOperand = symbol->typeSymbol;
            /* 数组变量不能充当左值 */
			if (!(operand_stack_top->typeOperand.typeCode & T_ARRAY))
			{
				operand_stack_top->storage_class |= SC_LVAL;
			}

			get_token();
		}
		// 如果是数组下标符号 - 左中括号"["。
		else if (get_current_token_type() == TK_OPENBR)      // <TK_OPENBR><expression><TK_CLOSEBR>
		{
			// 获取数组下标的第一个token。
			get_token();
			// 计算数组下标。
			expression();
			// 计算数组偏移地址。
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
/* 全局变量定义如下：                                                   */
/*    char g_char = 'a';                                                */
/*    short g_short = 123;                                              */
/*    int g_int = 123456;                                               */
/*    char g_str1[] = "g_strl";                                         */
/*    char* g_str2 = "g_str2";                                          */                                       
/* 一条形如 char a = 'a'; 的语句包括如下的指令。                        */
/*    MOV EAX,  61                                                      */
/*    MOV BYTE PTR SS: [EBP-1], AL                                      */
/* 一条形如 short b = 8; 的语句包括如下的指令。                         */
/*    MOV EAX,  8                                                       */
/*    MOV WORD PTR SS: [EBP-2], AX                                      */
/* 一条形如 int c = 6; 的语句包括如下的指令。                           */
/*    MOV EAX,  6                                                       */
/*    MOV DWORD PTR SS: [EBP-4], EAX                                    */
/* 一条形如 char str1[]  = "str 1"; 的语句包括如下的指令。              */
/*    MOV ECX,  5                                                       */
/*    MOV ESI,  scc_anal.00403015; ASCII"strl"                          */
/*    LEA EDI,  DWORD PTR SS: [EBP-9]                                   */
/*    REP MOVS  BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]                  */
/* 一条形如 char* str2 = "str2"; 的语句包括如下的指令。                 */
/*    MOV EAX,  scc_anal.0040301A; ASCII"str2"                          */
/*    MOV DWORD PTR SS: [EBP-C], EAX                                    */
/* 一条形如 a = g_char; 的语句包括如下的指令。                          */
/*    ; MOVSX - 带符号扩展传送指令                                      */
/*    MOVSX EAX,  BYTE PTR DS: [402000]                                 */
/*    MOV BYTE PTR SS: [EBP-1], AL                                      */
/* 一条形如 b = g_short; 的语句包括如下的指令。                         */
/*    MOVSX EAX,  WORD PTR DS: [402002]                                 */
/*    MOV WORD PTR SS: [EBP-2], AX                                      */
/* 一条形如 c = g_int; 的语句包括如下的指令。                           */
/*    MOV  EAX,  DWORD PTR DS: [402004]                                 */
/*    MOV  DWORD PTR SS: [EBP-4], EAX                                   */
/* 一条形如 printf(g_str1); 的语句包括如下的指令。                      */
/*    MOV  EAX,  scc_anal.00402008; ASCII“g_strl"                       */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvert.printf>                                         */
/*    ADD  ESP, 4                                                       */
/* 一条形如 printf(printf(g_str2);); 的语句包括如下的指令。             */
/*    MOV  EAX, DWORD PTR: [402010]: scc_anal.0040300E                  */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&-ms vert.printf>                                       */
/*    ADD  ESP,  4                                                      */
/* 一条形如 printf(str1); 的语句包括如下的指令。                        */
/*    LEA  EAX, DWORD PTR SS: [EBP-9]                                   */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvert.printf>                                         */
/*    ADD ESP. 4                                                        */
/* 一条形如 printf(str2); 的语句包括如下的指令。                        */
/*    MOV  EAX,  DWORD PTR SS: [EBP-C]                                  */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvert.printf>                                         */
/*    ADD ESP,  4                                                       */
/************************************************************************/
void primary_expression()
{
	// 这里的addr暂时没有用到。这个是用法是首先调用allocate_storage函数，从这个函数中获得。
	// 这个函数主要用于在空间不够的时候，分配节的空间，同时返回当前符号在节中的存储位置。
	// 这里的节是一个可执行程序中的概念。例如代码节，数据节，符号表节。
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
		// 整个操作分为三步：
		//   1.1. 生成一个字符串指针符号。
		iTokenCode = T_CHAR;
		typeExpression.typeCode = iTokenCode;
		mk_pointer(&typeExpression);
		typeExpression.typeCode |= T_ARRAY;
		// 字符串常量在.rdata节分配存储空间
        // sec = allocate_storage(&typeExpression, 
        //	      SC_GLOBAL, SEC_RDATA_STORAGE, 0, &addr);
		// var_sym_put(&typeExpression, SC_GLOBAL, 0, addr);

		// 字符串常量不应该是0 - TK_PLUS。而应该是TK_CSTR
		// 1.2. 调用allocate_storage分配空间。
		//      因为，typeExpression->ref->related_value = -1
		//      因此，会使用当前token计算数据长度。
        sec = allocate_storage(&typeExpression, 
        	SC_GLOBAL, SEC_RDATA_STORAGE, TK_CSTR, &addr);
		// 1.3. 调用var_sym_put把变量放入符号表。
		var_sym_put(&typeExpression, SC_GLOBAL, TK_CSTR, addr);
		// 1.4. 嵌套调用initializer完成变量初始化。
		//      因为前面分配空间的时候，是分配的全局空间，即SC_GLOBAL。
	    //      同时，allocate_storage的返回值sec不是空。
	    //      同时我们强制指定了T_ARRAY类型标志。
		//      我们会进入initializer函数的if部分。
	    //      把字符串memcpy到节的指定位置。
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
		// This can not move below . ex. printf("AAAA");
		get_token();
		// The problem of String at 07/11, We need the parse_string function
		if (token_code < TK_IDENT) 
		{
			// print_error("Need variable or constant\n");
			printf("Need variable or constant\n");
		}
		// 根据token_type查找符号。
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
		// 取出符号的寄存器存储类型
		storage_class = token_symbol->storage_class;
		operand_push(&token_symbol->typeSymbol, 
			storage_class, token_symbol->related_value);
        /* 如果取出来的符号是一个符号引用，而不是一个立即数。 */
        if (operand_stack_top->storage_class & SC_SYM) 
		{   
			// 操作数必须记录符号地址
			operand_stack_top->sym = token_symbol;      
			// 因为这个符号是一个符号引用，因此上，常量值无效。
            operand_stack_top->operand_value = 0;  
			// 用于函数调用，及全局变量引用 printf("g_cc=%c\n",g_cc);
        }
		break;
	}
}

/************************************************************************/
/* 功能：解析实参表达式表                                               */
/* <argument_expression_list> ::= <assignment_expression>               */
/*                     {<TK_COMMA><assignment_expression>}              */
/************************************************************************/
/* 函数定义如下：                                                       */
/*    int add(int x, int y)                                             */
/*    {                                                                 */
/*        return x+y;                                                   */
/*    }                                                                 */
/* 函数包括如下的指令。                                                 */
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
/* 其中1-3行为函数入口。4-6行执行x+y。7行执行return。8-10行为函数出口。 */
/* 一条形如 c = add(a, b); 的语句包括如下的指令。                       */
/*    MOV  EAX,  DWORD PTR SS: [EBP-8]  ; 把a取出来压栈。               */
/*    PUSH EAX                                                          */
/*    MOV  EAX,  DWORD PTR SS: [EBP-4]  ; 把b取出来压栈。               */
/*    PUSH EAX                                                          */
/*    CALL scc_anal.0040131B            ; 调用函数。                    */
/*    ADD  ESP, 8                       ; 恢复堆栈。                    */
/*                                      ; 因为有两个参数，这里写8。     */
/*    MOV DWORD PTR SS: [EBP-C], EAX    ; 把返回值取出来放到c里面。     */
/* 一条形如 printf("Hello"); 的语句包括如下的指令。                     */
/*    MOV  EAX, scc_anal.00403008; ASCII"Hello"                         */
/*    PUSH EAX                                                          */
/*    CALL <JMP.&msvcrt.printf>                                         */
/*    ADD ESP,  4                                                       */
/************************************************************************/
void argument_expression_list()
{
	Operand ret;
	Symbol *s, *sa;
	int nb_args = 0;
	s = operand_stack_top->typeOperand.ref;
	get_token();
	sa = s->nextSymbol;
	ret.typeOperand  = s->typeSymbol;
	ret.storage_class = REG_IRET;
	ret.operand_value = 0;
	if(get_current_token_type() != TK_CLOSEPA) 
	{
		for(;;)
		{
			assignment_expression();
			nb_args++;
			if (sa)
			{
				sa = sa->nextSymbol;
			}
			if(get_current_token_type() == TK_CLOSEPA)
				break;
			skip_token(TK_COMMA);
		}
	}
	if (sa)
	{
		print_error("实参个数少于函数形参个数", "");
	}
	skip_token(TK_CLOSEPA);
	gen_invoke(nb_args);
	operand_push(&ret.typeOperand, ret.storage_class, ret.operand_value);
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
		// SC_MEMBER不是e_StorageClass的一部分。而是e_TokenCode类型。
		// SC_MEMBER只不过是被放在e_StorageClass里面而已。
		struct_symbol = sym_push(token_code | SC_MEMBER, &typeOne, 0, *offset);
		*offset += size;
		**ps = struct_symbol;
		*ps = &struct_symbol->nextSymbol;
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
		print_error("Has defined", get_current_token());
	}
	maxalign = 1;
	symPtr = &sym->nextSymbol;
	offset = 0 ;
	// end of Adding Symbol operation
	while (get_current_token_type() != TK_END)  // } 右大括号
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
		print_error("Need struct_name\n", get_current_token());
	}
	// Adding Symbol operation
	sym = struct_search(token_code);
	if (!sym)
	{
		typeOne.typeCode = T_STRUCT;
		sym = sym_push(token_code | SC_STRUCT, &typeOne, 0, -1);
		sym->storage_class = 0;
	}
	typeStruct->typeCode = T_STRUCT;
	typeStruct->ref      = sym;
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
	e_TypeCode iTypeCode;
	int type_found = 0 ;
	Type typeStruct;
	iTypeCode = T_INT ;
	
	switch(get_current_token_type()) {
	case KW_CHAR:    // All of simple_type
		iTypeCode = T_CHAR;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_SHORT:
		iTypeCode = T_SHORT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_VOID:
		iTypeCode = T_VOID;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_INT:
		iTypeCode = T_INT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_STRUCT:
		iTypeCode = T_STRUCT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		struct_specifier(&typeStruct);
		typeSpec->ref = typeStruct.ref;
		break;
	default:
		break;
	}
	typeSpec->typeCode = iTypeCode;
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
	// v:	符号编号。类型为e_TokenCode。这里改为token_code。
	// int v = -1; // , has_init, addr;
	int token_code = -1; 
	
	// 这里的addr暂时没有用到。这个是用法是首先调用allocate_storage函数，从这个函数中获得。
	// 这个函数主要用于在空间不够的时候，分配节的空间，同时返回当前符号在节中的存储位置。
	// 这里的节是一个可执行程序中的概念。例如代码节，数据节，符号表节。
	// int has_init, r, addr = 0;
	e_Sec_Storage sec_area;
	int storage_class, addr = 0;
	Symbol * sym;
	Section * sec = NULL;
	print_all_stack("Starting external_declaration");
	// 1. 读取声明的类型。这里的类型是第一个token。
	//    这个类型可以理解为基础类型，因为如果后面跟了*就会变成指针类型。
	if (!type_specifier(&bTypeCurrent))
	{
		print_error("Need type token\n", get_current_token());
	}
	// 2. 如果前面type_specifier有处理结构体，这时bTypeCurrent.t就会等于T_STRUCT。
	//    这说明这一次，我们处理完了一个外部声明。我们就返回。
	if (bTypeCurrent.typeCode == T_STRUCT && get_current_token_type() == TK_SEMICOLON)
	{
		print_all_stack("End external_declaration");
		get_token();
		return;
	}
	// 2. 代码执行到这里，说明我们已经处理完成了所有的结构体声明。
	//    下面处理混合排列的函数声明，函数定义和全局变量声明。
	while (1)
	{
		typeCurrent = bTypeCurrent;
		// 3. 处理声明前缀。如果发现*，typeCurrent就会变成指针类型。
		declarator(&typeCurrent, &token_code, NULL);
		// 4. 如果是函数定义
		if (get_current_token_type() == TK_BEGIN)
		{
			// 函数不能是本地的。只能是全局的。
			if (iSaveType == SC_LOCAL)
			{
				print_error("Not nesting function\n", get_current_token());
			}
			// 声明后面跟着括号。必须是函数。否则就报错。
			if((typeCurrent.typeCode & T_BTYPE) != T_FUNC)
			{
				print_error("Needing function defination\n", get_current_token());
			}
			// 5. 如果查找到了这个符号。说明前面进行了函数定义。
			sym = sym_search(token_code);
			if(sym)
			{
				// 如果发现前面定义的不是函数定义。而是一个变量定义，就报错退出。
				if((sym->typeSymbol.typeCode & T_BTYPE) != T_FUNC)
				{
					char tmpStr[128];
					sprintf(tmpStr, "Function %s redefination\n", get_current_token());
					print_error(tmpStr, get_current_token());
				}
				sym->typeSymbol = typeCurrent;
			}
			// 6. 如果没有找到，就直接添加。
			else
			{
				sym = func_sym_push(token_code, &typeCurrent);
			}
			// 全局普通符号。
			sym->storage_class = SC_SYM | SC_GLOBAL;
			print_all_stack("Enter funtion body");
			// 7. 处理函数体。
			func_body(sym);
			break;
		}
		else
		{
			// 4. 函数声明
			if((typeCurrent.typeCode & T_BTYPE) == T_FUNC)
			{
				if (sym_search(token_code) == NULL)
				{
					sym = sym_push(token_code, &typeCurrent, SC_GLOBAL | SC_SYM, 0);
				}
			}
			// 4. 变量声明
			else
			{
			    storage_class = 0;
				// 如果是数组。就只能是左值。
				if(!(typeCurrent.typeCode & T_ARRAY))
				{
					storage_class |= SC_LVAL;
				}
				storage_class |= iSaveType;
				// 如果变量声明的时候同时做了赋初值。执行赋初值操作。
				// if (get_current_token_type() == TK_ASSIGN)  // int a = 5 ;
				if (get_current_token_type() == TK_ASSIGN)
				{
					sec_area = SEC_DATA_STORAGE;
				}
				else 
				{
					sec_area = SEC_BSS_STORAGE;
				}
				// 这里的这句if(has_init)其实可以等价于：
				//     if (get_current_token_type() == TK_ASSIGN)
				// 但是这个判断用的太多了。于是这里提出来。
				// 同时，这个判断是不受get_token影响的。
				// 5. 如果是等号。说明是赋值操作。需要再读一个符号，把右值也读进来。
				if(sec_area == SEC_DATA_STORAGE)
				{
					// 取得等号后面的右值。
					get_token(); //不能放到后面，
				//	initializer(&typeCurrent);
				}
				// 这里主要做了3项工作：
				//   (1) 为变量分配存储空间，局部变量存放在栈中，
				//       全局变量分两类，声明时进行赋值的存放在.data数据节中，
				//       声明时不进行赋值的存放在.bss节中。
				// 代码执行到这里的时候，如果存在赋值，我们已经取到了等号后面的右值。
				// 之所以要这样做，是因为我们需要处理char str[]="abc"的情况，
				// 通过allocate_storage求字符串长度。

				// 6. 首先为符号分配空间，
			    sec = allocate_storage(&typeCurrent, storage_class,
			    							sec_area, token_code, &addr);
				// 7. 之后添加符号。
				sym = var_sym_put(&typeCurrent, storage_class, token_code, addr);
				//   (2) 将全局变量放入COFF符号表。
			    if (iSaveType == SC_GLOBAL)
			    {
				    coffsym_add_update(sym, addr, sec->cSectionIndex, 
						CST_NOTFUNC, IMAGE_SYM_CLASS_EXTERNAL);
			    }
				//   (3) 对声明时进行赋值的变量进行初始化。
				//       如果上一个token是等号，使用当前token进行赋值。
			    if (sec_area == SEC_DATA_STORAGE)  // int a = 5 ;
			    {
					// 8. 赋值操作。
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
/* typeCurrent：  变量类型                                              */
/* storage_class：变量存储类型                                          */
/* sec_area：     根据是否是字符串常量和是否需要进行初始化。            */
/*                控制在哪一个节分配存储空间。                          */
/* token_code：   变量符号编号。类型为e_TokenCode。                     */
/*                因为小于TK_IDENT是保留值。                            */
/*                因此上，该值通常大于TK_IDENT。                        */
/*         注意： 对于常量字符串的时候，我们传入了TK_CSTR。             */
/*                虽然这里只需要传入一个小于TK_IDENT的保留值。          */
/*                甚至之前的代码传入了零。                              */
/*                但是为了便于理解，我们修改为传入TK_CSTR。             */
/*                这个值在var_sym_put的时候，会用来调用sym_search。
                               */
/* addr(输出) ：  变量存储偏移量地址。                                  */
/*                对于全局变量和字符串常量返回节内偏移量。              */
/*                对于局部变量返回栈内偏移量。                          */
/* 返回值：       变量存储节。                                          */
/*                对于全局变量和字符串常量返回对应节的地址。更新偏移量。*/
/*                对于局部变量返回NULL。                                */
/************************************************************************/
/* 函数逻辑如下：                                                       */
/*   首先我们调用type_size计算需要分配的存储空间。                      */
/*   这里有一个技巧。就是：                                             */
/*       如果是char * str1，他的长度是不固定的。这种情况下返回-1。      */
/*       能够返回-1的原因是typeCurrent->related_value = -1。            */
/*   如果type_size返回-1.就会使用前面读到的右值计算数据长度。           */
/*   之后我们就可以利用前面计算出来的长度分配空间了。                   */
/*   如果是全局变量。就分配在节上。如果是局部变量，就分配在栈上。       */
/************************************************************************/
Section * allocate_storage(Type * typeCurrent, int storage_class, 
						   e_Sec_Storage sec_area, int token_code, int *addr)
{
	int size, align;
	Section * sec= NULL;
	// 调用type_size计算需要分配的存储空间，
	size = type_size(typeCurrent, &align);
	// type_size返回-1说明是不定长指针数组。
	if (size < 0)
	{
		// 使用前面读到的右值计算数据长度。
		if (typeCurrent->typeCode & T_ARRAY 
			&& typeCurrent->ref->typeSymbol.typeCode == T_CHAR)
		{
			typeCurrent->ref->related_value = strlen((char *)get_current_token()) + 1;
			size = type_size(typeCurrent, &align);
		}
		else
			print_error("Unknown size of type", "");
	}

	// 局部变量在栈中分配存储空间
	if ((storage_class & SC_VALMASK) == SC_LOCAL)
	{
		function_stack_loc = calc_align(function_stack_loc - size, align);
		*addr = function_stack_loc;
	}
	else
	{
		// 初始化的全局变量在.data节分配存储空间
		// 通过判断(get_current_token_type() == TK_ASSIGN)得到。
		if (sec_area == SEC_DATA_STORAGE)
		{
			sec = sec_data;
		}
		// 字符串常量在.rdata节分配存储空间
		// 代码位于primary_expression的TK_CSTR分支。
		else if (sec_area == SEC_RDATA_STORAGE)	
		{
			sec = sec_rdata;
		}
		// 未初始化的全局变量在.bss节分配存储空间
		// 通过判断(get_current_token_type() == TK_ASSIGN)得到。
		// (has_init == SEC_BSS_STORAGE)
		else if (sec_area == SEC_BSS_STORAGE)	
		{
			sec = sec_bss;
		}
		else
		{
			print_error("Illegal section definition", "");
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
		// 在.rdata节分配存储空间的常量字符串。
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

