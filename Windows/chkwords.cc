// Read a file containing a list of words. Each word is echoed to 
// standard output, preceded by a `+' or `-' depending on whether
// it is or isn't in the dictionary.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dict.h"

#if !defined(W16) && !defined(W32) && !defined(__MSDOS__)
static void strlwr(char *s) { for (; *s; s++) if (isupper(*s)) *s = tolower(*s); }
static void strupr(char *s) { for (; *s; s++) if (islower(*s)) *s = toupper(*s); }
#endif
#if !defined(W16) && !defined(__MSDOS__)
#define far
#endif

void DoPass(char *fname, char *dictname)
{
    Dictionary *dict = 0;
    char *err = MakeDictionary(dict, (dictname ? dictname : "wwbig.dic"));
    if (err) { fprintf(stderr, "%s\n", err); return; }
    FILE *ifp = fopen(fname, "r");
    if (ifp==0) return;
    char buff[256];
    while (!feof(ifp))
    {
        if (fgets(buff, 256, ifp) == 0) break;
	buff[strlen(buff)-1] = 0;
	strupr(buff);
	printf("%c%s\n", (dict->Lookup(buff) ? '+' : '-'), buff);
    }
    fclose(ifp);
    delete dict;
}

//-------------------------------------------------------------------

main(int argc, char **argv)
{
    if (argc==3)
	DoPass(argv[2], argv[1]);
    else if (argc==2)
	DoPass(argv[1], 0);
    else
        fprintf(stderr, "Useage: chkwords [<dictionary>] <file>\n");
    return 0;
}

