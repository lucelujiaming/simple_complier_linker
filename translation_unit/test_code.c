int main(int argc, char* argv[]){	program_buffer = (char *)malloc(40960);	load_program(program_buffer, argv[1]);	init();
	do{	get_token();if(token_type == TK_CSTR)	tktable.push_back(std::string(token));
		color_token(LEX_NORMAL, token_type, token);} while(token_type != TK_EOF);
	cleanup();	printf("Success !\n");	return 1;}