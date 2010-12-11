#ifndef _LEXER_H_
#define _LEXER_H_

#define LEX_WORD 1
#define LEX_EOL 2
#define LEX_EOF 3

#define LE_NONE 0
#define LE_UNEXPECTED_SYMBOL 1
#define LE_UNCLOSED_QUOTES 2
#define LE_UNKNOWN_LEXER_STATE 3
/*#define LE_*/

typedef struct Lex {
	int type;
	char * str;
} Lex;

extern int lexerErrorNo;
extern char *lexTypeStr[];

void lexItQuiet();
void lexItVerbose();
void initLexer();
void initLexerByString(const char *);
void clearLexer();

Lex * consLex(int, char *);
Lex * getlex();
void delLex(Lex *);
void echoLex(Lex *);


#endif
