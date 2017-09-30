#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blowfish.h"
#include "dict.h"

unsigned char *BigKey = "6g$#2[]';"; // only 64-bits are used

long MaxDicSize()
{
    return 150000l;
}

char *MakeDictionary(Dictionary *&dict, char *dictname)
{
    if (dict) delete dict;
    dict = new Dictionary;
    char *rtn = dict->Load(dictname, BigKey);
    if (rtn)
    {
        delete dict;
	dict = 0;
    }
    return rtn;
}

int showprogress = 0;

void ShowProgress(int p)
{
    if (showprogress)
    {
        if (p) fprintf(stderr, "\b\b\b\b");
        fprintf(stderr, "%3d%%", p);
        fflush(stderr);
    }
}

void useage()
{
    fprintf(stderr, "Useage: consult [-m|-p|-s] [-l <n>] [-L <n>] [-w <n>] [-W <n>] [-P] [-o <file>] <pattern>\n");
    fprintf(stderr, "where:\n");
    fprintf(stderr, "\t-m\tSelect multi-word anagrams\n");
    fprintf(stderr, "\t-p\tSelect pattern prefix matching\n");
    fprintf(stderr, "\t-s\tSelect pattern suffix matching\n");
    fprintf(stderr, "\t-l\tSpecify minimum allowed word length\n");
    fprintf(stderr, "\t-L\tSpecify maximum allowed word length\n");
    fprintf(stderr, "\t-w\tSpecify minimum allowed number of words\n");
    fprintf(stderr, "\t-W\tSpecify maximum allowed number of words\n");
    fprintf(stderr, "\t-P\tShow progress (use only with -o or output redirection)\n");
    fprintf(stderr, "\t\t\tand currently doesn't work for multi-words\n");
    fprintf(stderr, "\t-o\tSpecify file to which results should be written\n");
    fprintf(stderr, "and:\n");
    fprintf(stderr, "\t<pattern> is the consultation pattern, consisting of\n");
    fprintf(stderr, "\t\tupper and lower case letters, letter ranges, and\n");
    fprintf(stderr, "\t\tthe special characters:\n\n");
    fprintf(stderr, "\t\tMeaning       Fixed   Moveable\n");
    fprintf(stderr, "\t\t=================================\n");
    fprintf(stderr, "\t\tAny letter      :         .\n");
    fprintf(stderr, "\t\tAny vowel       *         +\n");
    fprintf(stderr, "\t\tAny consonant   =         -\n");
    exit(-1);
}

main(int argc, char **argv)
{
    if (argc>1)
    {
	char patbuf[80], topicbuf[80];
	int typ=USEALL, minlength = 0, maxlength = 0, mincnt = 0, maxcnt = 0;
	patbuf[0] = topicbuf[0] = 0;
	FILE *outfp = stdout;
        for (int i = 1; i < (argc-1); i++)
	{
	    if (argv[i][0] == '-')
	    {
	        switch (argv[i][1])
		{
		case 'm' : if (showprogress) useage();
			   typ = MULTI; break;
		case 'p' : typ = PREFIX; break;
		case 's' : typ = SUFFIX; break;
		case 'l' : minlength = atoi(argv[++i]); break;
		case 'L' : maxlength = atoi(argv[++i]); break;
		case 'w' : mincnt = atoi(argv[++i]); break;
		case 'W' : maxcnt = atoi(argv[++i]); break;
		case 'P' : if (typ == MULTI) useage();
			   showprogress = 1; break;
		case 'o' : outfp = fopen(argv[++i], "w");
			   if (outfp == 0) outfp = stdout;
			   break;
		}
	    }
	    else useage();
	}
	strcpy(patbuf, argv[argc-1]);

	Dictionary *dict = 0;
	MakeDictionary(dict, "wwbig.dic");
	dict->SetLogFile(outfp);
#if 0
	dict->StartConsult(patbuf, typ, minlength, maxlength, mincnt, maxcnt, topicbuf);
	while (dict->NextConsult() == 0);
#else
        dict->MatchPattern(patbuf, typ, minlength, maxlength, mincnt, maxcnt, topicbuf);
#endif
	delete dict;
	if (outfp != stdout) fclose(outfp);
    }
    else useage();
}


