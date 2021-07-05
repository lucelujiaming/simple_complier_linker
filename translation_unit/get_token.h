#ifndef GET_TOKEN_H
#define GET_TOKEN_H

#include "token_code.h"

void token_cleanup();
void token_init(char * strFilename);
int get_token();
void color_token(int lex_state, int tokenType, char * printStr);

void push_token(char * strNewToken);

char * get_current_token();
int get_current_token_type();
void skip_token(int c);
#endif