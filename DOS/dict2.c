#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UNIX
#include <conio.h>
#endif
#include <assert.h>

#define GLOBAL extern

#include "version.h"
#include "ww.h"
#include "dict.h"

/*#define DEBUG*/

char dictName[64];
short abortMatch = 0;

NODE	*XEdges;
#if defined(UNIX) || defined(DEMO_VERSION)
	NODE	*Edges = NULL;
#else
	NODE	*Edges[EDGE_TABLES] = { NULL };
#endif

#if !defined(DEMO_VERSION)
static int noXdic = 1, exCnt = 0;
static char exWords[MAXEXWORDS][MAXBOARD+1];

void exclude(char *word) {
	FILE *fp = fopen("WORDS.EX","a");
	if (exCnt<MAXEXWORDS) {
		char msg[40];
		strcpy(exWords[exCnt],word);
		strupr(exWords[exCnt]);
		if (fp) fprintf(fp,"%s\n",exWords[exCnt]);
		sprintf(msg,"Zapped %s",exWords[exCnt++]);
		showInfo(1,msg);
	} else showInfo(1,"No more space!");
	if (fp) fclose(fp);
}
#endif

/***********************/
/* DICTIONARY HANDLING */
/***********************/

/* S-coder encryption from Dr Dobbs Jan 1990 */

static char 	*cryptext;	/* Key				*/
static int	crypt_ptr=0;	/* Circular pointer into key	*/
static int	crypt_length;	/* Key length			*/

static void crypt(char *buf)
{
  *buf ^= (char)(cryptext[crypt_ptr]^(cryptext[0]*crypt_ptr));
  cryptext[crypt_ptr] += ((crypt_ptr < (crypt_length - 1)) ?
			  cryptext[crypt_ptr+1] : cryptext[0]);
  if (!cryptext[crypt_ptr]) cryptext[crypt_ptr]++;
  if (++crypt_ptr >= crypt_length) crypt_ptr = 0;
}

static void encrypt(char *buf, int len, char *key) {
	cryptext = key;
	crypt_length = (int)strlen(key);
	while (len--) crypt(buf++);
}

int loadDawg(char *key) {
	int i, n;
	char realkey[80];
	FILE *dawg;
#if defined(DEMO_VERSION)
	strcpy(dictName,"wwdict.dem");
#endif
	dawg = fopen(dictName,"rb");
	if (dawg==NULL) return 0;
	if (key) {
		n = (int)strlen(key);
		if (n==0) key=NULL;
		else {
			for (i=0;i<n;i++) realkey[i] = key[i+1]&0x7F;
			n--;
			while (n>=0 && realkey[n]!=']') n--;
			if (realkey[n]==']') realkey[n]=0;
			key = realkey;
		}
	} else key="UNREGISTERED";
/*	printf("Decoding using key <%s>\n",key);*/
#if defined(UNIX) || defined(DEMO_VERSION)
	if (Edges) farfree(Edges);
	Edges = (NODE *)farcalloc(MAXNODES,sizeof(NODE));
	if (Edges==NULL) {
		fprintf(stderr,"Could not allocate dictionary of %ld bytes; only %ld bytes available!\n",
			(long)MAXNODES*(long)sizeof(NODE),farcoreleft());
		return 0;
	}
	n = fread(&Nodes(1),sizeof(NODE),MAXNODES,dawg);
	if (n>=0 && key) encrypt((char*)&Nodes(1),n*sizeof(NODE),key);
	assert(n>0 && n<MAXNODES && feof(dawg));
#else
	for (i=0;i<EDGE_TABLES;i++)
	{
		if (Edges[i]) farfree(Edges[i]);
		if ((Edges[i] = (NODE *)farcalloc(8192,sizeof(NODE))) == NULL)
		{
			fprintf(stderr,"Could not allocate dictionary of %ld bytes; only %ld bytes available!\n",
				i*8192l*sizeof(NODE),(long)farcoreleft());
			return 0;
		}
	}
#if defined(UNIX)||defined(DEMO_VERSION)
	n = (int)fread(&Edges[1],sizeof(NODE),8191,dawg);
	if (n>=0 && key) encrypt((char*)&Edges[1],n*sizeof(NODE),key);
#else
	n = (int)fread(&Edges[0][1],sizeof(NODE),8191,dawg);
	if (n>=0 && key) encrypt((char*)&Edges[0][1],n*sizeof(NODE),key);
#endif
	i = 1;
	while (!feof(dawg))
	{
		n=(int)fread(Edges[i],sizeof(NODE),8192,dawg);
		if (n>0)
		{
			assert(i<EDGE_TABLES);
			if (key) encrypt((char*)Edges[i],n*sizeof(NODE),key);
		}
		i++;
	}
#endif
	fclose(dawg);
#if !defined(DEMO_VERSION)
	dawg = fopen("words.no","rb");
	if (dawg) {
		if (XEdges) farfree(XEdges);
		XEdges = (NODE *)farcalloc(MAXADDNODES,sizeof(NODE));
		if (XEdges==NULL) {
			fprintf(stderr,"Could not allocate additional dictionary of %ld bytes; only %ld bytes available!\n",
				(long)MAXADDNODES*(long)sizeof(NODE),farcoreleft());
			return 0;
		}
		n = fread(&XNodes(1),sizeof(NODE),MAXADDNODES,dawg);
		assert(n>=0 && feof(dawg));
		fclose(dawg);
		noXdic = 0;
	}
#endif
	return 1;
}

int lookup(char *word) {
	int n = 1, last;
	if (word==NULL || *word==0) return 0;
	while (*word) {
		int v = *word-'A'+1;
		int N = n;
		if (n==0) return 0;
		for (;;) {
			int dif = Nodes(N).cindex-v;
			if (dif==0) {
				last = N;
				n = (int)(Nodes(N).next);
				break;
			} else if (dif>0 || Nodes(N).islast)
				return 0;
			N++;
		}
		word++;
	}
	return (Nodes(last).isterminal);
}

int excluded(char *word) {
#if defined(DEMO_VERSION)
	return 0;
#else
	int n = 1, i, last;
	char wrd[30];
	if (noXdic || word==NULL || *word==0) return 0;
	strcpy(wrd,word);
	strupr(word = wrd);
	if (exCnt) {
		for (i=0;i<exCnt;i++) {
			if (strcmp(exWords[i],word)==0)
				return 1;
		}
	}
	while (*word) {
		i = 0;
		if (n==0) return 0;
		for (;;) {
			if ((*word-'A'+1)==(int)(XNodes(n+i).cindex)) {
				last = n+i;
				n = (int)(XNodes(n+i).next);
				break;
			} else if (XNodes(n+i).islast) return 0;
			i++;
		}
		word++;
	}
	return (XNodes(last).isterminal);
#endif
}

/*********************************************/
/* Recursively match words in the dictionary */
/*********************************************/

#define PAT_USED	1
#define PAT_MANDATORY	2

typedef struct {
	ulong chars;
	char flag;
} patLetterT;

patLetterT patInfo[20];
static char newWord[20], newPat[20];
static int patLen, realPatLen, patThink,
	patAnagrams, hasMandatory, multiWord = 0;

static void clearMarks(void)
{
	unsigned long i;
	for (i=1;i<MAXNODES;i++) Nodes(i).matched=0;
}

static void recurse(int pos, ushort n)
{
	if (n==0) return;
	while (!abortMatch)
	{
		int c = Nodes(n).cindex-1;
		if (((1l<<c) & patInfo[newPat[pos]].chars)==0)
			goto skipit; /* don't have the letter*/
		/* Put the corresponding letter in the word */
		newWord[pos] = (char)(c+'A');
		/* If we have reached the end of the word, and the node
			is terminal, we have a valid matched word. */
		if (pos==(patLen-1))
		{
			if (Nodes(n).isterminal)
			{
				if (patAnagrams<2 || !Nodes(n).matched)
				{
					int i;
					newWord[pos+1]=0;
					/* If there is an unused mandatory letter don't
							show this word */
					if (hasMandatory)
					{
						for (i=0;i<realPatLen;i++)
							if ((patInfo[i].flag&3)==PAT_MANDATORY)
								goto noShow;
					}
					if (!excluded(newWord))
						abortMatch = showMatch(newWord);
noShow:
					Nodes(n).matched = 1;
				}
			}
		}
		else recurse(pos+1,Nodes(n).next);
skipit:
		/* Move to next edge */
		if (!Nodes(n).islast) n++;
		else break;
	}
}

/* Match multiple words. We do the following:
    
static short freq[27], multiLength=0;

static void resetFreq(void)
{
	int i;
	for (i=1;i<=26;i++) freq[i] = 0;
}

static void multiRecurse(ushort n)
{
	if (n==0) return;
	length++;
	while (!abortMatch)
	{
		int c = Nodes(n).cindex;
		if (freq[c]==0)
			goto skipit; /* don't have the letter*/
		freq[c]--;
		/* Put the corresponding letter in the word */
		*result++ = (char)(c+'A'-1);
		/* If the node is terminal, try next word */
		if (Nodes(n).isterminal)
		{
			if (multiLength==patLen)
			{
				if (patAnagrams<2 || !Nodes(n).matched)
				{
					*result = '\0';
					abortMatch = showMatch(multiResult);
				}
			}
			else
			{
				*result++=' ';
				multiRecurse(0);
				result--;
			}
		}
		else multiRecurse(Nodes(n).next);
		freq[c]++;
		result--;
skipit:
		/* Move to next edge */
		if (!Nodes(n).islast) n++;
		else break;
	}
	length--;
}

#ifdef DEBUG
static void showPattern(int l)
{
	int i;
	printf("\bOrder: ");
	for (i=0;i<l;i++) printf("%d ", (int)newPat[i]);
	printf("\n");
}
#endif

static char thinkCh[] = { '-', '\\', '|', '//' };

static void recursivechoose(int l, int pos)
{
	static thinkCnt = 0;
	if (abortMatch) return;
	if (l==0)
	{
#ifdef DEBUG
		showPattern(patLen);
#endif
		recurse(0,1);
		if (patThink)
		{
			thinkCnt = (thinkCnt+1) & 0xFF;
			if (!thinkCnt)
			{
				printf("\b%c", thinkCh[patThink-1]);
				fflush(stdout);
				patThink = 1 + (patThink%4);
			}
		}
#ifndef UNIX
		if (kbhit())
			if (getch()==27)
				abortMatch = 1;
#endif
	}
	else
	{
		int i, lim;
		if (patAnagrams) lim = realPatLen;
		else lim = pos+1;
		for (i=pos;i<lim;i++)
		{
			if (patInfo[i].flag&PAT_USED) continue;
			newPat[patLen-l]=(char)i;
			patInfo[i].flag |= PAT_USED;
			if (patAnagrams) recursivechoose(l-1,0);
			else recursivechoose(l-1,lim);
			patInfo[i].flag &= ~PAT_USED;
		}
	}
}

void matchPattern(char *pat, int anagrams, int allLengths, int showThink)
{
	int i, j, start, l;
	i = j = hasMandatory = 0;
	while (pat[i]) {
		if (pat[i]=='*') {
			i++;
			patInfo[j].flag = PAT_MANDATORY;
			hasMandatory = 1;
		} else patInfo[j].flag = 0;
		if (pat[i]=='_' || pat[i]=='?') {
			patInfo[j].chars = 0x3FFFFFFl;
		} else if (pat[i]=='[') {
			ulong mask;
			int negate=0;
			i++;
			if (pat[i]=='!') {
				negate=1;
				i++;
				patInfo[j].chars = 0x3FFFFFFl;
			} else patInfo[j].chars = 0l;
			while (pat[i]!=']' && pat[i]) {
				if (pat[i+1]==0) return; /* syntax error */
				start = pat[i];
				mask = (1l<<(start-'A'));
				if (pat[i+1]=='-') {
					i+=2;
					if (pat[i]==0) return;
					while (start<=pat[i]) {
						if (negate) patInfo[j].chars &= ~mask;
						else patInfo[j].chars |= mask;
						start++;
						mask = (1l<<(start-'A'));
					}
				} else if (negate) patInfo[j].chars &= ~mask;
				else patInfo[j].chars |= mask;
				i++;
			}
		} else patInfo[j].chars = 1l << (pat[i]-'A');
		j++;
		i++;
	}
	newPat[j]=newWord[j]=0;
	patAnagrams = anagrams;
	patThink = showThink;
	realPatLen = j;
	clearMarks();
	if (allLengths)
		for (l=2; l<=j; l++)
		{
			if (showThink) printf("\bLength %d:\n", l);
			recursivechoose(patLen = l,0);
			if (patAnagrams == 2) clearMarks();
		}
	else recursivechoose(patLen = j,0);
}
