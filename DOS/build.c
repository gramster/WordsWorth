#include <stdio.h>

typedef unsigned short ushort;
typedef unsigned long ulong;

#ifdef UNIX
#define MAXNODES	20000
typedef ulong NODE;
#define FREE		((ulong)0x80000000l)
#define TERMINAL	((ulong)0x40000000l)
#define void
#define huge
#else
#define MAXNODES	500
typedef ushort NODE;
#define FREE		((ushort)0x8000l)
#define TERMINAL	((ushort)0x4000l)
#endif

NODE huge node[MAXNODES][27];
ushort Index[MAXNODES];

NODE	botnode = (NODE)1,
	topnode= (NODE)0,
	freeNodes = MAXNODES;

FILE *lex, *dawg;

#define MAXLEN	20	/* Max word length */

char word[MAXLEN+1],lastword[MAXLEN+1];
NODE	nodeStk[MAXLEN];


#ifdef UNIX

void strupr(word) char *word; {
	while (*word){
		if (*word>='a' && *word<='z')
			*word = *word-'a'+'A';
		word++;
	}
}

#endif

void setup() {
	NODE i;
	int j;
	for (j=0;j<27;j++) node[0][j] = (NODE)0;
	for (i=1;i<MAXNODES;i++) {
		node[i][0] = FREE;
		for (j=1;j<27;j++) node[i][j] = (NODE)0;
#ifdef DEBUG
		if (i%100==0) printf("Set up nodes %ld\n",(ulong)i);
#endif
	}
}

NODE allocNode(void) {
	NODE i;
	for (i=botnode;i<MAXNODES;i++) 
		if (node[i][0] & FREE) {
			int j;
			for (j=0;j<27;j++) node[i][j] = (NODE)0;
			if (i>topnode) topnode=i;
			botnode = i; /* Start next search from here if no frees */
			freeNodes--;
			return i;
		}
	fprintf(stderr,"Out of nodes!\n");
	exit(0);
}

int nodeCmp(n1, n2) NODE n1, n2; {
	int i;
	NODE huge *N1, huge *N2, n3, n4;
	if (n1==n2) return 1;
	N1 = node[n1];
	N2 = node[n2];
	if (N1[0]!=N2[0]) return 0;
	for (i=1;i<=26;i++) {
		n3 = N1[i];
		n4 = N2[i];
		if (n3!=n4) 
		if (n3) {
			if (n4) { /* both have children? */
				if (n3!=n4)
					if (nodeCmp(n3,n4)==0)
						return 0;
			} else return 0;
		} else if (n4) return 0;
		/* else neither have this child */
	}
	return 1;
}

/* change all refs to n2 to refer to n1 */

void relink(n1, n2) NODE n1, n2; {
	NODE i, huge *N;
	int j;
	for (i=0l;i<=topnode;i++) {
		N = node[i];
		if (N[0]&FREE) continue;
		for (N++,j=27;j>0;j--, N++) {
			if (*N==n2) *N=n1;
		}
	}
}

void printGraph(void) {
	NODE i;
	int j;
	for (i=0;i<=topnode;i++) {
		if (node[i][0] & FREE) continue;
		if (node[i][0] & TERMINAL) printf("Terminal ");
		printf("Node %ld\n",(ulong)i);
		for (j=1;j<27;j++) {
			if (node[i][j])
				printf("    Child %ld (%c)\n",(ulong)node[i][j],(char)(j+'A'-1));
		}
	}
}

ushort usednodes;

void markGraph(n) NODE n; {
	int i;
	if (node[n][0]&FREE) {
		node[n][0] -= FREE;
		usednodes++;
	}
	for (i=1;i<27;i++)
		if (node[n][i]) {
			markGraph(node[n][i]);
		}
}

void writeDawg(void) {
	ushort index=1, j, chld, last;
	NODE i;
	for (i=0;i<=topnode;i++) {
		if (node[i][0]&FREE) continue;
		chld = 0;
		for (j=1;j<27;j++) if (node[i][j]) chld++;
		if (chld) {
			Index[i] = index;
			index += chld;
		} else Index[i] = 0;
	}
	dawg = fopen("dawg","wb");
	for (i=0;i<=topnode;i++) {
		if (node[i][0]&FREE) continue;
		if (Index[i]) {
#ifdef DEBUG
			printf("Writing node %ld, index %d\n",(ulong)i, Index[i]);
#endif
			for (j=26;j;j--) {
				if (node[i][j]) {
					last = j;
					break;
				}
			}
			for (j=1;j<27;j++) {
				NODE ch = node[i][j];
				if (ch) {
					ulong v = (ulong)Index[ch];
					if (node[ch][0]&TERMINAL)
						v |= 0x10000l;
					if (j==last)
						v |= 0x20000l;
					v += ((ulong)j)<<24;
					fwrite(&v,1,sizeof(long),dawg);
#ifdef DEBUG
					printf("  Writing edge for %c (%08lX)\n",(char)(j+'A'-1),v);
#endif
				}
			}
		}
	}
	fclose(dawg);
}

int mergeNodes(n) NODE n; {
	NODE i;
	for (i=1;i<=topnode;i++) {
		if (node[i][0]&FREE) continue;
		if (nodeCmp(i,n)) {
			if (i!=n) {
				relink(i,n);
				node[n][0] = FREE;
				if (n<botnode) botnode = n;
				freeNodes++;
				return 1;
			}
		}
	}
	return 0; /* no match */
}

int stkTop=0;

void addWord(word, pos) char *word; int pos; {
	NODE n = nodeStk[pos];
#ifdef DEBUG
	printf("Adding %-16s Freenodes %ld\n",word,(ulong)freeNodes);
#endif
	while (word[pos]) {
		int j = word[pos] - 'A' + 1;
		node[n][j] = allocNode();
		node[n][0]++;
		node[topnode][0] = 0;
		n = node[n][j];
		pos++;
		nodeStk[pos] = n;
	}
	node[n][0] = TERMINAL;
	stkTop = pos;
}

/* compare two differing strings, return position of first difference*/

int cmpstr(s1,s2) char *s1, *s2; {
	int pos = 0;
	while ((s1[pos]==s2[pos]) && s1[pos]) pos++;
	return pos;	
}

main() {
	ushort l; int i, pos;
	char c='0', word[16];
	NODE n;
	lex = fopen("lex","rt");
	if (lex==NULL) exit(0);
	setup();
	lastword[0]=0;
	for (i=0;i<=MAXLEN;i++) nodeStk[i]=0;
	for (;;) {
		fgets(word,MAXLEN,lex);
		if (feof(lex)) break;
		strupr(word);
		l = strlen(word)-1;
		while (l>0 && (word[l]<'A' || word[l]>'Z')) l--;
		/* l now indexes the last letter */
		word[l+1] = 0;
		if (l<=0) word[0] = 0;
		pos = cmpstr(word,lastword);
		strcpy(lastword,word);
		i = stkTop;
		while (i>pos) {
			if (mergeNodes(nodeStk[i]) == 0) break;
			i--;
		}
		if (l>0) addWord(word, pos);
		else break;
#ifdef DEBUG
/*		printGraph();*/
#endif
	}
	i = stkTop;
	while (i>0) mergeNodes(nodeStk[i--]); 
	fclose(lex);
	printf("Marking graph\n");
	for (n=0;n<=topnode;n++) node[n][0]|=FREE;
	usednodes = 0;
	markGraph((NODE)0l);
#ifdef DEBUG
	printGraph();
#endif
	/* We now have to convert the DAWG into an array of edges
	// in a file.  */
	writeDawg();
	printf("Max nodes used: %ld,  Final nodes %ld\n",(ulong)topnode,(ulong)usednodes);
}

