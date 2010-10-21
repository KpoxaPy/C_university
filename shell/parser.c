#include "buffer.h"
#include "parser.h"
#include "echoes.h"

struct flags {
	int EOI,
			EOW,
			EOS,
			Word,
			Amp,
			Quote,
			DQuote,
			BQuote,
			Escape;
};

void initFlags(struct flags *);
void echoFlags(struct flags *);

int parseCharToWord(struct flags *, int, char **);
int handleChar(struct bufferlist *, struct flags *, int);
int parseWord(struct flags *, char **);
/*int getNextChar(struct flags *);*/

void initFlags(struct flags * fp)
{
	fp->EOI = 0;
	fp->EOW = 0;
	fp->EOS = 0;
	fp->Word = 0;
	fp->Amp = 0;
	fp->DQuote = 0;
	fp->Quote = 0;
	fp->BQuote = 0;
	fp->Escape = 0;
}


/*
 *int getNextChar(struct flags * fp)
 *{
 *  int c = getchar();
 *
 *  if (c == EOF)
 *    fp->EOI = 1;
 *
 *  if (c == EOF ||
 *      c == '\n' && fp->Word && !fp->Quote && !fp->Escape)
 *    fp->EOS = 1;
 *
 *  if (c == EOF && fp->Word && !fp->Quote ||
 *      (c == ' ' || c == '\t' || c == '\n') 
 *      && fp->Word && !fp->Quote && !fp->Escape)
 *  {
 *    fp->Word = 0;
 *    fp->EOW = 1;
 *  }
 *
 *  if (c == '"' && !fp->Quote)
 *  {
 *    fp->Quote = 1;
 *    fp->Word = 1;
 *  }
 *  if (c == '"' && fp->Quote)
 *    fp->Quote = 0;
 *
 *  if (!fp->Word &&
 *      c != ' ' && c != '\t' && c != '\n' && c != EOF && c != '"')
 *    fp->Word = 1;
 *}
 *
 */
int parse(struct wordlist * words)
{
	int status;
	static struct flags f = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	char * str;

	clearWordList(words);
	initFlags(&f);

	echoPromt(PROMT_DEFAULT);
	while ((status = parseWord(&f, &str)) <= 0 && str != NULL)
	{
		if (str != NULL)
			addWord(words, str);

		/*printf("Word \"%s\": status: %d\tamp? %d\n", str, status, f.Amp);*/

		if (status == PARSE_ST_EOS ||
				status == PARSE_ST_EOF)
			break;
	}

	/*printf("SUM: status: %d\tamp? %d\n", status, f.Amp);*/

	return status;
}



int parseWord(struct flags * fp, char ** str)
{
	char * tmpStr = NULL;
	int status = 0;

	fp->EOW = 0;

	while (!fp->EOW && !fp->EOS && !fp->EOI)
	{
		status = parseCharToWord(fp, getchar(), &tmpStr);

		if (status == PARSE_ST_MORE)
			echoPromt(PROMT_EXTENDED);
	}

	*str = tmpStr;

	return status;
}



int parseCharToWord(struct flags * fp, int c, char ** str)
{
	static struct bufferlist buffer = {NULL, NULL, 0};
	int status;

	status = handleChar(&buffer, fp, c);

	if (fp->EOW)
		*str = flushBuffer(&buffer);
	else
		*str =  NULL;

	return status;
}

/*
 * Handles chars to buffer, modiffies flags and returns status:
 *  PARSE_ST_MORE - need more input
 *  PARSE_ST_EOS - end of string (getted \n and not expected more input)
 *  PARSE_ST_EOF - end of file (getted EOF and not expected more input)
 *  PARSE_ST_ERROR_QUOTES - error: unbalanced quotes
 *  PARSE_ST_OK - all right
 */
int handleChar(struct bufferlist * buffer, struct flags * fp, int c)
{
	int status = 0;

	/*
	 *printf("Before: char: %c \n", c);
	 *echoFlags(fp);
	 *printf("\n");
	 */

	switch (c)
	{
		case EOF:
			fp->EOI = 1;
			fp->EOS = 1;
			status = PARSE_ST_EOF;
			if (fp->Quote)
			{
				status = PARSE_ST_ERROR_QUOTES;
				break;
			}

		case '\n':
			if (fp->Word && !fp->Quote)
			{
				fp->Word = 0;
				fp->EOW = 1;
				fp->EOS = 1;
			}
			else if (!fp->Word)
				fp->EOS = 1;
			if (fp->Quote)
				status = PARSE_ST_MORE;
			break;

		case '&':
			if (!fp->Quote)
			{
				if (fp->Word)
				{
					fp->Word = 0;
					fp->EOW = 1;
					ungetc(c, stdin);
				}
				else
				{
					fp->EOW = 1;
					fp->Amp = 1;
					addChar(buffer, c);
				}
			}
			break;

		case ' ':
		case '\t':
			if (!fp->Quote && fp->Word)
			{
				fp->Word = 0;
				fp->EOW = 1;
			}
			else if (fp->Quote)
				addChar(buffer, c);
			break;

		case '"':
			fp->Quote = !fp->Quote;
			if (fp->Quote)
				fp->Word = 1;
			break;

		default:
			fp->Word = 1;
			addChar(buffer, c);
			break;
	}

	if (!status && fp->EOS)
		status = PARSE_ST_EOS;

	/*
	 *printf("After: stat: %d \n", status);
	 *echoFlags(fp);
	 *printf("\n\n");
	 */

	return status;
}


void echoFlags(struct flags * fp)
{
	if (fp->EOI)
		printf("EOI ");
	if (fp->EOW)
		printf("EOW ");
	if (fp->EOS)
		printf("EOS ");
	if (fp->Word)
		printf("Word ");
	if (fp->Amp)
		printf("Amp ");
	if (fp->DQuote)
		printf("DQuote ");
	if (fp->Quote)
		printf("Quote ");
	if (fp->BQuote)
		printf("BQuote ");
	if (fp->Escape)
		printf("Escape ");
}
