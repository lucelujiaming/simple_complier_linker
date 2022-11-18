#pragma warning(disable : 4786)
#include "get_token.h"
#include "symbol_table.h"
#include <windows.h>
#include<ctype.h>
#include <vector>
#include <string>

#define PROG_SIZE 40960

#define DELIM_TOKENS  " ;,+-*^/%=()><[]{}.!\"'\\|"  // Not include #

enum string_output_status{
	STRING_OUTPUT_NONE,
	STRING_OUTPUT_START,
	STRING_OUTPUT_END
};

std::vector<std::string> tk_hashtable;
//std::vector<std::string> tktable;
// Change it in this version until we know the meaning of Symbol
std::vector<TkWord> tktable;
char token[256];
std::string source_str; 

string_output_status string_output_status_indicator = STRING_OUTPUT_NONE;

char * program_buffer ;

int token_type;
int lineNum;

struct commands { /* keyword lookup table */
  char command[20];
  e_TokenCode tok;
};

static struct commands token_table[] = {
	/* ��������ָ��� */
	"+",        TK_PLUS,		// + �Ӻ�
    "-",        TK_MINUS,		// - ����
    "*",        TK_STAR,		// * �Ǻ�
    "/",        TK_DIVIDE,		// / ����
    "%",        TK_MOD,			// % ���������
    "==",       TK_EQ,			// == ���ں�
    "!=",       TK_NEQ,			// != �����ں�
    "<",        TK_LT,			// < С�ں�
    "<=",       TK_LEQ,			// <= С�ڵ��ں�
    ">",        TK_GT,			// > ���ں�
    ">=",       TK_GEQ,			// >= ���ڵ��ں�
    "=",        TK_ASSIGN,		// = ��ֵ����� 
    "->",       TK_POINTSTO,	// -> ָ��ṹ���Ա�����
    ".",        TK_DOT,			// . �ṹ���Ա�����
	"&",        TK_AND,         // & ��ַ�������
	"(",        TK_OPENPA,		// ( ��Բ����
	")",        TK_CLOSEPA,		// ) ��Բ����
	"[",        TK_OPENBR,		// [ ��������
	"]",        TK_CLOSEBR,		// ] ��Բ����
	"{",        TK_BEGIN,		// { �������
	"}",        TK_END,			// } �Ҵ�����
    ";",        TK_SEMICOLON,	// ; �ֺ�    
    ",",        TK_COMMA,		// , ����
	"...",      TK_ELLIPSIS,	// ... ʡ�Ժ�
	// TK_EOF,			// �ļ�������

    /* ���� */
    // TK_CINT,		// ���ͳ���
    // TK_CCHAR,		// �ַ�����
    // TK_CSTR,		// �ַ�������

	/* �ؼ��� */
	"char",          KW_CHAR,		// char�ؼ���
	"short",         KW_SHORT,		// short�ؼ���
	"int",           KW_INT,		// int�ؼ���
    "void",          KW_VOID,		// void�ؼ���  
    "struct",        KW_STRUCT,		// struct�ؼ���   
	"if",            KW_IF,			// if�ؼ���
	"else",          KW_ELSE,		// else�ؼ���
	"for",           KW_FOR,		// for�ؼ���
	"while",         KW_WHILE,		// while�ؼ���
	"continue",      KW_CONTINUE,	// continue�ؼ���
    "break",         KW_BREAK,		// break�ؼ���   
    "return",        KW_RETURN,		// return�ؼ���
    "sizeof",        KW_SIZEOF,		// sizeof�ؼ���

    "__align",       KW_ALIGN,		// __align�ؼ���	
    "__cdecl",       KW_CDECL,		// __cdecl�ؼ��� standard c call
	"__stdcall",     KW_STDCALL,    // __stdcall�ؼ��� pascal c call
	"include",       TK_INCLUDE,
    "", TK_TOKENCODE_END  /* mark end of table */
	/* ��ʶ�� */
	// TK_IDENT
};

void find_eol();
void output_comment_eol();
void color_single_char(int lex_state, int tokenType, char printChar);
void color_token(int lex_state, int tokenType, char * printStr);

void string_output_status()
{
	if(string_output_status_indicator == STRING_OUTPUT_NONE)
		string_output_status_indicator = STRING_OUTPUT_START;
	else if(string_output_status_indicator == STRING_OUTPUT_START)
		string_output_status_indicator = STRING_OUTPUT_END;
	else // STRING_OUTPUT_END
		string_output_status_indicator = STRING_OUTPUT_NONE;
}

void string_output_status_tailoperation()
{
	if(string_output_status_indicator == STRING_OUTPUT_END)
		string_output_status_indicator = STRING_OUTPUT_NONE;
}

int load_program(char *p, char *pname)
{
  FILE *fp = 0 ;
  int i=0;
  
  if(!(fp=fopen(pname, "r"))){
      printf("load_program failed : %s\n", pname);
      return 0;
  }

  do {
    *p = getc(fp);
    p++; i++;
  }while(!feof(fp) && i<PROG_SIZE);
  *(p-2) = '\0'; /* null terminate the program */
  fclose(fp);
  return 1;
}


void parse_ctype_comment()
{
	do {
		// read until meet <ENTER> / <STAR> / <END>
		do{
			if(program_buffer[0] == '\n' || program_buffer[0] == '*'
				|| program_buffer[0] == '\0')
				break;
			else
			{
				color_single_char(LEX_NORMAL, TK_COMMENT, *program_buffer);
				program_buffer++;
			}
		} while (1);

		if(program_buffer[0] == '\n')
		{
			color_single_char(LEX_NORMAL, TK_COMMENT, *program_buffer);
			lineNum++;
			program_buffer++;
		}
		else if(program_buffer[0] == '*')
		{
			color_single_char(LEX_NORMAL, TK_COMMENT, *program_buffer);
			program_buffer++;
			if (program_buffer[0] == '/')
			{
				color_single_char(LEX_NORMAL, TK_COMMENT, *program_buffer);
				program_buffer++;
				return;
			}
		}
		else {
			printf("No */");
			return;
		}
	} while (1);
}

int isdelim(char c)
{       
  // if(strchr(" ;,+-<>/*%^=()[]", c) || c==9 || c=='\r' || c=='\n' || c==0)
  if(strchr(DELIM_TOKENS, c) || c==9 || c=='\r' || c=='\n' || c==0)
    return 1;
  return 0;
}

int iswhite(char c)
{
  if(c==' ' || c=='\t') return 1;
  else return 0;
}

void preprocess()
{
	register char *temp;
	
	token_type=0;
	temp = token;

  if(*(program_buffer)=='\0') { /* end of file */
    *token=0;
    token_type = TK_EOF;
  }

  while(1)
  {
	  if(iswhite(*(program_buffer))){
		color_single_char(LEX_NORMAL, TK_COMMON, *program_buffer);
  		++program_buffer;  /* skip over white space */
	  }
	  else if(*(program_buffer) == '\r' 
		  &&*(program_buffer + 1) == '\n'){
		  color_token(LEX_NORMAL, TK_COMMON, "\r\n");
		  program_buffer += 2;   // Jump over \r\n
	  }
	  else if(*(program_buffer) == '\n'){
		  color_token(LEX_NORMAL, TK_COMMON, "\n");
		  program_buffer += 1;   // Jump over \r\n
	  }
	  else if(*(program_buffer) == '/' 
		  &&*(program_buffer + 1) == '/'){
		  // use logic in the find_eol 
		  output_comment_eol();
	  }
	  else if(*(program_buffer) == '/' 
		  &&*(program_buffer + 1) == '*'){
		  color_token(LEX_NORMAL, TK_COMMENT, "/*");
		  program_buffer += 2;
		  parse_ctype_comment();
	  }
	  else
		  break;
  }
}

int look_up(char *s)
{
  register int i; // ,j;
  char *p;

  /* convert to lowercase */
  p = s;
  while(*p){ *p = tolower(*p); p++; }

  /* see if token is in table */
  for(i=0; *token_table[i].command; i++)
      if(!strcmp(token_table[i].command, s)) return token_table[i].tok;
  return -1; /* unknown command */
}

void find_eol()
{
  while(*program_buffer!='\n'
  	&& *program_buffer!='\0')
  {
	  ++program_buffer;
  }
  if(*program_buffer)
  {
  	  program_buffer++;
  }
}

void output_comment_eol()
{
  while(*program_buffer!='\n'
  	&& *program_buffer!='\0')
  {
      color_single_char(LEX_NORMAL, TK_COMMENT, *program_buffer);
	  ++program_buffer;
  }
  if(*program_buffer)
  {
      color_single_char(LEX_NORMAL, TK_COMMENT, *program_buffer);
  	  program_buffer++;
  }
}

void parse_string(char sep)
{
	register char *temp;
	char c;
	temp=token;

	*temp++ = *(program_buffer)++;
	for (;;)
	{
		if(program_buffer[0] == sep)
		{
			*temp++ = program_buffer[0];
			program_buffer++;
			break;
		}
		else if (program_buffer[0] == '\\')
		{
			*temp++ = program_buffer[0];
			program_buffer++;
			switch(program_buffer[0]) {
			case '0':
				c = '\0';
				break;
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 't':
				c = '\t';
				break;
			case 'n':
				c = '\n';
				break;
			case 'v':
				c = '\v';
				break;
			case 'f':
				c = '\f';
				break;
			case 'r':
				c = '\r';
				break;
			case '\"':
				c = '\"';
				break;
			case '\'':
				c = '\'';
				break;
			case '\\':
				c = '\\';
				break;
			default:
				c = program_buffer[0];
				if (c >= '!' && c <= '~')
				{
					printf("Illegal char");
				}
			}
			// *temp++ = c;
			*temp++ = program_buffer[0];
			program_buffer++;
		}
		else
		{
			*temp++ = *(program_buffer)++;
		}
	}
}

/* Get a token. */
int get_token()
{	
	register char *temp;

	token_type=0; 
	temp=token;
	memset(token, 0x00, 256);
	preprocess();

	if(*program_buffer == '\0')
	{
		return (token_type = TK_EOF);
	}
	
	if(*program_buffer=='#') {
		// find_eol();
		*temp++=*(program_buffer)++;
		// token[0]='\r'; token[1]='\n'; token[2]=0;
		// return (objThreadCntrolBlock->token_type = DELIMITER);
		syntax_indent();
		return (token_type = TK_INCLUDE);
	}

	if(strchr(DELIM_TOKENS, *program_buffer)){ /* delimiter */
  		if(strchr("<>=", *program_buffer)) {
			switch(*program_buffer) {
				case '<':
					if(*(program_buffer+1) == '=') {
						*temp++=*(program_buffer)++;
						*temp++=*(program_buffer)++;   /* advance to next position */
					//	*temp = LE; temp++; *temp = LE;
					}
					else if(*(program_buffer+1) == '<') {
						*temp++=*(program_buffer)++;
						*temp++=*(program_buffer)++;   /* advance to next position */
						// *temp = NE; temp++; *temp = NE;
					}
					else {
						*temp++=*(program_buffer)++;   /* advance to next position */
					//	*temp = LT;
					}
					break;
				case '>':
					if(*(program_buffer+1) == '=') {
						*temp++=*(program_buffer)++;
						*temp++=*(program_buffer)++;
						// *temp = GE; temp++; *temp = GE;
					}
					else {
						*temp++=*(program_buffer)++;
						// *temp = GT;
					}
					break;
				case '=':
					if(*(program_buffer+1) == '=') {
						*temp++=*(program_buffer)++;
						*temp++=*(program_buffer)++;
						// *temp = EQ; temp++; *temp = EQ;
					}
					else {
						*temp++=*(program_buffer)++;
						// *temp = EQ;
					}
					break;
			}
		}
		else if (*program_buffer == '\"' || 
			*program_buffer == '\'')
		{
			string_output_status();
			*temp=*program_buffer;
			parse_string(program_buffer[0]);   // End char is same with the start char.
			syntax_indent();
			return (token_type = TK_CSTR);
		}
		else
		{
			*temp=*program_buffer;
			program_buffer++; /* advance to next position */
			token_type = look_up(token); /* convert to internal rep */
			if(token_type == -1)
			{
				syntax_indent();
				return token_type;
			}
		}
		// lookup the token type by temp 2021/0627
		temp++;
		*temp=0;
		token_type = look_up(token); /* convert to internal rep */
		if(token_type == -1)
		{
			token_type=TK_IDENT;
		}
		tkword_insert(token);
		syntax_indent();
		return token_type;
	}

	
  if(isdigit(*(program_buffer))) { /* number */
    while(!isdelim(*(program_buffer))) *temp++=*(program_buffer)++;
    *temp = '\0';
	syntax_indent();
    return(token_type = TK_CINT);
  }

  if(isalpha(*(program_buffer))) { /* var or command */
    while(!isdelim(*(program_buffer))) 
		*temp++=*(program_buffer)++;
    token_type=TK_IDENT;
  }

  *temp = '\0';

  /* see if a string is a command or a variable */
  if(token_type==TK_IDENT) {
    token_type = look_up(token); /* convert to internal rep */
	if(token_type == -1)
	{
		token_type=TK_IDENT;
	}
	tkword_insert(token);
  }
  
  syntax_indent();
  return token_type;
}

void skip_token(int c)
{
	if (get_current_token_type() != c)
	{
		printf("Missing %d\n", c);
	}
	get_token();
}

void color_token(int lex_state, int tokenType, char * printStr)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
//	char * p;
	switch(lex_state) {
	case LEX_NORMAL:
		// Always reset ConsoleTextAttribute
		SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY);
		if (tokenType >= TK_IDENT)
		{
			if(tokenType == TK_INCLUDE)
				SetConsoleTextAttribute(handle, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			else if(tokenType == TK_COMMENT)
				SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
			else 
				SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY);
		}
		else if (tokenType >= KW_CHAR)
		{
			SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}
		else if (tokenType >= TK_CINT)
		{
			SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_RED);
		}
		else if (tokenType >= TK_CINT)
		{
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		}

		if(string_output_status_indicator != STRING_OUTPUT_NONE)
		{
			SetConsoleTextAttribute(handle, FOREGROUND_BLUE | FOREGROUND_RED);
			printf("%s", printStr);
			string_output_status_indicator = STRING_OUTPUT_NONE;
		}
		else
		{
			printf("%s", printStr);
		}
		break;
	case LEX_SEP:
		printf("%c", *printStr);
		break;
	default:
		break;
	}
}

void color_single_char(int lex_state, int tokenType, char printChar)
{
	char cTemp[4];
	memset(cTemp, 0x00, 4);
	sprintf(cTemp, "%c", printChar);
	color_token(lex_state, tokenType, cTemp);
}

void token_init(char * strFilename)
{
	program_buffer = (char *)malloc(40960);
	load_program(program_buffer, strFilename);

	lineNum = 1;
	// init_lex();
}

void token_cleanup()
{
	int i = 0 ;
//	printf("\ntoken table has %d tokens", tktable.size());
//	for(i = 0; i < tktable.size(); i++)
//	{
//		printf("tktable[%d] = %s \n", i, tktable[i].c_str());
//	}
//	free(tktable.data);
}

TkWord* tkword_insert(char * strNewToken) // , e_TokenCode tokenCode)
{
	int length;
//	if(token_type == TK_CSTR)
//			tktable.push_back(std::string(strNewToken));
	TkWord tkWord;
	printf("\n -- tkword_insert -- ");
	for (int i = 42; i < tktable.size(); i++)
	{
		printf(" (%s, %08X, %08X) ", tktable[i].spelling, tktable[i].sym_struct, tktable[i].sym_identifier);
	}
	printf(" ---- \n");
	for (i = 0; i < tktable.size(); i++)
	{
		// printf("tktable[%d].spelling = %s \n", i, tktable[i].spelling);
		if (!strcmp(tktable[i].spelling,strNewToken))
		{
			return &tktable[i];
		}
	}
	
	length = strlen(strNewToken);
	tkWord.spelling = (char *)malloc(length + 1); 
	strcpy(tkWord.spelling, strNewToken);
	tkWord.tkcode = tktable.size() - 1;
	tkWord.sym_identifier = NULL;
	tkWord.sym_struct = NULL;
	
	// printf("tkword_insert tktable.count = %d \n", tktable.size());
	// tkWord.tkcode = tokenCode;
	tktable.push_back(tkWord);
	return tktable.end() - 1;
}

char * get_current_token()
{
	return token;
}

int get_current_token_type()
{
	return token_type;
}

void set_current_token_type(int iTokenType)
{
	token_type = iTokenType;
}