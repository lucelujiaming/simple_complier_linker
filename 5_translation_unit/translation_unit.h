#ifndef TRANSLATION_UNIT_H
#define TRANSLATION_UNIT_H

/*******************************grammar.h begin****************************/
/* 语法状态 */
enum e_SynTaxState
{
	SNTX_NUL,       // 空状态，没有语法缩进动作
	SNTX_SP,		// 空格 int a; int __stdcall MessageBoxA(); return 1;
	SNTX_LF_HT,		// 换行并缩进，每一个声明、函数定义、语句结束都要置为此状态
	SNTX_DELAY      // 延迟取出下一单词后确定输出格式，取出下一个单词后，根据单词类型单独调用syntax_indent确定格式进行输出 
};

/* 存储类型 */
enum e_StorageClass
{
	SC_GLOBAL =   0x00f0,		// 包括：包括整型常量，字符常量、字符串常量,全局变量,函数定义          
	SC_LOCAL  =   0x00f1,		// 栈中变量
	SC_LLOCAL =   0x00f2,       // 寄存器溢出存放栈中
	SC_CMP    =   0x00f3,       // 使用标志寄存器
	SC_VALMASK=   0x00ff,       // 存储类型掩码             
	SC_LVAL   =   0x0100,       // 左值       
	SC_SYM    =   0x0200,       // 符号	

	SC_ANOM	  = 0x10000000,     // 匿名符号
	SC_STRUCT = 0x20000000,     // 结构体符号
	SC_MEMBER = 0x40000000,     // 结构成员变量
	SC_PARAMS = 0x80000000,     // 函数参数
};

/* 类型编码 */
enum e_TypeCode
{
	T_INT    =  0,			// 整型                     
	T_CHAR   =  1,			// 字符型                 
	T_SHORT  =  2,			// 短整型                       
	T_VOID   =  3,			// 空类型                        
	T_PTR    =  4,			// 指针                          
	T_FUNC   =  5,			// 函数                    
	T_STRUCT =  6,			// 结构体 
	
	T_BTYPE  =  0x000f,		// 基本类型掩码          
	T_ARRAY  =  0x0010,		// 数组
};

#endif