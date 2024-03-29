/* ���ʱ��� */
enum e_TokenCode
{  
	/* ��������ָ��� */
	TK_PLUS,		// + �Ӻ�
    TK_MINUS,		// - ����
    TK_STAR,		// * �Ǻ�
    TK_DIVIDE,		// / ����
    TK_MOD,			// % ���������
    TK_EQ,			// == ���ں�
    TK_NEQ,			// != �����ں�
    TK_LT,			// < С�ں�
    TK_LEQ,			// <= С�ڵ��ں�
    TK_GT,			// > ���ں�
    TK_GEQ,			// >= ���ڵ��ں�
    TK_ASSIGN,		// = ��ֵ����� 
    TK_POINTSTO,	// -> ָ��ṹ���Ա�����
    TK_DOT,			// . �ṹ���Ա�����
	TK_AND,         // & ��ַ�������
	TK_OPENPA,		// ( ��Բ����
	TK_CLOSEPA,		// ) ��Բ����
	TK_OPENBR,		// [ ��������
	TK_CLOSEBR,		// ] ��Բ����
	TK_BEGIN,		// { �������
	TK_END,			// } �Ҵ�����
    TK_SEMICOLON,	// ; �ֺ�    
    TK_COMMA,		// , ����
	TK_ELLIPSIS,	// ... ʡ�Ժ�
	TK_EOF,			// �ļ�������

    /* ���� */
    TK_CINT,		// ���ͳ���
    TK_CCHAR,		// �ַ�����
    TK_CSTR,		// �ַ�������

	/* �ؼ��� */
	KW_CHAR,		// char�ؼ���
	KW_SHORT,		// short�ؼ���
	KW_INT,			// int�ؼ���
    KW_VOID,		// void�ؼ���  
    KW_STRUCT,		// struct�ؼ���   
	KW_IF,			// if�ؼ���
	KW_ELSE,		// else�ؼ���
	KW_FOR,			// for�ؼ���
	KW_WHILE,       // while�ؼ���
	KW_CONTINUE,	// continue�ؼ���
    KW_BREAK,		// break�ؼ���   
    KW_RETURN,		// return�ؼ���
    KW_SIZEOF,		// sizeof�ؼ���

    KW_ALIGN,		// __align�ؼ���	
    KW_CDECL,		// __cdecl�ؼ��� standard c call
	KW_STDCALL,     // __stdcall�ؼ��� pascal c call
	
	/* ��ʶ�� */
	TK_IDENT,
	/* ע�� */
	TK_COMMENT,
	/* ����� */
	TK_INCLUDE,
    TK_COMMON,
    TK_TOKENCODE_END
};

enum{
	LEX_NORMAL,
	LEX_SEP
};

