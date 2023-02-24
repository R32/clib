#include "rjson.h"

enum token {
	Eof = 0,
	CNull,    // null
	CTrue,
	CFalse,
	CFloat,
	CString,  // "strinig"
	Comma,    // ,
	DblDot,   // :
	LBrace,   // {
	RBrace,   // }
	LBracket, // [
	RBracket, // ]
	OpSub,    // -
};

#define lto_parser(ptr)      container_of(ptr, struct rjson_parser, lex)
#define lto_buffer(lex)      (&lto_parser(lex)->json.buffer)
#define lto_crlfcnt(lex)     (&lto_parser(lex)->crlfcnt)

#define lpmin(lex)           ((lex)->pos.min)
#define lpmax(lex)           ((lex)->pos.max)

#define buffer_reset(p)      wcsbuf_reset(&(p)->json.buffer)

int copy_lexchars(struct rlex *lex, int pos, int len, wchar_t *out, int outlen)
{
	LEXCHAR *source = ((LEXCHAR *)lex->src) + pos;
#if LEXCHAR_UCS2
	if (out == NULL)
		return len;
	wmemcpy(out, source, len);
	out[len] = 0;
	return len;
#else
	if (out == NULL)
		return utf8towcs(NULL, source, len);
	utf8towcs(out, source, len);
	out[outlen] = 0;
	return outlen;
#endif
}

static void copy_lexchars_to_buffer(struct rlex *lex, int pos, int len, struct wcsbuf *buffer)
{
	LEXCHAR *source = ((LEXCHAR *)lex->src) + pos;
#if LEXCHAR_UCS2
	wcsbuf_append_string(buffer, source, len);
#else
	int dstlen = utf8towcs(NULL, source, len);
	if (len < 0)
		dstlen--;  // without '\0'
	VLADecl(wchar_t, dest, dstlen + 1);
	utf8towcs(dest, source, len);
	wcsbuf_append_string(buffer, dest, dstlen);
#endif
}

%% // lexer starts,

%EOF(Eof)
%TOKEN(token)

let integer = "0" | "[1-9][0-9]*"

let floatpoint = ".[0-9]+" | "[0-9]+.[0-9]*"

let exp = "[eE][+-]?[0-9]+"

let float = integer + Opt(exp) | floatpoint + Opt(exp)

let crlf = "\r?\n"

let token = function
| crlf ->
	crlf_add(lto_crlfcnt(lex), lpmax(lex));
	token()
| "[ \t]+" ->    // spaces
	token()
| "//[^\n]*" ->  // line comment
	token()
| "-" -> OpSub
| "[" -> LBracket
| "]" -> RBracket
| "{" -> LBrace
| "}" -> RBrace
| "," -> Comma
| ":" -> DblDot
| float -> CFloat
| "null" -> CNull
| "true" -> CTrue
| "false" -> CFalse
| "/\\*" ->
	blkcomment();
	token()
| '"' ->
	struct rjson_parser *parser = lto_parser(lex);
	buffer_reset(parser);
	int min = lpmin(lex);
	enum token tok = tstring();
	if (tok == Eof) {
		fprintf(stderr, "UnClosed String: %d-%d", min, lpmax(lex));
		exit(-1);
	}
	lpmin(lex) = min;
	rj_wchars wcs = rj_wchars_flush(&parser->json, NULL);
	rarray_push(&parser->parray, &((struct pos_wchars){.pos = min, .wcs = wcs}));
	tok

| _ ->
	struct rjson_parser *parser = lto_parser(lex);
	struct lncolumn lcn = crlf_get(&parser->crlfcnt, lpmax(lex));
	int len = lpmin(lex) - lpmax(lex); // if error then min >= max

	int outlen = copy_lexchars(lex, lpmax(lex), len, NULL, 0);
	VLADecl(wchar_t, wcstr, outlen + 1);
	copy_lexchars(lex, lpmax(lex), len, wcstr, outlen);

	fprintf(stderr, "%ls:%d: characters %d-%d : UnMatched: %ls\n",
		parser->filename, lcn.line, lcn.column, lcn.column + len, wcstr
	);
	0

let blkcomment = function
| "*/" ->
	CTrue    // exit
| "*"
| "[^*\n]+" ->
	blkcomment()
| crlf ->
	crlf_add(lto_crlfcnt(lex), lpmax(lex));
	blkcomment()

let tstring = function
| '"' ->
	CString
| '\\n' ->
	wcsbuf_append_char(lto_buffer(lex), '\n');
	tstring()
| '\\r' ->
	wcsbuf_append_char(lto_buffer(lex), '\r');
	tstring()
| '\\t' ->
	wcsbuf_append_char(lto_buffer(lex), '\t');
	tstring()
| '\\"' ->
	wcsbuf_append_char(lto_buffer(lex), '"');
	tstring()
| '\\' ->
	wcsbuf_append_char(lto_buffer(lex), '\\');
	tstring()

| '[^"\n\r\t\\]+' ->
	copy_lexchars_to_buffer(lex, lpmin(lex), rlex_cursize(lex), lto_buffer(lex));
	tstring()

%% // lexer end
