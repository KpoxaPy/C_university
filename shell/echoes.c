#include <unistd.h>
#include "echoes.h"
#include "parser/parser.h"
#include "parser/lexer.h"

const char promtFormat[]    = "%s@ ";
const char promtFormatExt[] = "> ";

void echoErrorLex(void);
void exhoErrorSymbol(void);

void echoPromt(int num)
{
	char * cwd;

	if (prStatus.quiet)
		return;

	switch (num)
	{
		case PROMT_DEFAULT:
			cwd = getcwd(NULL, 0);
			printf(promtFormat, cwd);
			free(cwd);
			break;
		case PROMT_EXTENDED:
			printf(promtFormatExt);
			break;
		case PROMT_LEAVING:
			printf("\n");
			break;
	}
}

void echoParserError(void)
{
	char errHead[] = "Parser error: ";

	switch (parserErrorNo)
	{
		case PE_UNEXPECTED_END_OF_COMMAND:
			fprintf(stderr, "%sUnexpected end of command\n", errHead);
			echoErrorLex();
			break;
		case PE_NULL_POINTER:
			fprintf(stderr, "%s(Internal) Parser getted null pointer to tCmd\n", errHead);
			echoErrorLex();
			break;
		case PE_EXPECTED_RPAREN:
			fprintf(stderr, "%sExpected right paren\n", errHead);
			echoErrorLex();
			break;
		case PE_EXPECTED_WORD:
			fprintf(stderr, "%sExpected word\n", errHead);
			echoErrorLex();
			break;
		case PE_LEXER_ERROR:
			echoLexerError();
			break;
		case PE_NONE:
		default:
			break;
	}
}

void echoLexerError(void)
{
	char errHead[] = "Lexer error: ";

	switch (lexerErrorNo)
	{
		case LE_UNEXPECTED_SYMBOL:
			fprintf(stderr, "%sUnexpected symbol\n", errHead);
			break;
		case LE_UNCLOSED_QUOTES:
			fprintf(stderr, "%sUnclosed quotes\n", errHead);
			break;
		case LE_UNKNOWN_LEXER_STATE:
			fprintf(stderr, "%s(Internal) Lexer went to unknown state\n", errHead);
			break;
		case LE_NONE:
		default:
			break;
	}
}

void echoErrorLex(void)
{
	if (errorLex == NULL)
	{
		fprintf(stderr, "There's no error lex.\n");
		return;
	}

	fprintf(stderr, "Error lex: %s", lexTypeStr[errorLex->type]);

	if (errorLex->str != NULL)
		fprintf(stderr, " \"%s\"\n", errorLex->str);
	else
		fprintf(stderr, "\n");
}

void exhoErrorSymbol(void)
{
}
