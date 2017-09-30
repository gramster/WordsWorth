#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "reg.h"

// Given two words, returns true or false depending on whether the
// first word matches a topic with the second.

#if !defined(W16) && !defined(W32) && !defined(__MSDOS__)
static void strlwr(char *s) { for (; *s; s++) if (isupper(*s)) *s = tolower(*s); }
static void strupr(char *s) { for (; *s; s++) if (islower(*s)) *s = toupper(*s); }
#endif

/* S-coder encryption from Dr Dobbs Jan 1990 */

class Crypto
{
    int crypt_length;		/* Key length			*/
    int	crypt_ptr;		/* Circular pointer into key	*/
    char *cryptext;	/* Key				*/
    void Crypt(char *buf);
  public:
    Crypto(char *key)
	: crypt_length((int)strlen(key)), crypt_ptr(0), cryptext(key) 
    {
    }
    void Encrypt(char *buf, int len);
};

void Crypto::Crypt(char *buf)
{
    *buf ^= (char)(cryptext[crypt_ptr]^(cryptext[0]*crypt_ptr));
    cryptext[crypt_ptr] += ((crypt_ptr < (crypt_length - 1)) ?
    	  cryptext[crypt_ptr+1] : cryptext[0]);
    if (!cryptext[crypt_ptr]) cryptext[crypt_ptr]++;
    if (++crypt_ptr >= crypt_length) crypt_ptr = 0;
}

void Crypto::Encrypt(char *buf, int len)
{
    while (len--) Crypt(buf++);
}

Crypto *scribble = 0;

void MakeScribble()
{
    extern char Register[];
    char *key = Register;
    char *realkey=0, keybuf[80];
    int l = (int)strlen(key);
    if (l>0)
    {
    	for (int i=0; i<l; i++)
    	    keybuf[i] = key[i+1]&0x7F;
    	l--;
    	while (l>=0 && keybuf[l]!=']') l--;
    	if (keybuf[l]==']')
    	    keybuf[l]=0;
    	realkey = keybuf;
    }
    else realkey="UNREGISTERED";
    scribble = realkey ? (new Crypto(realkey)) : 0;
}

int GetEntry(char *inword, short topics[])
{
    FILE *fp = fopen("thes.dat", "rb");
    if (fp == 0) return 0;
    MakeScribble();
    int tnum = 0;
    while (!feof(fp))
    {
        char word[32];
	int ch, l = 0;
	while ((ch = GetC(fp)) != 0)
	    if (feof(fp)) return 0;
	    else word[l++] = ch;
	word[l] = 0;
	if ((l%2) == 0) (void)GetC(fp);
	int match = (strcmp(word, inword)==0);
	short t;
	for (;;)
	{
	    unsigned char c1 = GetC(fp);
	    unsigned char c2 = GetC(fp);
	    t = (c1<<8)|c2;
	    if (!feof(fp) && t)
	    {
	        if (match)
	        {
		    topics[tnum++] = t;
//		    printf("Adding topic %d\n", t);
		}
	    }
	    else break;
	}
	if (match) break;
    }
    fclose(fp);
    delete scribble;
    return tnum;
}

main(int argc, char **argv)
{

    if (argc != 3) {
	fprintf(stderr, "Useage: check <keyword> <checkword>\n");
	exit(-1);
    }
    char kbuf[32], wbuf[32];
    strcpy(kbuf, argv[1]);
    strupr(kbuf);
    strcpy(wbuf, argv[2]);
    strupr(wbuf);
    short ktopics[50];
    short wtopics[50];
    int knum = GetEntry(kbuf, ktopics);
    int wnum = GetEntry(wbuf, wtopics);
    for (int i = 0; i < knum; i++)
        for (int j = 0; j < wnum; j++)
	    if (ktopics[i] == wtopics[j])
	    {
		puts("TRUE");
		return 0;
	    }
    puts("FALSE");
}


