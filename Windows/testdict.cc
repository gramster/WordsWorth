#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "register.h"
#include "dict.h"

Dictionary *dict = 0;

void main(int argc, char **argv)
{
    if (argc < 3 || argc > 5)
    {
	fprintf(stderr, "Useage: testdict <pat> <type> [<minlen> [<maxwords>]]\n");
	exit(-1);
    }
    int ml = 0, mw = 0;
    if (argc >= 4) ml = atoi(argv[3]);
    if (argc == 5) mw = atoi(argv[4]);
    char *pat = argv[1];
    int typ = atoi(argv[2]);
    dict = new 0;
    char *msg = LoadDictionary(dict, "wwmed.dic");
    if (dict)
    {
        dict->SetLogFile(stdout);
#if 1
        dict->MatchPattern(pat, typ, ml, 0, 0, mw);
#else
        dict->StartConsult(pat, typ, ml, 0, 0, mw);
        long cnt = 0;
        while (dict->NextConsult() == 0) cnt++;
        printf("%ld consult calls\n", cnt);
#endif
        delete dict;
    }
}
