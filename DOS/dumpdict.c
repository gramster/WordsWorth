#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOBAL extern

#include "dict.h"

typedef unsigned short ushort;
typedef unsigned long ulong;

char newWord[20];

/* stubs for dict.c */

void showInfo(int paws, char *msg)
{
	(void)paws;
	(void)msg;
}

int showMatch(char *word)
{
	(void)word;
	return 0;
}

static void recurse(int pos, nodenum n)
{
	int c;
	if (n==(nodenum)0) return;
	for (;;)
	{
		NODE N = Nodes(n);
		c = NodeIndex(N)-1;
		/* Put the corresponding letter in the word */
		newWord[pos] = (char)(c+'a');
		/* If the node is terminal, we have a valid matched word. */
		if (IsWordEnd(N))
		{
			int i;
			newWord[pos+1]=0;
			puts(newWord);
		}
		recurse(pos+1,NextNode(N));
		/* Move to next edge */
		if (!IsLastChild(N)) n++;
		else break;
	}
}

static void rawdump(void)
{
	unsigned long i;
	extern unsigned long dictsz;
	printf("Nodes: %ld\n", dictsz);
	for (i=0;i<dictsz;i++)
	{
		NODE N = Nodes(i);
		printf("%-6ld %-6ld %c%c%c %2d (%c)\n", (long)i,
			(long)NextNode(N),IsWordEnd(N)?'T':' ',
			IsLastChild(N)?'L':' ',
			IsMarked(N)?'M':' ',
			(int)NodeIndex(N), NodeChar(N));
	}
}

int main(int argc, char *argv[])
{
	char word[16];
	int i, raw = 0;
	if (argc!=2)
	{
		if (argc==3 && strcmp(argv[1], "-R")==0)
			raw = 1;
		else
		{
			fprintf(stderr,"Usage: %s [-R] <dictionary>\n",argv[0]);
			exit(0);
		}
	}
	strcpy(dictName,argv[argc-1]);
	if (loadDawg("")==0)
	{
		fprintf(stderr, "Dictionary load failed!\n");
		exit(0);
	}
	else if (raw)
	{
		puts("RAW DUMP");
		rawdump();
	}
	else recurse(0, (nodenum)1);
	return 0;
}
