#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOBAL extern

#include "dict.h"

typedef unsigned short ushort;
typedef unsigned long ulong;

#ifndef __MSDOS__
void strupr(word) char *word;
{
        while (*word)
	{
                if (*word>='a' && *word<='z')
                        *word = *word-'a'+'A';
                word++;
        }
}
#endif

int showMatch(char *word)
{
	puts(word);
	return 0;
}

main(int argc, char *argv[])
{
	char word[16];
	int i;
	if (argc!=2)
	{
		fprintf(stderr,"Usage: %s <dictionary>\n",argv[0]);
		exit(0);
	}
	else strcpy(dictName,argv[1]);
	puts("Loading dictionary");
	if (loadDawg("")==0)
		exit(0);
	puts("Enter QUIT to quit");
	for (;;)
	{
		puts("Word?");
		fgets(word,15,stdin);
		strupr(word);
		i = strlen(word) -1;
		word[i]=0;
		printf("Looking up *%s*\n",word);
		if (lookup(word))
			puts("OK");
		else
			puts("Not found");
		if (strcmp("QUIT",word)==0)
			break;
	}
	return 0;
}


