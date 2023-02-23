#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include "rjson.h"
#include "rclibs.h"

void rjson_parser_init(struct rjson_parser *parser, wchar_t *filename, char *text, int len);
void rjson_parser_release(struct rjson_parser *parser);
void rjson_parser_read(struct rjson_parser *parser);

int main(int argc, char** argv)
{
	setlocale(LC_CTYPE, "");
	if (argc < 2) {
		printf("json file required\n");
		return 0;
	}
	char *filename = argv[1];
	int wlen = mbstowcs(NULL, filename, 0);
	VLADecl(wchar_t, wcsfile, wlen + 1);
	wcsfile[wlen] = 0;
	mbstowcs(wcsfile, filename, wlen);
#ifdef _MSC_VER
	FILE *file = _wfopen(wcsfile, "rb");
#else
	FILE *file = fopen(filename, "rb");
#endif
	if (file == NULL) {
		fprintf(stderr, "File not found : %ls\n", wcsfile);
		exit(-1);
	}
	fseek(file, 0, SEEK_END);
	int len = ftell(file);
	fseek(file, 0, SEEK_SET);
	// read utf8
	char *text = malloc(len + 1);
	text[len] = 0;
	fread(text, sizeof(char), len, file);
	fclose(file);

	struct rjson_parser parser;
	rjson_parser_init(&parser, wcsfile, text, len);

	rjson_parser_read(&parser);

	rjson_print(&parser.json, 0, stdout);

	rjson_parser_release(&parser);

	return 0;
}
