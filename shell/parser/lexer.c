#include <stdlib.h>
#include <stdio.h>
#include "../buffer.h"
#include "../echoes.h"
#include "lexer.h"

#define GC_TYPE_STDIN 0
#define GC_TYPE_STRING 1

#define LS_START 0	/* starting state */
#define LS_WORD 1	/* Waiting word state */
#define LS_QUOTE 2	/* In quotes state */
#define LS_BSQUOTE 3	/* Back slash into quotes state */
#define LS_BS 4	/* Back slash (outter of quotes) state*/
#define LS_SPECIAL_1 5	/* Waiting for 1-symbol lex state */
#define LS_SPECIAL_1_2 6	/* Waiting for 1- or 2-symbol lex state*/
#define LS_ERROR 7	/* Error state */

struct gcInfo {
	int type;
	int quiet;
	int c;

	const char * srcStr;
	struct bufferlist * buf;
} gcInfo = {GC_TYPE_STDIN, 1, -2, NULL, NULL};

int lexerErrorNo = LE_NONE;

char *lexTypeStr[] = {
	"",
	"LEX_WORD",
	"LEX_BG",
	"LEX_AND",
	"LEX_OR",
	"LEX_PIPE",
	"LEX_REDIRECT_OUTPUT",
	"LEX_REDIRECT_OUTPUT_APPEND",
	"LEX_REDIRECT_INPUT",
	"LEX_SCRIPT",
	"LEX_LPAREN",
	"LEX_RPAREN",
	"LEX_EOL",
	"LEX_EOF"};

void waitChar();
int isWaitChar();
void gc();
void echoExtendedPromt();

void lexItQuiet()
{
	gcInfo.quiet = 1;
}

void lexItVerbose()
{
	gcInfo.quiet = 0;
}

void initLexer()
{
	gcInfo.type = GC_TYPE_STDIN;
	gcInfo.quiet = 0;
	gcInfo.srcStr = NULL;

	waitChar();

	if (gcInfo.buf == NULL)
		gcInfo.buf = newBuffer();
	else
		clearBuffer(gcInfo.buf);
}

void initLexerByString(const char * str)
{
	gcInfo.type = GC_TYPE_STRING;
	gcInfo.quiet = 1;
	gcInfo.srcStr = str;

	waitChar();

	if (gcInfo.buf == NULL)
		gcInfo.buf = newBuffer();
	else
		clearBuffer(gcInfo.buf);
}

void clearLexer()
{
	gcInfo.type = GC_TYPE_STDIN;
	gcInfo.srcStr = NULL;

	waitChar();

	if (gcInfo.buf != NULL)
	{
		clearBuffer(gcInfo.buf);
		free(gcInfo.buf);
		gcInfo.buf = NULL;
	}
}


Lex * getlex()
{
	int state = LS_START;
	int lexType = 0;
	int errorType = LE_NONE;
	lexerErrorNo = LE_NONE;

	if (isWaitChar())
		gc();

	for (;;)
	switch (state)
	{
		case LS_START:
			if (gcInfo.c == ' ' || gcInfo.c == '\t')
				gc();
			else if (gcInfo.c == '&' ||
					gcInfo.c == '|' ||
					gcInfo.c == '>')
				state = LS_SPECIAL_1_2;
			else if (gcInfo.c == '<' ||
					gcInfo.c == ';' ||
					gcInfo.c == '(' ||
					gcInfo.c == ')')
				state = LS_SPECIAL_1;
			else if (gcInfo.c == '\n')
			{
				waitChar();
				return consLex(LEX_EOL, NULL);
			}
			else if (gcInfo.c == EOF)
				return consLex(LEX_EOF, NULL);
			else
				state = LS_WORD;
			break;

		case LS_WORD:
			if (gcInfo.c == '"')
			{
				state = LS_QUOTE;
				gc();
			}
			else if (gcInfo.c == '\\')
			{
				state = LS_BS;
				gc();
			}
			else if (gcInfo.c == '&' ||
				gcInfo.c == '|' ||
				gcInfo.c == '<' ||
				gcInfo.c == '>' ||
				gcInfo.c == ';' ||
				gcInfo.c == '(' ||
				gcInfo.c == ')' ||
				gcInfo.c == ' ' ||
				gcInfo.c == '\t' ||
				gcInfo.c == '\n' ||
				gcInfo.c == EOF)
			{
				return consLex(LEX_WORD, flushBuffer(gcInfo.buf));
			}
			else
			{
				addChar(gcInfo.buf, gcInfo.c);
				gc();
			}
			break;

		case LS_QUOTE:
			if (gcInfo.c == EOF)
			{
				errorType = LE_UNCLOSED_QUOTES;
				state = LS_ERROR;
			}
			else if (gcInfo.c == '\\')
			{
				state = LS_BSQUOTE;
				gc();
			}
			else if (gcInfo.c == '\n')
			{
				echoExtendedPromt();
				addChar(gcInfo.buf, gcInfo.c);
				gc();
			}
			else if (gcInfo.c == '"')
			{
				state = LS_WORD;
				gc();
			}
			else
			{
				addChar(gcInfo.buf, gcInfo.c);
				gc();
			}
			break;

		case LS_BSQUOTE:
			if (gcInfo.c == EOF)
			{
				errorType = LE_UNCLOSED_QUOTES;
				state = LS_ERROR;
			}
			else if (gcInfo.c == '\n')
			{
				state = LS_QUOTE;
				echoExtendedPromt();
				gc();
			}
			else if (gcInfo.c == '\\' ||
				gcInfo.c == '"')
			{
				state = LS_QUOTE;
				addChar(gcInfo.buf, gcInfo.c);
				gc();
			}
			else
			{
				state = LS_QUOTE;
				addChar(gcInfo.buf, '\\');
				addChar(gcInfo.buf, gcInfo.c);
				gc();
			}
			break;

		case LS_BS:
			if (gcInfo.c == EOF)
			{
				addChar(gcInfo.buf, '\\');
				return consLex(LEX_WORD, flushBuffer(gcInfo.buf));
			}
			state = LS_WORD;
			if (gcInfo.c == '\n')
				echoExtendedPromt();
			else
				addChar(gcInfo.buf, gcInfo.c);
			gc();
			break;

		case LS_SPECIAL_1:
			if (gcInfo.c == '<')
				lexType = LEX_REDIRECT_INPUT;
			else if (gcInfo.c == ';')
				lexType = LEX_SCRIPT;
			else if (gcInfo.c == '(')
				lexType = LEX_LPAREN;
			else if (gcInfo.c == ')')
				lexType = LEX_RPAREN;
			waitChar();
			return consLex(lexType, NULL);
			break;

		case LS_SPECIAL_1_2:
			if (gcInfo.buf->count == 0)
			{
				addChar(gcInfo.buf, gcInfo.c);
				gc();
			}
			else if (gcInfo.c == '&' ||
					gcInfo.c == '|' ||
					gcInfo.c == '>')
			{
				char c = getChar(gcInfo.buf);

				if (c != gcInfo.c)
				{
					errorType = LE_UNEXPECTED_SYMBOL;
					state = LS_ERROR;
					break;
				}
				else if (c == '&')
					lexType = LEX_AND;
				else if (c == '|')
					lexType = LEX_OR;
				else if (c == '>')
					lexType = LEX_REDIRECT_OUTPUT_APPEND;

				waitChar();
				return consLex(lexType, NULL);
			}
			else
			{
				char c = getChar(gcInfo.buf);

				if (c == '&')
					lexType = LEX_BG;
				else if (c == '|')
					lexType = LEX_PIPE;
				else if (c == '>')
					lexType = LEX_REDIRECT_OUTPUT;

				return consLex(lexType, NULL);
			}
			break;

		case LS_ERROR:
			printf("Lexer: ");
			if (errorType == LE_NONE)
				printf("Unknown error.");
			else if (errorType == LE_UNCLOSED_QUOTES)
				printf("Unexpeceted end of command: there is unclosed quotes.");
			else if (errorType == LE_UNEXPECTED_SYMBOL)
				printf("Unexpected symbol.");
			else if (errorType == LE_UNKNOWN_LEXER_STATE)
				printf("Unknown lexer state.");
			printf(" Last symbol: ");
			if (gcInfo.c == EOF)
				printf("EOF.");
			else
				printf("'%c'.", gcInfo.c);
			printf("\n");

			lexerErrorNo = errorType;
			return NULL;

		default:
			errorType = LE_UNKNOWN_LEXER_STATE;
			state = LS_ERROR;
			break;
	}
}

void echoExtendedPromt()
{
	if (!gcInfo.quiet)
		echoPromt(PROMT_EXTENDED);
}


void waitChar()
{
	gcInfo.c = -2;
}

int isWaitChar()
{
	return gcInfo.c == -2;
}

void gc()
{
	switch (gcInfo.type)
	{
		case GC_TYPE_STDIN:
			gcInfo.c = getchar();
			break;
		case GC_TYPE_STRING:
			if (*gcInfo.srcStr != '\0')
			{
				gcInfo.c = *gcInfo.srcStr;
				++gcInfo.srcStr;
			}
			else
				gcInfo.c = EOF;
			break;
	}
}

Lex * consLex(int type, char * str)
{
	Lex * lex = (Lex *)malloc(sizeof(Lex));

	if (lex != NULL)
	{
		lex->type = type;
		lex->str = str;
	}

	return lex;
}

void delLex(Lex * lex)
{
	if (lex == NULL)
		return;

	if (lex->str != NULL)
		free(lex->str);

	free(lex);
}

void echoLex(Lex * lex)
{
	if (lex == NULL)
	{
		printf("It's not lex.\n\n");
		return;
	}

	printf("Lex type: %s\n", lexTypeStr[lex->type]);

	if (lex->str != NULL)
		printf("Lex string: \"%s\".\n\n", lex->str);
	else
		printf("No lex string.\n\n");
}
