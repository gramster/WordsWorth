#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef UNIX
#include <conio.h>
#endif
#include <assert.h>
#include <sys/stat.h>

#define GLOBAL extern

#include "version.h"
#include "ww.h"
#include "dict.h"

char dawgError[120] = { '\0' };
char dictName[64];
short abortMatch = 0;
unsigned long dictsz = 0l;
nodenum NumDictNodes = 0;

NODE huge	*XEdges = NULL;
NODE huge	*Edges = NULL;
#if !defined(DEMO_VERSION)
static int noXdic = 1, exCnt = 0;
static char exWords[MAXEXWORDS][MAXBOARD+1];

void exclude(char *word)
{
	FILE *fp = fopen("WORDS.EX","a");
	if (exCnt<MAXEXWORDS)
	{
		char msg[40];
		strcpy(exWords[exCnt],word);
		strupr(exWords[exCnt]);
		if (fp) fprintf(fp,"%s\n",exWords[exCnt]);
		sprintf(msg,"Zapped %s",exWords[exCnt++]);
		showInfo(1,msg);
	}
	else showInfo(1,"No more space!");
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

static void encrypt(char *buf, int len, char *key)
{
	cryptext = key;
	crypt_length = (int)strlen(key);
	while (len--) crypt(buf++);
}

static nodenum FileSize(FILE *fp)
{
	struct stat st;
	if (fstat(fp->fd, &st)<0)
		return 0;
	else return st.st_size;
}

int loadDawg(char *key)
{
	int i, n;
	char realkey[80];
	FILE *dawg;
#if defined(DEMO_VERSION)
	strcpy(dictName,"wwdict.dem");
#endif
	dawg = fopen(dictName,"rb");
	if (dawg==NULL)
	{
		sprintf(dawgError, "Failed to open dictionary %s! %s",
				dictName, strerror(errno));
		return 0;
	}
#if defined(DEMO_VERSION)
	NumDictNodes = MAXNODES;
#else
	NumDictNodes = (FileSize(dawg) / sizeof(NODE))+1; /* add 1 for safety */
#endif
	if (key)
	{
		int l = (int)strlen(key);
		if (l==0) key=NULL;
		else
		{
			for (i=0;i<l;i++)
				realkey[i] = key[i+1]&0x7F;
			l--;
			while (l>=0 && realkey[l]!=']') l--;
			if (realkey[l]==']')
				realkey[l]=0;
			key = realkey;
		}
	}
	else key="UNREGISTERED";
/*	printf("Decoding using key <%s>\n",key);*/
	if (Edges) farfree(Edges);
	Edges = (NODE huge *)farcalloc(NumDictNodes,sizeof(NODE));
	if (Edges==NULL)
	{
		sprintf(dawgError,"Could not allocate dictionary of %ld bytes; only %ld bytes available!\n",
			(long)NumDictNodes*(long)sizeof(NODE),farcoreleft());
		return 0;
	}
	n = fread(Edges+1,sizeof(NODE),8191,dawg);
	if (n>=0 && key)
		encrypt((char*)(Edges+1),n*sizeof(NODE),key);
	dictsz = n+1;
	while (!feof(dawg))
	{
		n=fread((char *)(Edges+dictsz),sizeof(NODE),8192,dawg);
		if (n>0)
		{
			if (key) encrypt((char*)(Edges+dictsz),n*sizeof(NODE),key);
			dictsz += n;
			assert(dictsz<=NumDictNodes);
		}
	}
	fclose(dawg);
#if !defined(DEMO_VERSION)
	dawg = fopen("words.no","rb");
	if (dawg)
	{
		if (XEdges) farfree(XEdges);
		XEdges = (NODE huge *)farcalloc(MAXADDNODES,sizeof(NODE));
		if (XEdges==NULL)
		{
			sprintf(dawgError,"Could not allocate additional dictionary of %ld bytes; only %ld bytes available!\n",
				(long)MAXADDNODES*(long)sizeof(NODE),farcoreleft());
			return 0;
		}
		n = fread(XEdges+1,sizeof(NODE),MAXADDNODES,dawg);
		assert(n>=0 && feof(dawg));
		fclose(dawg);
		noXdic = 0;
	}
#endif
	return 1;
}

void freeDict(void)
{
	if (Edges)
	{
		farfree(Edges);
		Edges = NULL;
	}
#if !defined(DEMO_VERSION)
	if (XEdges) farfree(XEdges);
#endif
}

int lookup(char *word)
{
	nodenum n = (nodenum)1;
	NODE last;
	if (word==NULL || *word==0)
		return 0;
	while (*word)
	{
		int v = *word-'A'+1;
		if (n==(nodenum)0)
			return 0;
		for (;;)
		{
			NODE N = Nodes(n);
			int dif = NodeIndex(N)-v;
			if (dif==0)
			{
				last = N;
				n = NextNode(N);
				break;
			}
			else if (dif>0 || IsLastChild(N))
				return 0;
			n++;
		}
		word++;
	}
	return (IsWordEnd(last));
}

int excluded(char *word)
{
#if defined(DEMO_VERSION)
	return 0;
#else
	unsigned int i;
	nodenum n = 1;
	NODE last;
	char wrd[30];
	if (word==NULL || *word==0) return 0;
	strcpy(wrd,word);
	strupr(word = wrd);
	if (exCnt)
	{
		for (i=0;i<exCnt;i++)
		{
			if (strcmp(exWords[i],word)==0)
				return 1;
		}
	}
	if (noXdic) return 0;
	while (*word)
	{
		int v = *word-'A'+1;
		if (n==(nodenum)0) return 0;
		for (;;)
		{
			NODE N = XNodes(n);
			int dif = NodeIndex(N)-v;
			if (dif==0)
			{
				last = N;
				n = NextNode(N);
				break;
			}
			else if (dif>0 || IsLastChild(N))
				return 0;
			n++;
		}
		word++;
	}
	return (IsWordEnd(last));
#endif
}

/*********************************************/
/* Recursively match words in the dictionary */
/*********************************************/

#define PAT_USED	1
#define PAT_MANDATORY	2

typedef struct
{
	ulong chars;
	char flag;
} patLetterT;

patLetterT patInfo[20];
static char newWord[20], newPat[20];
static int patLen, realPatLen, patThink,
	patAnagrams, patRepeats, hasMandatory;

static void clearMarks(void)
{
	nodenum i;
	for (i=1;i<NumDictNodes;i++)
		Unmark(i);
}

static void recurse(int pos, nodenum n)
{
	if (n==(nodenum)0) return;
	if (pos<=1) (void)kbhit(); /* Ctrl-C check */
	while (!abortMatch)
	{
		NODE N = Nodes(n);
		int c = NodeIndex(N)-1;
		if (((1l<<c) & patInfo[newPat[pos]].chars)==0)
			goto skipit; /* don't have the letter*/
		/* Put the corresponding letter in the word */
		newWord[pos] = (char)(c+'A');
		/* If we have reached the end of the word, and the node
			is terminal, we have a valid matched word. */
		if (pos==(patLen-1))
		{
			if (IsWordEnd(N))
			{
				if (patRepeats || !IsMarked(N))
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
					Mark(n);
				}
			}
		}
		else recurse(pos+1, NextNode(N));
skipit:
		/* Move to next edge */
		if (!IsLastChild(N)) n++;
		else break;
	}
}

#ifdef DEBUG
static void showRange(int start, int end)
{
	if ((end-start)>2)
	{
		putchar(start+'A');
		putchar('-');
		putchar(end+'A');
	}
	else while (start<=end)
	{
		putchar(start+'A');
		start++;
	}
}

static void showPatElt(int i)
{
	int l, start=-1;
	if (patInfo[i].flag&PAT_MANDATORY)
		putchar('*');
	putchar('[');
	for (l=0; l<26; l++)
	{
		if (patInfo[i].chars & (1L<<l))
		{
			if (start<0)
				start = l;
		}
		else
		{
			if (start>=0)
			{
				showRange(start, l-1);
				start = -1;
			}
		}
	}
	if (start>=0)
		showRange(start, 25);
	putchar(']');
}

static void showPattern(int l)
{
	int i;
	printf("\bOrder: ");
	for (i=0;i<l;i++)
		printf("%d ", (int)newPat[i]);
	printf("\n");
	for (i=0;i<l;i++)
	{
		showPatElt((int)newPat[i]);
	}
	printf("\n");
}
#endif

static char thinkCh[] = { '-', '\\', '|', '/' };

static void recursivechoose(int l, int pos)
{
	static thinkCnt = 0;
	if (abortMatch) return;
	if (l==0)
	{
#ifdef DEBUG
		showPattern(patLen);
#endif
		recurse(0, (nodenum)1);
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
		if (patAnagrams)
			lim = patLen; /* was realPatLen */
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

void matchPattern(char *pat, int anagrams, int allLengths,
	int repeats, int showThink)
{
	int i, j, start, l;
	i = j = hasMandatory = 0;
	while (pat[i])
	{
		if (pat[i]=='*')
		{
			i++;
			patInfo[j].flag = PAT_MANDATORY;
			hasMandatory = 1;
		}
		else patInfo[j].flag = 0;
		if (pat[i]=='_' || pat[i]=='?')
		{
			patInfo[j].chars = 0x3FFFFFFl;
		}
		else if (pat[i]=='[')
		{
			ulong mask;
			int negate=0;
			i++;
			if (pat[i]=='!')
			{
				negate=1;
				i++;
				patInfo[j].chars = 0x3FFFFFFl;
			}
			else patInfo[j].chars = 0l;
			while (pat[i]!=']' && pat[i])
			{
				if (pat[i+1]==0) return; /* syntax error */
				start = pat[i];
				mask = (1l<<(start-'A'));
				if (pat[i+1]=='-')
				{
					i+=2;
					if (pat[i]==0) return;
					while (start<=pat[i])
					{
						if (negate) patInfo[j].chars &= ~mask;
						else patInfo[j].chars |= mask;
						start++;
						mask = (1l<<(start-'A'));
					}
				}
				else if (negate) patInfo[j].chars &= ~mask;
				else patInfo[j].chars |= mask;
				i++;
			}
		}
		else patInfo[j].chars = 1l << (pat[i]-'A');
		j++;
		i++;
	}
	newPat[j]=newWord[j]=0;
	patAnagrams = anagrams;
	patRepeats = repeats;
	patThink = showThink;
	realPatLen = j;
	clearMarks();
	if (allLengths)
	{
		patLetterT tmpPat[20];
		for (i=0;i<j;i++)
		{
			tmpPat[i] = patInfo[i];
			patInfo[i].flag = 0;
		}
		for (l=2; l<=j; l++)
		{
			/*if (showThink) printf("\bLength %d:\n", l);*/
			/* copy the pattern starting from l-j */
			if (allLengths==2)
			{
				for (i=0; i<l; i++)
					patInfo[i] = tmpPat[j-l+i];
				for (i=l; i<j; i++)
					if (tmpPat[i-l].flag & PAT_MANDATORY)
						break;
				if (i<j) continue; /* subset does not have mandatory char */
			}
			else
			{
				for (i=0; i<l; i++)
					patInfo[i] = tmpPat[i];
				for (i=l; i<j; i++)
					if (tmpPat[i].flag & PAT_MANDATORY)
						break;
				if (i<j) continue; /* subset does not have mandatory char */
			}
			recursivechoose(patLen = l, 0);
			if (!repeats) clearMarks();
		}
	}
	else recursivechoose(patLen = j, 0);
}



