/* Crossword helper. Looks up matches in the DAWG to patterns specified
   using ? for an unknown character. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MSDOS__
#include <dos.h>
#include <conio.h>
#endif
#include <ctype.h>
#include <assert.h>

#define GLOBAL extern

typedef unsigned short ushort;
typedef unsigned long  ulong;

#define VERSION "2.4"
#include "xwordreg.h"
#include "dict.h"

#define HIST_LINES	10
#define BUFF_LINES	22 /* Don't make bigger than this!! */
#define BUFF_LNLEN	80

char buffer[BUFF_LINES][BUFF_LNLEN];
char history[HIST_LINES][BUFF_LNLEN];
int bufpos = 0;
int histpos = 0;

int multiInit(char *input);
static void Consult(void);
void multiRecurse(nodenum n, nodenum start, short first, short len, short cnt);

static int anagrams = 0, multi = 0, repeats = 1,
	blockword = 0, grafik = 0, deelay = 0, xword = 0,
	anylength, wait = 1;

static FILE *logfp = NULL;

long match;

/* Make string upper case. */
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

#define textbackground(c)
#define textcolor(c)
#define gotoxy(x,y)
#define cputs(s) puts(s)
#define putch(c) putchar(c)
#define cprintf printf
#define kbhit()	0
#define getch()	getchar()
#define clrscr()
#endif

static void toggleLog(void)
{
		if (logfp)
		{
			fclose(logfp);
			logfp = NULL;
		}
		else
			logfp = fopen("xword.log", "a");
}

void showInfo(int paws, char *msg)
{
	fprintf(stdout,msg);
	if (paws>0) sleep((unsigned)paws);
}

static char lastMulti[256] = { 0 };

static short line = 0, waiting = 0;

int flushbuffer(void)
{
	int esc;
	waiting = 0;
	esc = (getch()==27);
	putchar('\n');
	line = 0;
	/* flush buffer */
	while (line < bufpos)
	       	printf(buffer[line++]);
	bufpos = 0;
	return esc;
}

int showMatch(char *word)
{
	char *res = word;
	if (multi)
	{
		int i, j, l;
		char tmpMulti[256], tmp2[256], *indexes[64], *t;
		strcpy(t = tmpMulti, word);
		indexes[0] = t;
		i = 1;
		while (*t)
		{
			if (*t == ' ')
			{
				*t++ = '\0';
				if (*t) indexes[i++] = t;
			}
			else t++;
		}
		/* Sort the indexes */
		l = i;
		for (i=0;i<(l-1);i++)
			for (j=i+1;j<l;j++)
				if (strcmp(indexes[i],indexes[j])>0)
				{
					char *tmp = indexes[i];
					indexes[i] = indexes[j];
					indexes[j] = tmp;
				}
		/* Rebuild the line */
		tmp2[0] = '\0';
		for (i=0;i<l;i++)
		{
			strcat(tmp2, indexes[i]);
			strcat(tmp2, " ");
		}
		/* Compare with last */
		if (repeats || strcmp(tmp2, lastMulti) > 0)
		{
			strcpy(lastMulti, tmp2);
			res = lastMulti;
		}
		else return 0;
	}
	match++;
	if (waiting)
		sprintf(buffer[bufpos++], "\b%5ld %s\n",match,res);
	else
		printf("\b%5ld %s\n",match,res);
	if (logfp)
		fprintf(logfp, "%5ld %s\n",match,res);
	if (waiting)
	{
		if (bufpos == BUFF_LINES || kbhit())
			if (flushbuffer())
				return 1;
	}
	else if (wait && ++line==24)
	{
		fprintf(stdout, "Press any key for more (or ESC to stop)...");
		waiting = 1;
	}
	else if (deelay) sleep(1);
	return abortMatch;
}

/*********************************************************************/
/* Multi-word anagrams */

static short freq[27], minlength = 1, length=0, maxcnt = 0;
static char multiResult[256], *result;

int multiInit(char *input)
{
	int i;
	for (i=1;i<=26;i++) freq[i] = 0;
	length = 0;
	while (*input)
	{
		char c = *input++;
		if (c>='a' && c<='z') freq[c-'a'+1]++;
		else if (c>='A' && c<='Z') freq[c-'A'+1]++;
		else if (c==' ') length--;
		else return -1;
		length++;
	}
	result = multiResult;
	return 0;
}

void multiRecurse(nodenum n, nodenum start, short first, short len, short cnt)
{
	if (n==(nodenum)0) return;
	length--;
	/* for long searches that find no matches, give a chance
		for ctrl-break processing at least 26 times */
	if (len==1) (void)kbhit();

	while (!abortMatch)
	{
		NODE N = Nodes(n);
		int c = NodeIndex(N);
		if (freq[c]==0)
			goto skipit; /* don't have the letter*/
		freq[c]--;
		/* Put the corresponding letter in the word */
		*result++ = (char)(c+'A'-1);
		if (IsWordEnd(N) && len>=minlength)
		{
			if (maxcnt == 0 || cnt<=maxcnt)
			{
				if (length==0)
				{
					*result = '\0';
					abortMatch = showMatch(multiResult);
				}
				else
				{
					/* try next word */
					*result++=' ';
					multiRecurse(start, start, 1, 1, cnt+1);
					result--;
				}
			}
		}
		if (length)
			multiRecurse((nodenum)NextNode(N), start, 0, len+1, cnt);
		freq[c]++;
		result--;
skipit:
		/* Move to next edge */
		if (!IsLastChild(N))
		{
			if (first) start++;
			n++;
		}
		else break;
	}
	length++;
}

/*********************************************************************/
/* Block words */

#define MAX_BLOCK 15

static char blk_tbl[MAX_BLOCK][MAX_BLOCK];
static int blk_size;

static void printBlockF(FILE *fp)
{
	int r, c;
	for (r=0; r<blk_size; r++)
	{
		fputc('\n', fp);
		for (c=0; c<blk_size; c++)
			fputc(blk_tbl[r][c], fp);
	}
	fputc('\n', fp);
}

static void printBlock(void)
{
	match++;
	if (!grafik) printBlockF(stdout);
	else
	{
		int r, c;
		gotoxy(50, 6);
		cprintf("Matches: %d", match);
		for (r=0; r<blk_size; r++)
			for (c=0; c<blk_size; c++)
			{
				gotoxy(c+50, r+8);
				putch(blk_tbl[r][c]);
			}
	}
	if (logfp) printBlockF(logfp);
	if (deelay) sleep(1);
}

static int findTerm(nodenum n, int l)
{
	int c;
	if (n==(nodenum)0) return 0;
	l--;
	for (;;)
	{
		NODE N = Nodes(n);
		if (l)
		{
			if (findTerm(NextNode(N), l)) return 1;
		}
		else if (IsWordEnd(N)) return 1;
		if (IsLastChild(N)) break;
		else n++;
	}
	return 0;
}

static int findWord(char *buf, int gotlen, int totlen)
{
	ushort n = 1;
  int i, l;
	l = totlen-gotlen-1;
	/* Work thru the dict tree for the prefix... */
	for (i = 0; i<=gotlen; i++)
	{
		int ch = buf[i] - 'A' + 1;
		for (;;)
		{
			NODE N = Nodes(n);
			int c = NodeIndex(N);
			if (c==ch)
			{
				if (l==0 && i==gotlen)
					return (IsWordEnd(N));
				n = NextNode(N);
				break;
			}
			else if (c>ch || IsLastChild(N))
				return 0;
			n++;
		}
	}
	/* Now we must find a terminator of length (totlen-gotlen-1) */
	if (findTerm(n, l)) return 1;
	return 0;
}

static int findMatch(nodenum n, char *buf, unsigned long *xchk,
		int totlen, int after)
{
	/* find a totlen-letter word where the letter at position pos is in
		xchk[pos], and put it in buf. If buf already has a word, start
		after that one.
	*/
	int ch = -1;
	if (n==(nodenum)0) return 0;
	if (buf[0]) ch = buf[0] - 'A';
	for (;;)
	{
		NODE N = Nodes(n);
		int c = NodeIndex(N)-1;
		if (after && c>ch) after = 0; /* got past last word */
		if (!after) ch = -1;
		if (c>=ch && (((1l << c) & *xchk) != 0))
		{
			*buf = 'A'+c;
			totlen--;
			if (totlen==0)
			{
				if (c>ch && IsWordEnd(N)) return 1;
			}
			else if (findMatch(NextNode(N), buf+1, xchk+1, totlen, after))
				return 1;
			totlen++;
		}
		if (IsLastChild(N)) break;
		else n++;
	}
	return 0;
}

static void findBlockRow(int row)
{
	int col, cont;
	unsigned long blk_chk[MAX_BLOCK];
	char buf[MAX_BLOCK+1];
	buf[MAX_BLOCK] = '\0';
#ifdef __MSDOS__
	if (kbhit())
		if (getch()==27)
			abortMatch = 1;
#endif
	if (abortMatch) return;
	for (col = 0; col < blk_size; col++)
	{
		int p;
		blk_chk[col] = 0l;
		for (p=0; p<=row; p++)
			buf[p] = blk_tbl[p][col];
		for (p = cont = 0; p<26; p++)
		{
			buf[row+1] = p+'A';
			if (findWord(buf, row+1, blk_size))
			{
				blk_chk[col] |= (1l << p);
				cont = 1;
			}
		}
		if (!cont)
			return;
	}
	row++;
	for (col = 0; col<=blk_size; col++) buf[col] = 0;
	while (findMatch(1, buf, blk_chk, blk_size, 1))
	{
		for (col = 0; col<blk_size; col++)
		{
			blk_tbl[row][col] = buf[col];
			if (grafik)
			{
				gotoxy(col+10, row+8);
				putch(buf[col]);
			}
		}
		if (row == (blk_size-1)) printBlock();
		else findBlockRow(row);
	}
	if (grafik)
	{
		for (col = 0; col<blk_size; col++)
		{
			gotoxy(col+10, row+8);
			putch(' ');
		}
	}
	row--;
}

static void WordBlock(int size, char *seed)
{
	int col;
	for (col = 0; col<size; col++) blk_tbl[0][col] = seed[col];
	blk_size = size;
	strupr(seed);
	abortMatch = 0;
	if (grafik)
	{
		clrscr();
		gotoxy(10, 8);
		cputs(seed);
	}
	findBlockRow(0);
}

/*********************************************************************/

#ifdef DEBUG
static void dumpDict(void)
{
	unsigned long i;
	FILE *fp = fopen("dump","w");
	for (i=0;i<NumDictNodes;i++)
	{
		NODE N = Nodes(i);
		fprintf(stdout,"%-6d %-6d %c%c%c %2d (%c)\n",i,
			NextNode(N),IsWordEnd(N)?'T':' ',
			IsLastChild(N)?'L':' ',
			IsMarked(N)?'M':' ',
			(int)NodeIndex(N),
			NodeChar(N));
	}
	fclose(fp);
}
#endif

/*********************************************************************/
/* crossword solver */

#define KEY_DOWN	0x5000   
#define KEY_UP	0x4800   
#define KEY_LEFT	0x4B00
#define KEY_RIGHT	0x4D00
#define KEY_F1  	0x3B00
#define KEY_HOME	0x4700
#define KEY_END 	0x4F00
#define KEY_PGUP	0x4900
#define KEY_PGDN	0x5100

#define MAX_XWORD	19

/* flags */

#define PUT		1
#define ANYA		2
#define ANYD		4
#define STARTA 	8
#define STARTD 	16
#define RECOMPUTE	32

int	xwordr, xwordc, xword_len, xword_dir, xword_r, xword_c, 
		xword_apos, xword_dpos, localChk = 0;
char	xwordTbl[MAX_XWORD][MAX_XWORD],
		xWord[MAX_XWORD];
unsigned char
		xwordFlag[MAX_XWORD][MAX_XWORD];

unsigned long
	aChk[MAX_XWORD][MAX_XWORD],
	dChk[MAX_XWORD][MAX_XWORD],
	globalChk[MAX_XWORD][MAX_XWORD],
	tmpChk[MAX_XWORD];
long xword_acnt, xword_dcnt;

short my_getch(void)
{
#ifndef __MSDOS__
	return (short)getchar();
#else
	unsigned short rtn;
	_AH=0;
	geninterrupt(0x16);
	rtn = _AX;
	if (rtn & 0x7F) /* ASCII? */
		rtn &= 0x7F;
	return (short)rtn;
#endif
}

#define USED	(0x80000000l)

static void clearWrds(void)
{
	int r;
	for (r=1;r<24;r++)
	{
		gotoxy(60,r);
		cputs("                ");
	}
}

static void clearChks(int doGlobals)
{
	int r, c;
	for (r=0; r<xwordr; r++)
		for (c=0; c<xwordc; c++)
		{
			aChk[r][c] = dChk[r][c] = 0l;
			if (doGlobals)
			{
				globalChk[r][c] = 0x3FFFFFFFl;
				if (xwordFlag[r][c]&PUT)
				{
					xwordTbl[r][c] = '?';
					xwordFlag[r][c] &= ~PUT;
				}
			}
		}
}

static void showBlock(int r, int c, int curspos)
{
	char ch;
	gotoxy(c*3+2, r+2);
	if (xwordTbl[r][c] == '#')
	{
		textbackground(BLACK);
		ch = ' ';
	}
	else
	{
		textbackground(LIGHTGRAY);
		ch = xwordTbl[r][c];
		if (ch=='?')
		{
			if (xwordFlag[r][c]&ANYA)
			{
				if (xwordFlag[r][c]&ANYD)
					ch = '+';
				else ch = '-';
			}
			else if (xwordFlag[r][c]&ANYD)
				ch = '|';
			else ch = '.';
		}
	}
	switch (curspos+((xwordFlag[r][c]&PUT)?2:0))
	{
	case 0:
		if (xwordFlag[r][c]&STARTA)
		{
			if (xwordFlag[r][c]&STARTD)
				textcolor(CYAN);
			else textcolor(MAGENTA);
		}
		else if (xwordFlag[r][c]&STARTD)
			textcolor(BROWN);
		else 
			textcolor(BLACK);
		break;
	case 1:
		textbackground(BLUE);
		textcolor(LIGHTGRAY);
		break;
	case 2:
		textcolor(RED);
		break;
	case 3:
		textcolor(GREEN);
		break;
	}
	putch(' ');
	putch(ch);
	putch(' ');
	textbackground(BLACK);
	textcolor(LIGHTGRAY);
}

static void drawBoard(void)
{
	int r, c;
	/* draw border */
	for (r=0; r<xwordr; r++)
	{
		gotoxy(1,r+2);	 	putch(186); /* left down */
		gotoxy(2+3*xwordc,r+2);	putch(186); /* right down */
	}
	for (c=0; c<xwordc; c++)
	{
		gotoxy(3*c+2,1);	putch(205); putch(205); putch(205); /* top across */
		gotoxy(3*c+2,2+xwordr);	putch(205); putch(205); putch(205); /* bottom across */
	}
	gotoxy(1,1);   			putch(201); /* top left */
	gotoxy(3*xwordc+2,1);		putch(187); /* top right */
	gotoxy(1,xwordr+2);   		putch(200); /* bottom left  */
	gotoxy(3*xwordc+2,xwordr+2);	putch(188); /* bottom right */
	for (r=0; r<xwordr; r++)
		for (c=0; c<xwordc; c++)
			showBlock(r, c, 0);
}

static int isDecideable(int r, int c)
{
	int p, v = -1;
	ulong chk = globalChk[r][c];
	for (p=0; p<26; p++)
		if (chk & (1l << p))
		{
			if (v==-1) v = p;
			else 
			{
				v = -2;
				break;
			}
		}
	if (v>=0)
	{
		if (xwordTbl[r][c] != v+'A')
		{
			xwordTbl[r][c] = v+'A'; /* Only one choice */
			return 1;
		}
	}
	return 0;
}

static int addCrossWord(char *word)
{
	if (xword_dir)
	{
		/* Down word */
		int r, l = xword_len;
		if (localChk)
		{
			if (xword_dpos >23)
			{
				if (localChk==2)
				{
					gotoxy(60,13); cprintf("Press a key...");
					while (!kbhit());
					if (getch()==27) localChk = 1;
					else xword_dpos = 14;
					gotoxy(60,13); cprintf("              ");
				}
			}
			if (xword_dpos <= 23)
			{
				gotoxy(60,xword_dpos);
				cputs("                ");
				gotoxy(60,xword_dpos);
				cputs(word);
				xword_dpos++;
			}
			xword_dcnt++;
			if (xword_dcnt%100 == 0 && kbhit()) return 1; 
		}
		for (r = xword_r; l--; r++)
		{
#ifdef DEBUGSOL
fprintf(stderr,"Add %c to dChk[%d][%d]\n", *word,r,xword_c);
#endif
			dChk[r][xword_c] |= 1l << ( (*word++) - 'A' );
		}
	}
	else
	{
		/* Across word */
		int c, l = xword_len;
		if (localChk)
		{
			if (xword_apos > 12)
			{
				if (localChk==2)
				{
					gotoxy(60,1); cprintf("Press a key...");
					while (!kbhit());
					if (getch()==27) localChk = 1;
					else xword_apos = 2;
					gotoxy(60,1); cprintf("              ");
				}
			}
			if (xword_apos <= 12)
			{
				gotoxy(60,xword_apos);
				cputs("                ");
				gotoxy(60,xword_apos);
				cputs(word);
				xword_apos++;
			}
			xword_acnt++;
			if (xword_acnt%100 == 0 && kbhit()) return 1; 
		}
		for (c = xword_c; l--; c++)
		{
#ifdef DEBUGSOL
fprintf(stderr,"Add %c to aChk[%d][%d]\n", *word,xword_r,c);
#endif
			aChk[xword_r][c] |= 1l << ( (*word++) - 'A' );
		}
	}
	return 0;
}

static int recursiveSearch(int pos, nodenum n)
{
	while (n)
	{
		NODE N = Nodes(n);
		int c = NodeIndex(N)-1;
		if (((1l<<c) & tmpChk[pos]) != 0)
		{
			xWord[pos] = (char)(c+'A');
			if (pos==(xword_len-1))
			{
				if (IsWordEnd(N))
				{
					int i;
					xWord[pos+1]=0;
					if (addCrossWord(xWord)) return 1;
				}
			}
			else recursiveSearch(pos+1,NextNode(N));
		}
		/* Move to next edge */
		if (!IsLastChild(N)) n++;
		else break;
	}
	return 0;
}

static void computeCrossCheck(int r, int c, unsigned dir)
{
	int start, end;
	clearChks(0);
	if (xwordFlag[r][c]&ANYA)
		globalChk[r][c] = aChk[r][c] = 0x3FFFFFFFl;
	else if ((dir & ANYA)==0)
		aChk[r][c] = globalChk[r][c];
	else
	{
		/* find across words */
		xword_r = r;
		xword_dir = 0;
		for (start = c; start>0; start--)
			if (xwordTbl[r][start-1]=='#' || (xwordFlag[r][start]&STARTA))
				break;
		for (end = c; end<(xwordc-1); end++)
			if (xwordTbl[r][end+1]=='#' || (xwordFlag[r][end+1]&STARTA))
				break;
		xword_len = end-start+1;
		if (xword_len>1)
		{
			xword_c = start;
			while (start<=end)
			{
				if (xwordTbl[r][start] == '?')
					tmpChk[start-xword_c] = globalChk[r][start];
				else
					tmpChk[start-xword_c] = 1l << (xwordTbl[r][start] - 'A');
				start++;
			}
		    if (recursiveSearch(0, 1))
			{
				dChk[r][c] = aChk[r][c] = globalChk[r][c];
				goto done;
			}
			else if (localChk)
			{
				gotoxy(60,1);
				cprintf("%5ld Across", xword_acnt);
				if (localChk==2 && xword_apos > 2)
				{
					while (xword_apos < 13)
					{
						gotoxy(60,xword_apos++);
						cprintf("              ");
					}
					gotoxy(60,1); cprintf("Press a key...");
					while (!kbhit());
					if (getch()==27) localChk = 1;
					gotoxy(60,1); cprintf("              ");
				}
			}
		}
		else aChk[r][c] = globalChk[r][c];
	}

	if (xwordFlag[r][c]&ANYD)
		dChk[r][c] = 0x3FFFFFFFl;
	else if ((dir & ANYD)==0)
		dChk[r][c] = globalChk[r][c];
	else
	{
		/* find down words */

		xword_c = c;
		xword_dir = 1;
		for (start = r; start>0; start--)
			if (xwordTbl[start-1][c]=='#' || (xwordFlag[start][c]&STARTD))
				break;
		for (end = r; end<(xwordr-1); end++)
			if (xwordTbl[end+1][c]=='#' || (xwordFlag[end+1][c]&STARTD))
				break;
		xword_len = end-start+1;
		if (xword_len>1)
		{
			xword_r = start;
			while (start<=end)
			{
				if (xwordTbl[start][c] == '?')
					tmpChk[start-xword_r] = globalChk[start][c];
				else
					tmpChk[start-xword_r] = 1l << (xwordTbl[start][c] - 'A');
				start++;
			}
		    if (recursiveSearch(0, 1))
				dChk[r][c] = globalChk[r][c];
			else if (localChk)
			{
				gotoxy(60,13);
				cprintf("%5ld Down", xword_dcnt);
				if (localChk==2 && xword_dpos > 14)
				{
					while (xword_dpos < 24)
					{
						gotoxy(60,xword_dpos++);
						cprintf("              ");
					}
					gotoxy(60,13); cprintf("Press a key...");
					while (!kbhit());
					if (getch()==27) localChk = 1;
					gotoxy(60,13); cprintf("              ");
				}
			}
		}
		else dChk[r][c] = globalChk[r][c];
	}
done:
	globalChk[r][c] = (aChk[r][c] & dChk[r][c]);
	if (isDecideable(r, c))
	{
		xwordFlag[r][c] |= PUT;
		showBlock(r, c, 0);
	}
}

static void computeGlobalChecks(void)
{
	int r, c, k, n = xwordr*xwordc, pass=0, change = 0;
	clearChks(0);
	drawBoard();
/*localChk = 0; */
	pass = 0;
	gotoxy(5,24);
	cprintf("                                                ");
	for (r=0; r<xwordr; r++)
		for (c=0; c<xwordc; c++)
			if (xwordTbl[r][c]=='?')
				xwordFlag[r][c] |= RECOMPUTE;
			else if (xwordTbl[r][c]=='#')
				globalChk[r][c] = 0l;
			else if (xwordTbl[r][c]!='?')
				globalChk[r][c] = 1l << (xwordTbl[r][c]-'A');
	do
	{
		change = 0;
		pass++;
		gotoxy(5,24);
		cprintf("Pass %d", pass);
		for (r=0; r<xwordr; r++)
			for (c=0; c<xwordc; c++)
			{
				ulong mask = globalChk[r][c];
				k = r*xwordc + c + 1;
				gotoxy(20,24);
				cprintf("[%3d%%]",k*100/n);
				if (xwordFlag[r][c]&RECOMPUTE)
				{
					computeCrossCheck(r, c, (ANYA|ANYD));
					xwordFlag[r][c] &= ~RECOMPUTE;
				}
				if (mask != globalChk[r][c])
				{
					int r2, c2;
					change++;
					/* Try to limit the effects a bit */
					r2 = r; c2 = c;
					while (r2>=0 && xwordTbl[r2][c2]!='#')
					{
						if (xwordTbl[r2][c2]=='?')
							xwordFlag[r2][c2] |= RECOMPUTE;
						r2--;
					}
					r2 = r;
					while (r2<xwordr && xwordTbl[r2][c2]!='#')
					{
						if (xwordTbl[r2][c2]=='?')
							xwordFlag[r2][c2] |= RECOMPUTE;
						r2++;
					}
					r2 = r;
					while (c2>=0 && xwordTbl[r2][c2]!='#')
					{
						if (xwordTbl[r2][c2]=='?')
							xwordFlag[r2][c2] |= RECOMPUTE;
						c2--;
					}
					c2 = c;
					while (c2<xwordc && xwordTbl[r2][c2]!='#')
					{
						if (xwordTbl[r2][c2]=='?')
							xwordFlag[r2][c2] |= RECOMPUTE;
						c2++;
					}
				}
#ifdef __MSDOS__
				if (kbhit())
					if (getch()==27)
						goto done;
#endif
			}
		gotoxy(28,24);
		cprintf("%3d changes", change);
	}
	while (change);
done:
	gotoxy(5,24);
	cputs("                                                     ");
}

static int addEntry(int r, int c, int ch)
{
	switch(ch)
	{
	case '.':
	case '?':
		xwordTbl[r][c] = '?';
		break;
	case '#':
		xwordTbl[r][c] = '#';
		break;
	case '^':
		xwordFlag[r][c] |= STARTD;
		break;
	case '>':
		xwordFlag[r][c] |= STARTA;
		break;
	case '+':
		xwordFlag[r][c] |= ANYA|ANYD;
		break;
	case '|':
		xwordFlag[r][c] |= ANYD;
		break;
	case '-':
		xwordFlag[r][c] |= ANYA;
		break;
	default:
		if (ch>='A' && ch<='Z')
			xwordTbl[r][c] = ch;
		else return -1;
		break;
	}
	return 0;
}


static int setEntry(int r, int c, int ch)
{
	xwordTbl[r][c] = '?';
	xwordFlag[r][c] = 0;
	return addEntry(r,c,ch);
}

static int loadBoard(char *nm)
{
	int ch, r, c;
	FILE *fp = fopen(nm, "rt");
	if (fp == NULL) return 0;
	r = c = xwordr = 0;
	if (fscanf(fp, "%d %d", &xwordr, &xwordc)!=2)
		return 0;
	ch = fgetc(fp);
	while (!feof(fp))
	{
		if (ch==';') /* comment */
		{
			do	ch = fgetc(fp);
			while (ch!='\n' && ch!='\r' && !feof(fp));
		}
		else if (ch=='\n' || ch=='\r')
		{
			while ((ch=='\n' || ch=='\r') && !feof(fp))
				ch = fgetc(fp);
		}
		else
		{
			if (ch>='a' && ch<='z') ch -= 'a'-'A';
			if (setEntry(r,c,ch)==0) c++;
			if (c==xwordc)
			{
				r++;
				if (r==xwordr) break;
				c=0;
			}
			ch = fgetc(fp);
		}
	}
	while (!feof(fp))
	{
		int r2, c2;
		if (fscanf(fp, "%d %d %c", &r2, &c2, &ch)==3)
			addEntry(r2-1,c2-1,ch);
		else break;
	}
	return xwordr*xwordc;
}

static void showHelp(void)
{
	clrscr();
	puts("Move around the board with the cursor keys.");
	puts("\nSQUARE TYPES:\n");
	puts("     ?    Set square to empty");
	puts("     #    Set square to black");
	puts("    A-Z   Set square to contain this letter");
	puts("     ^    Mark square as start of a new down word (multi-word clue)");
	puts("     >    Mark square as start of a new across word (multi-word clue)");
	puts("\nBOARD LAYOUT COPYING/REFLECTING:\n");
	puts("     `    Mirror-reflect top half of board setup left-to-right");
	puts("     '    Copy left half board setup to right half");
	puts("     \"    Copy top half board setup to bottom half");
	puts("\n                                                     Press a key...");
	fflush(stdout);
	while (!kbhit());
	getch();
	clrscr();
	puts("\nADMISSIBLE LETTER RESTRICTION COMPUTING:\n");
	puts("    BkSpc Recompute global letter restrictions from scratch");
	puts("    Enter Recompute global restrictions starting from current ones");
	puts("    Space Clear any computed global restrictions");
	puts("    0/1   Turn restriction computing off/on (for fast cursor movement)");
	puts("     =    Accept suggested letters (shown in red)");
	puts("\nDICTIONARY OVERRIDING (for words not in dictionary):\n");
	puts("     |    Don't limit current square to admissible down words");
	puts("     -    Don't limit current square to admissible across words");
	puts("     +    Don't limit current square by dictionary at all");
	puts("\nOTHER OPTIONS:\n");
	puts("    ESC   Quit and save board to XWORD.OUT file");
	puts("     @    Start an XWord dictionary consultation");
	puts("    ^A    Step through all possible across words for current square");
	puts("    ^D    Step through all possible down words for current square");
	puts("\n                                            Press a key to return...");
	fflush(stdout);
	while (!kbhit());
	getch();
	clrscr();
	drawBoard();
}

static int computeLocalCheck(int r, int c, int crunch, int dir)
{
	ulong mask = globalChk[r][c];
	if (xwordTbl[r][c] == '#')
		globalChk[r][c] = aChk[r][c] = dChk[r][c] = 0l;
	else if (xwordTbl[r][c] != '?' && (xwordFlag[r][c]&PUT)==0)
		globalChk[r][c] = aChk[r][c] = dChk[r][c] = 1l << (xwordTbl[r][c]-'A');
	else if (crunch)
	{
		if (crunch==2)
		{
			globalChk[r][c] = 0x3FFFFFFFl;
			dir = ANYA|ANYD;
		}
		computeCrossCheck(r,c,dir);
	}
	else aChk[r][c] = dChk[r][c] = globalChk[r][c];
	return (mask!=globalChk[r][c]);
}

static void saveBoard(void)
{
	FILE *outfp;
	int r, c;
	outfp = fopen("xword.out", "wt");
	if (outfp)
	{
		fprintf(outfp,"%d %d\n\n; Created by XWord\n\n", xwordr, xwordc);
		for (r=0; r<xwordr; r++)
		{
			for (c=0; c<xwordc; c++)
			{
				if (xwordTbl[r][c]=='?' || (xwordFlag[r][c]&PUT))
					fputc('.', outfp);
				else fputc(xwordTbl[r][c], outfp);
			}
			fputc('\n', outfp);
		}
		fputc('\n', outfp);
		fputc('\n', outfp);
		for (r=0; r<xwordr; r++)
		{
			for (c=0; c<xwordc; c++)
			{
				if (xwordFlag[r][c]&ANYA)
					fprintf(outfp, "%d %d -\n", r+1, c+1);
				if (xwordFlag[r][c]&ANYD)
					fprintf(outfp, "%d %d |\n", r+1, c+1);
				if (xwordFlag[r][c]&STARTD)
					fprintf(outfp, "%d %d ^\n", r+1, c+1);
				if (xwordFlag[r][c]&STARTA)
					fprintf(outfp, "%d %d >\n", r+1, c+1);
			}
		}
		fputc('\n', outfp);
		fclose(outfp);
	}
}

static void solveIt(int clear)
{
	int r, c, ch, i, autoskip = 0, r2, c2, backg = 0, crunch = 1;
	clrscr();
	localChk = 0;
	if (clear>0)
	{
		for (r=0; r<xwordr; r++)
			for (c=0; c<xwordc; c++)
			{
				xwordTbl[r][c] = '#';
				xwordFlag[r][c] = 0;
			}
	}
	clearChks(1);
	drawBoard();
	r = c = 0;
	for (;;)
	{
		xword_apos = 2;
		xword_dpos = 14;
		xword_acnt = xword_dcnt = 0l;
		showBlock(r, c, 1);
		gotoxy(5,24);
		cprintf("XWord v%s  Press F1 for help",VERSION);
		if (crunch)
		{
			unsigned long mask;
			localChk = 1;
			(void)computeLocalCheck(r, c, 1, ANYA|ANYD);
			localChk = 0;
			mask = globalChk[r][c];
			gotoxy(5,23);
			cputs("Valid choices:                             ");
			gotoxy(20,23);
			for (i=0; i<26;i++)
				if (mask & (1l << i))
					putch('A'+i);
			while (!kbhit())
			{
				backg = backg % (xwordr*xwordc);
				(void)(computeLocalCheck(backg/xwordc, backg%xwordc,
					1, ANYA|ANYD));
				backg++; /* move to next */
			}
		}
		ch = my_getch();
		showBlock(r, c, 0);
		switch(ch)
		{
		case '0': crunch = 0;
			break;
		case '1': crunch = 1;
			break;
		case 1:
			localChk = 2;
			xword_apos = 2;
			(void)computeLocalCheck(r, c, 1, ANYA);
			localChk = 0;
			break;
		case 4:
			localChk = 2;
			xword_dpos = 14;
			(void)computeLocalCheck(r, c, 1, ANYD);
			localChk = 0;
			break;
		case '@':
			clrscr();
			Consult();
			clrscr();
			drawBoard();
			break;
		case '+':
			if ((xwordFlag[r][c] & (ANYA|ANYD)) == (ANYA|ANYD))
				xwordFlag[r][c] &= ~(ANYA|ANYD);
			else
				xwordFlag[r][c] |= (ANYA|ANYD);
			goto clr_and_skip;
		case '|':
			if (xwordFlag[r][c] & ANYD)
				xwordFlag[r][c] &= ~ANYD;
			else
				xwordFlag[r][c] |= ANYD;
			clearChks(1);
			goto clr_and_skip;
		case '-':
			if (xwordFlag[r][c] & ANYA)
				xwordFlag[r][c] &= ~ANYA;
			else
				xwordFlag[r][c] |= ANYA;
			clearChks(1);
			goto clr_and_skip;
		case '>':
			if (xwordFlag[r][c] & STARTA)
				xwordFlag[r][c] &= ~STARTA;
			else
				xwordFlag[r][c] |= STARTA;
			goto clr_and_skip;
		case '^':
			if (xwordFlag[r][c] & STARTD)
				xwordFlag[r][c] &= ~STARTD;
			else
				xwordFlag[r][c] |= STARTD;
			goto clr_and_skip;
		case '\'':
			/* reflect left half to right half */
			for (r2=0; r2<=xwordr; r2++)
				for (c2=0; c2<(xwordc/2); c2++)
					xwordTbl[r2][xwordc-c2-1] =
						(xwordTbl[r2][c2]=='#') ? '#' : '?';
			drawBoard();
			clearChks(1);
			break;
		case '"':
			/* reflect top half to bottom half */
			for (r2=0; r2<(xwordr/2); r2++)
				for (c2=0; c2<xwordc; c2++)
					xwordTbl[xwordr-r2-1][xwordc-c2-1] =
						(xwordTbl[r2][c2]=='#') ? '#' : '?';
			drawBoard();
			clearChks(1);
			break;
		case '`':
			/* flip top half */
			for (r2=0; r2<=(xwordr/2); r2++)
				for (c2=0; c2<=(xwordc/2); c2++)
				{
					char ch = xwordTbl[r2][c2];
					xwordTbl[r2][c2] = xwordTbl[r2][xwordc-c2-1];
					xwordTbl[r2][xwordc-c2-1] = ch;
				}
			drawBoard();
			clearChks(1);
			break;
		case '=':
			/* accept all put letters */
			for (r2=0; r2<xwordr; r2++)
				for (c2=0; c2<xwordc; c2++)
					xwordFlag[r2][c2] &= ~PUT;
			drawBoard();
			break;
		case 8:
			clearChks(1);
			/* fall thru... */
		case 13:
			computeGlobalChecks();
			clearWrds();
			break;
		case 27:
			gotoxy(5,23);
			cputs("Do you want to quit?                       ");
			ch = my_getch();
			if (ch=='y' || ch=='Y')
			{
				saveBoard();
				gotoxy(1,24);puts("");
				return;
			}
			break;
		case KEY_F1:
			showHelp();
			break;
		case ' ':
		clr_and_skip:
			clearChks(1);
		skip:
			showBlock(r, c, 0);
			if (autoskip)
			{
				if (c < (xwordc-1)) goto moveright;
				else if (r < (xwordr-1)) goto movedown;
			}
			else
			{
				if (r < (xwordr-1)) goto movedown;
				else if (c < (xwordc-1)) goto moveright;
			}
			break;
		case KEY_HOME:
			c = 0;
			goto moved;
		case KEY_END:
			c = xwordc-1;
			goto moved;
		case KEY_PGUP:
			r = 0;
			goto moved;
		case KEY_PGDN:
			r = xwordr-1;
			goto moved;
		movedown:
		case KEY_DOWN:
			if (++r == xwordr) --r;
			autoskip = 0;
			goto moved;
		moveup:
		case KEY_UP:
			if (--r < 0) r=0;
			goto moved;
		moveleft:
		case KEY_LEFT:
			if (--c < 0) c=0;
			goto moved;
		moveright:
		case KEY_RIGHT:
			if (++c == xwordc) --c;
			autoskip = 1;
			goto moved;
		moved:
			clearWrds();
			break;
		default:
			if (ch>='a' && ch<='z') ch -= 'a'-'A';
			if (ch=='?' || ch=='#' || (ch>='A' && ch<='Z'))
			{
				xwordFlag[r][c] &= ~PUT;
				if (xwordTbl[r][c]!=ch)
				{
					if (ch=='?') globalChk[r][c] = 0x3FFFFFFFl;
					if (xwordTbl[r][c]!='?' || (ch<'A' || ch>'Z'))
						clearChks(1);
					else
					{
						/* make a sensible decision about where to move */
						/* if meant to go right but can't, change direction to down */
						if (autoskip && ( c==(xwordc-1) || xwordTbl[r][c+1]=='#'))
							autoskip = 0;
						/* if meant to go down but can't, change direction to right */
						else if (!autoskip && ( r==(xwordr-1) || xwordTbl[r+1][c]=='#'))
							autoskip = 1;
					}
					xwordTbl[r][c] = ch;
					showBlock(r, c, 0);
				}
				goto skip;
			}
			break;
		}
	}
}

void setMode(int md) /* 0 - normal; 1 - multi; 2 - blockword */
{
	minlength = 1;
	maxcnt = 0;
	switch(md)
	{
	case 0:
		multi = 0;
		blockword = 0;
		break;
	case 1:
		multi = 1;
		blockword = 0;
		break;
	case 2:
		blockword = 1;
		multi = 0;
		break;
	}
}

static void Consult(void)
{
	int i, ch;
	char word[256];
	for (;;) {

		/* Get the pattern, make upper case and strip off the newline */

		printf("%s or command? ", blockword?"Seed Word":(multi?"Letters":"Pattern"));
		fflush(stdout);
		fgets(word,256,stdin);
		i = strlen(word) - 1;
		word[i]=0;
		if (i==1 ||
		   ((i==2 || i==3) && (word[0]=='m' || word[0]=='M') && isdigit(word[1])))
		{
			if (*word=='?' || *word=='?')
			{
				puts("There are three modes: consult, blockword, and multi-word anagram.");
				puts("Single letter commands toggle various settings on/off. These are:\n");
				puts("Consult Mode (the default; A,L,l all enter this mode):");
				puts(" A - Show words matching anagrams of patterns");
				puts(" L - Show words matching any leading subpattern of pattern");
				puts(" l - Show words matching any trailing subpattern of pattern");
				puts(" R - Avoid showing repeats (misses some matches, tho...)\n");
				puts(" Use ? for an unknown character, [] for a range (! inverts range)");
				puts(" and a * in front to force use of a character/range\n");
				puts("B - Enter blockword mode (not a toggle)");
				puts(" G - `In-progress' graphic blockword display");
				puts(" D - toggle 1-second delay when a match is found\n");
				puts("M[#[#]] - Enter multi-word anagram mode (not a toggle)\n");
				puts("W - Wait for keypress after every 24 lines");
				puts("F - Log results to file XWORD.LOG");
				puts("Ctrl-Break at any time interrupts the matching process.");
				puts("You can recall and reuse previous inputs with H and 0-9.\n");
				printf("Current mode: %s        ", multi ? "Multi-word anagram"
						: (blockword ? "Blockword" : "Consult"));
				printf("Current settings: %c%c%c%c%c%c%c%c%c%d%d\n", logfp?'F':' ',
					anagrams?'A':' ', repeats?' ':'R',
					anylength?(anylength==2 ? 'l':'L'):' ',
					wait?'W':' ',
					blockword?'B':' ', grafik?'G':' ',
					deelay?'D':' ',
					multi?'M':' ',
					minlength, maxcnt);
			}
			else if (*word=='Q' || *word=='q') break;
			else if (*word=='F' || *word=='f') toggleLog();
			else if (*word=='B' || *word=='b')
				setMode(2);
			else if (*word=='W' || *word=='w')
				wait = 1 - wait;
			else if (*word=='G' || *word=='g')
				grafik = 1 - grafik;
			else if (*word=='A' || *word=='a')
			{
				setMode(0);
				anagrams = 1 - anagrams;
			}
			else if (*word=='R' || *word=='r')
				repeats = 1 - repeats;
			else if (*word=='L')
			{
				setMode(0);
				anylength = (anylength==1) ? 0 : 1;
			}
			else if (*word=='l')
			{
				setMode(0);
				anylength = (anylength==2) ? 0 : 2;
			}
			else if (*word=='M' || *word=='m')
			{
				setMode(1);
				if (i>=2)
					minlength = word[1]-'0';
				if (i==3)
					maxcnt = word[2]-'0';
			}
			else if (*word=='D' || *word=='d')
				deelay = 1 - deelay;
			else if (*word=='H' || *word=='h')
			{
				int h;
				for (h = 0; h < HIST_LINES; h++)
				{
					if (history[h][0])
						printf("%d %s\n", h+1, history[h]);
					else break;
				}
			}
			else if (isdigit(*word))
			{
				int h = atoi(word) - 1;
				if (h >= 0 && h < HIST_LINES &&
					history[h][0])
				{
					strcpy(word, history[h]);
					goto reuse_pattern;
				}
			}
			continue;
		}
		else strupr(word);
		strcpy(history[histpos++], word);
		if (histpos==HIST_LINES)
			histpos = 0;
	reuse_pattern:
	
		if (blockword && grafik && !logfp) toggleLog();

		/* Find the matches */
	
		printf("Looking up *%s*\n",word);
		match = 0l;
		line = 0;
		abortMatch = 0;
		if (multi)
		{
			/* Strip out any excluded letters */
			char *s = strchr(word,'-');
			if (s)
			{
				*s++ = 0;
				while (*s)
				{
					if (*s>='A' && *s<='Z')
					{
						char *t = word;
						while (*t)
						{
							if (*t == *s)
							{
								*t = ' ';
								break;
							}
							t++;
						}
					}
					s++;
				}
			}
			lastMulti[0] = '\0';
			if (multiInit(word) == 0)
				multiRecurse(1, 1, 1, 1, 1);
			else puts("Invalid character in words!");
		}
		else if (blockword) WordBlock(strlen(word), word);
		else matchPattern(word,anagrams,anylength,repeats,1);
		if (bufpos)
			flushbuffer();
		printf("\b%ld matches found\n",match);
	}
	if (logfp) fclose(logfp);
}

static void printIntro(void)
{
	char buff[80];
	clrscr();
#ifdef DEMO_VERSION
	fprintf(stderr,"XWord v%s\n(c) 1994 by Graham Wheeler, All Rights Reserved\nRegistered to %s\n", VERSION, Register);
	sprintf(buff,"XWord v%s Serial #%s UNREGISTERED DEMO COPY\n",
		VERSION,SERIAL);
	puts(buff);
	puts("This software is fully copyrighted and may not be hired out or sold");
	puts("without the express permission of the author. This restricted version may be");
	puts("freely copied and given away provided this is in its complete form with");
	puts("all original files intact and unmodified, AS PART OF THE WORDSWORTH");
	puts("DISTRIBUTION. XWord MAY NOT BE DISTRIBUTED SEPARATELY FROM WORDSWORTH");
	puts("For details on how to order the full versions of XWord/WW by becoming");
	puts("a registered user, run WordsWorth with a -R argument. This will create");
	puts("a file called REGISTER.DOC with further details.");
	sleep(10);
	puts("If you have read and understood the conditions above and");
	puts("agree to them, press ENTER or RETURN to begin playing...\n");
	puts("Read the file XWORD.DOC for instructions on how to use XWord.");
	while (!kbhit());
	(void)getchar();
#else
	sprintf(buff,"XWord v%s Serial #%s Registered to: %s\n",
		VERSION,SERIAL,Register);
	puts(buff);
	puts("\n(c) Graham Wheeler 1994. All Rights Reserved.\n");
	sleep(1);
#endif
}

/*********************************************************************/

static void useage(void)
{
	fprintf(stderr,"Usage: xword [-a] [-A] [-l] [-L] [<dictionary>]\n");
	fprintf(stderr,"   or: xword -M[#[#]] [<dictionary>]\n");
	fprintf(stderr,"   or: xword -B [<dictionary>]\n");
	fprintf(stderr,"   or: xword -X <board file> [<dictionary>]\n");
	fprintf(stderr,"   or: xword -X <rows> <cols> [<dictionary>]\n");
#ifdef DEBUG
	fprintf(stderr,"   or: xword -D [<dictionary>]\n");
#endif
	fprintf(stderr,"\tIf no <dictionary> is specified then WW.DIC is used.\n");
	fprintf(stderr,"\n\tSee XWORD.DOC for full information.\n");
	exit(0);
}

int HandleBreak(void)
{
	fprintf(stderr, "Aborting match...\n");
	abortMatch=1;
	return 1; /* continue execution */
}

void main(int argc, char *argv[])
{
	int i;
#ifdef DEBUG
	int dump = 0;
#endif
	for (i= 0; i < HIST_LINES; i++)
		history[i][0] = '\0';
	i = -1;
	strcpy(dictName,"wwbig.dic");
	do
	{
		i++;
		Register[i] -= (char)0x80;
	} while (Register[i]);
	printIntro();
	i = 1;
	while (i<argc)
	{
		if (argv[i][0]=='-')
			switch(argv[i][1])
			{
				case 'A':
					setMode(0);
					anagrams = 1;
					repeats = 0;
					break;
				case 'a':
					setMode(0);
					anagrams = 1;
					break;
				case 'b':
				case 'B':
					setMode(2);
					break;
				case 'g':
				case 'G':
					grafik = 1;
					break;
				case 'l':
					setMode(0);
					anylength = 2;
					break;
				case 'L':
					setMode(0);
					anylength = 1;
					break;
				case 'm':
				case 'M':
					setMode(1);
					if (isdigit(argv[i][2]))
					{
						minlength = argv[i][2]-'0';
						if (isdigit(argv[i][3]))
							maxcnt = argv[i][3]-'0';
					}
					repeats = 0;
					break;
				case 'x':
				case 'X':
				 	i++;
				 	if (i==argc) useage();
				 	if (isdigit(argv[i][0]))
					{
				 		xwordr = atoi(argv[i++]);
					 	if (i==argc) useage();
				 		xwordc = atoi(argv[i]);
						xword = 1;
					}
				 	else xword = -loadBoard(argv[i]);
					break;
#ifdef DEBUG
				case 'D':
					dump = 1;
					break;
#endif
				default:
					useage();
			}
		else if (i==(argc-1)) strcpy(dictName,argv[i]);
		else useage();
		i++;
	}
	if ((multi + ((anylength?1:0)|anagrams) + (xword?1:0))>1) useage();
	puts("Loading dictionary");
#if 1
	if (loadDawg(Register)==0)
	{
		extern char dawgError[];
		fprintf(stderr, "Dictionary load failed: %s\n", dawgError);
		exit(0);
	}
#else
	if (loadDawg("")==0) exit(0); /* HAX FOR DEVELOPMENT ONLY!!!!! */
#endif
	/* Make A and I words */
	MarkWordEnd(1);
	MarkWordEnd(9);
#ifdef DEBUG
	if (dump)
	{
		dumpDict();
		exit(0);
	}
#endif
#if __MSDOS__
	ctrlbrk(HandleBreak);
#else
	/* use signal */
#endif
	if (xword) solveIt(xword);
	else Consult();
}



