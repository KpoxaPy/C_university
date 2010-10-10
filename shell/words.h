#ifndef _WORDS_H_
#define _WORDS_H_

struct word {
  char * str;
  int len;
  struct word * next;
};

struct wordlist {
  struct word * first; 
  struct word * last;
  unsigned count;
};


void clearWordList(struct wordlist *);
void echoWordList(struct wordlist *);
void addWord(struct wordlist *, char *);

#endif
