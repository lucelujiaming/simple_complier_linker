#ifndef GET_TOKEN_H
#define GET_TOKEN_H

#include "token_code.h"
#include "symbol_table.h"
#include "translation_unit.h"
#include "coff_generator.h"

void token_cleanup();
void token_init(char * strFilename);
int get_token();
void color_token(int lex_state, int tokenType, char * printStr);

struct TkWord* tkword_insert(char * strNewToken); // , e_TokenCode tokenCode);

char * get_current_token();
int get_current_token_type();
void set_current_token_type(int iTokenType);
void skip_token(int cTokenCode);

void syntax_indent();
void translation_unit();
void print_error(char * strErrInfo, char * subject_str);
#endif