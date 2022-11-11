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
void skip_token(int c);

void syntax_indent();
void translation_unit();

#endif