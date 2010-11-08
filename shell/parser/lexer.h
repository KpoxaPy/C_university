#ifndef _LEXER_H_
#define _LEXER_H_

#define LEX_WORD 1
#define LEX_BG 2
#define LEX_AND 3
#define LEX_OR 4
#define LEX_PIPE 5
#define LEX_REDIRECT_OUTPUT 6
#define LEX_REDIRECT_OUTPUT_APPEND 7
#define LEX_REDIRECT_INPUT 8
#define LEX_SCRIPT 9
#define LEX_LPAREN 10
#define LEX_RPAREN 11
#define LEX_EOL 12
#define LEX_EOF 13

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

void initLexer();
void initLexerByString(const char *);
void clearLexer();

Lex * consLex(int, char *);
Lex * getlex();
void delLex(Lex *);
void echoLex(Lex *);


#endif
