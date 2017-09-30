#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <ctype.h>

	typedef unsigned long ulong;

#ifndef __MSDOS__ /* for me to build real dictionaries */
#define huge
#ifdef BIGDICT
#define MAXNODES	80000L
	typedef unsigned long nodenum;
#else
#define MAXNODES	60000
	typedef unsigned short nodenum;
#endif /* BIGDICT */

void strupr(char *s)
{
    while (*s) { if (islower(*s)) *s = toupper(*s); s++; }
}


#else /* MSDOS */
#define MAXNODES	500
	typedef ushort nodenum;
#endif

	typedef unsigned long NODE;

NODE	node[MAXNODES][27];

#define FREE		((ulong)0x80000000l)
#define TERMINAL	((ulong)0x40000000l)

FILE *dawg = (FILE *)0;
FILE *lex = stdin;

nodenum	botnode = (nodenum)1,
	topnode= (nodenum)0,
	freeNodes = MAXNODES-1l,
	usednodes;

nodenum	Index[MAXNODES];

#define MAXLEN	20	/* Max word length */

char word[MAXLEN+1],lastword[MAXLEN+1];
nodenum	nodeStk[MAXLEN];
int wcnt = 0;
int stkTop=0;
int must_abort;

#ifdef UNIX

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

void SaveState(void)
{
	int i;
	FILE *stf = fopen("buildict.tmp", "w");
	if (stf == 0)
	{
		fprintf(stderr, "Can't open state file\n");
		return;
	}
	fwrite(&botnode, sizeof(botnode), 1, stf);
	fwrite(&topnode, sizeof(topnode), 1, stf);
	fwrite(&freeNodes, sizeof(freeNodes), 1, stf);
	fwrite(&usednodes, sizeof(usednodes), 1, stf);
	fwrite(word, sizeof(char), MAXLEN+1, stf);
	fwrite(lastword, sizeof(char), MAXLEN+1, stf);
	fwrite(&wcnt, sizeof(wcnt), 1, stf);
	fwrite(&stkTop, sizeof(stkTop), 1, stf);
	fwrite(nodeStk, sizeof(nodenum), stkTop+1, stf);
	for (i = 0; i <= topnode; i++)
		fwrite(node+i, sizeof(NODE), 27, stf);
	fwrite(Index, sizeof(nodenum), topnode+1, stf);
	fclose(stf);
	fprintf(stderr, "Saved state; resume with the -R flag\n");
}

void RestoreState(void)
{
	int i;
	FILE *stf = fopen("buildict.tmp", "r");
	fprintf(stderr, "Restoring state...\n");
	if (stf == 0)
	{
		fprintf(stderr, "Can't open state file\n");
		exit(-1);
		return;
	}
	fread(&botnode, sizeof(botnode), 1, stf);
	fread(&topnode, sizeof(topnode), 1, stf);
	fread(&freeNodes, sizeof(freeNodes), 1, stf);
	fread(&usednodes, sizeof(usednodes), 1, stf);
	fread(word, sizeof(char), MAXLEN+1, stf);
	fread(lastword, sizeof(char), MAXLEN+1, stf);
	fread(&wcnt, sizeof(wcnt), 1, stf);
	fread(&stkTop, sizeof(stkTop), 1, stf);
	fprintf(stderr, "Bot %d Top %d Free %d Used %d Cnt %d Stk %d Wrd %s\n",
		botnode, topnode, freeNodes, usednodes, wcnt, stkTop, lastword);
	fread(nodeStk, sizeof(nodenum), stkTop+1, stf);
	for (i = 0; i <= topnode; i++)
		fread(node+i, sizeof(NODE), 27, stf);
	fread(Index, sizeof(nodenum), topnode+1, stf);
	fclose(stf);
	/* Now skip forward in lex file to next word */
	for (i = wcnt; i--; )
		fgets(word, MAXLEN, lex);
	fprintf(stderr, "Resuming at %s\n", word);
}

void setup(void)
{
	nodenum i;
	int j;
	for (j=0;j<27;j++) node[0][j] = (nodenum)0;
	/* set up free list */
	for (i=1;i<MAXNODES;i++)
	{
		node[i][0] = FREE;
		node[i][1] = i+1;
	}
}

nodenum allocNode(void)
{
	nodenum i;
	int j;
	if (botnode >= MAXNODES)
	{
		fprintf(stderr,"Out of nodes!\n");
		exit(0);
	}
	i = botnode;
	freeNodes--;
	botnode = node[i][1];
	if (i>topnode) topnode = i;
	for (j=0;j<27;j++) node[i][j] = (NODE)0;
	return i;
}

#define nodeCmp(n1,n2)	(memcmp(node[n1], node[n2], 27*sizeof(NODE))==0)

/* change all refs to n2 to refer to n1 */

void relink(NODE n1, NODE n2)
{
	NODE huge *N;
	nodenum i;
	int j;
	for (i=0l; i<=topnode; i++)
	{
		N = node[i];
		if (N[0]&FREE) continue;
		for (N++,j=27 ; j>0 ; j--, N++)
		{
			if (*N==n2) *N=n1;
		}
	}
}

void printGraph(void)
{
	nodenum i;
	int j;
	for (i=0;i<=topnode;i++)
	{
		if (node[i][0] & FREE) continue;
		if (node[i][0] & TERMINAL) printf("Terminal ");
		printf("Node %ld\n",(ulong)i);
		for (j=1;j<27;j++)
		{
			if (node[i][j])
				printf("    Child %ld (%c)\n",(ulong)node[i][j],(char)(j+'A'-1));
		}
	}
}

void markGraph(nodenum n)
{
	int i;
	if (node[n][0]&FREE)
	{
		node[n][0] &= ~FREE;
		usednodes++;
	}
	for (i=1;i<27;i++)
		if (node[n][i])
		{
			markGraph(node[n][i]);
		}
}

void writeDawg(void)
{
	ushort j, chld, last;
	nodenum i, index = 1;
	for (i=0;i<=topnode;i++)
	{
		if (node[i][0]&FREE) continue;
		chld = 0;
		for (j=1;j<27;j++)
			if (node[i][j])
				chld++;
		if (chld)
		{
			Index[i] = index;
			index += chld;
		}
		else Index[i] = (nodenum)0;
	}
	for (i=0;i<=topnode;i++)
	{
		if (node[i][0]&FREE) continue;
		if (Index[i])
		{
#ifdef DEBUG
			printf("Writing node %ld, index %d\n",(ulong)i, Index[i]);
#endif
			for (j=26;j;j--)
			{
				if (node[i][j])
				{
					last = j;
					break;
				}
			}
			for (j=1;j<27;j++)
			{
				NODE ch = node[i][j];
				if (ch)
				{
					ulong v = (ulong)Index[ch];
					if (node[ch][0]&TERMINAL)
						v |= 0x20000000l;
					if (j==last)
						v |= 0x40000000l;
					v += ((ulong)j)<<24;
					fwrite(&v,1,sizeof(long),dawg);
#ifdef DEBUG
					printf("  Writing edge for %c (%08lX)\n",(char)(j+'A'-1),v);
#endif
				}
			}
		}
	}
}

int _mergeNodes(nodenum first, nodenum last, NODE n)
{
	nodenum i;
	for (i=first;i<=last;i++)
	{
		if (node[i][0]&FREE) continue;
		if (nodeCmp(i,n))
		{
			relink(i,n);
			node[(nodenum)n][0] = FREE;
			if (n<botnode)
			{
				node[(nodenum)n][1] = botnode;
				botnode = n;
			}
			else
			{
				i = n-1;
				while (i>=botnode)
				{
					if (node[i][0]&FREE)
					{
						node[n][1] = node[i][1];
						node[i][1] = n;
						break;
					}
					i--;
				}
			}
			freeNodes++;
			return 1;
		}
	}
	return 0; /* no match */
}

int mergeNodes(NODE n)
{
	/* This funny way of doing it is (hopefully!) a performance improvement
		as we no longer have to check for equality in nodeCmp */
	if (_mergeNodes(1, n-1, n)) return 1;
	return _mergeNodes(n+1, topnode, n);
}

void addWord(char *word, int pos)
{
	nodenum n = nodeStk[pos];
	printf("%d: %-16s Free %d\n", ++wcnt, word, freeNodes);
	while (word[pos])
	{
		int j = word[pos] - 'A' + 1;
		node[n][j] = allocNode();
		node[n][0]++;
/*		node[topnode][0] = 0; This is a bug I think! */
		n = node[n][j];
		pos++;
		nodeStk[pos] = n;
	}
	node[n][0] = TERMINAL;
	stkTop = pos;
}

/* compare two differing strings, return position of first difference*/

int cmpstr(char *s1, char *s2)
{
	int pos = 0;
	while ((s1[pos]==s2[pos]) && s1[pos]) pos++;
	if (s1[pos] && s1[pos]<s2[pos])
	{
		fprintf(stderr,"ERROR: input must be sorted - unlike %s and %s\n",
			s2,s1);
		exit(0);
	}
	return pos;	
}

static void useage(void)
{
	fprintf(stderr,"Useage: buildict [-R] [-o <output file>] [ <input file> ]\n");
	fprintf(stderr,"\n\tIf no input file is specified, standard input is used\n");
	fprintf(stderr,"\tIf no output file is specified, DAWG is used\n");
	exit(0);
}

static void HandleSignal(int);
typedef void (*handler)(int);

int main(int argc, char *argv[])
{
	ushort l;
	int i, pos;
	nodenum n;
	char word[80], *dname = "dawg";
	int restore = 0;
	fprintf(stderr, "Welcome to buildict!\n");
	for (i=1;i<argc;i++)
	{
		switch(argv[i][0])
		{
		case '-':
			switch(argv[i][1])
			{
			case 'o':
				dname = argv[++i];
				if (i>=argc) useage();
				break;
			case 'R':
				restore = 1;
				break;
			default:
				useage();
			}
			break;
		default:
			lex = fopen(argv[i],"r");
			if (lex==(FILE *)0)
			{
				fprintf(stderr,"Cannot open input file %s\n",
						argv[i]);
				exit(-1);
			}
			else if (++i != argc)
				useage();
			break;
		}
	}
	dawg = fopen(dname,"wb");
	if (dawg==(FILE *)0)
	{
		fprintf(stderr,"Cannot open output file %s!\n",dname);
		exit(-2);
	}
	fprintf(stderr, "Initialising...\n");
	setup();

	/* clear the stack containing last word */

	lastword[0]=0;
	for (i=0;i<=MAXLEN;i++) nodeStk[i]=0;

	/* add words */

	must_abort = 0;
	if (restore)
		RestoreState();
	(void)signal(SIGINT, (handler)HandleSignal);
#ifndef __MSDOS__
	(void)signal(SIGQUIT, (handler)HandleSignal);
#endif
	while (!must_abort)
	{
		fgets(word,80,lex);
		if (feof(lex)) break;
		strupr(word);
		l = strlen(word)-1;

		/* strip trailing garbage */

		while (l>0 && (word[l]<'A' || word[l]>'Z')) l--;

		/* l now indexes the last letter */

		word[l+1] = 0;
		if (l >= MAXLEN)
		{
		    fprintf(stderr, "Word is too long! - %s\n", word);
		    continue;
		}

		if (l<1) /* less than 2 letters */
		{
			if (l) fprintf(stderr,"WARNING: word %s is too short; skipping\n",word);
			continue;
		}

		/* Find the point of departure from last word */

		pos = cmpstr(word,lastword);
		strcpy(lastword,word);

		/* `bottom-out' of tree to point of departure, merging
			any completed subtrees */

		i = stkTop;
		while (i>pos)
		{
			if (mergeNodes(nodeStk[i]) == 0) break;
			i--;
		}
		if (l>0) addWord(word, pos);
		else break;
#ifdef DEBUG
/*		printGraph();*/
#endif
	}
	if (must_abort)
	{
		SaveState();
		exit(0);
	}

	/* Make sure we've bottomed out... */

	i = stkTop;
	while (i>0) mergeNodes(nodeStk[i--]);

	fclose(lex);
	printf("Setting all nodes to FREE\n");
	for (n=0;n<=topnode;n++)
		if (node[n][0] & FREE)
			node[n][1] = 0;	/* clear next free pointer */
		else node[n][0]|=FREE;
	printf("Marking graph\n");
	usednodes = (nodenum)0;
	markGraph((nodenum)0);
#ifdef DEBUG
	printGraph();
#endif
	/* We now have to convert the DAWG into an array of edges
	// in a file.  */
	printf("Writing dawg to file\n");
	writeDawg();
	fclose(dawg);
	printf("Max space used: %%%5.2f,  Final space used %%%5.2f\n",
		(double)topnode*100./(double)MAXNODES,
		(double)usednodes*100./(double)MAXNODES);
	return 0;
}

static void HandleSignal(int signo)
{
	must_abort = 1;
	fprintf(stderr, "Interrupted!\n");
	return;
}

