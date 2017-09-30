#ifndef DICBUILD_H
#define DICBUILD_H

typedef unsigned long ulong;
typedef unsigned long NODE;

#define FREE		((ulong)0x80000000l)
#define TERMINAL	((ulong)0x40000000l)

#ifdef BIGDICT
#define MAXNODES	80000L
	typedef unsigned long nodenum;
#else
#define MAXNODES	65000
	typedef unsigned short nodenum;
#endif /* BIGDICT */

typedef NODE TreeNode[27];

#define MAXLEN	20	/* Max word length */

class DictionaryBuilder
{ 
    class wxFrame *frame;
#if (defined(W32) || defined(W16))
    HGLOBAL nodehandle, indexhandle;
#endif
    TreeNode huge *node;
    nodenum huge *Index;
    nodenum botnode, topnode, freeNodes, usednodes;
    char word[MAXLEN+1],lastword[MAXLEN+1];
    nodenum nodeStk[MAXLEN];
    int wcnt;
    int stkTop;
    FILE *lex;
    FILE *dawg;
    int running;

    nodenum AllocNode();
    void Setup(FILE *restore);
    void SaveState();
    void RestoreState(FILE *lex);
    void Relink(NODE n1, NODE n2);
    void PrintGraph(FILE *fp);
    void MarkGraph(nodenum n);
    void WriteDawg(FILE *dawg, 
    		   unsigned char *key = "+\\2=dke;4fd5*^23!x$",
		   int keylen = 16);
    int  MergeNodes(nodenum first, nodenum last, NODE n);
    int  MergeNodes(NODE n);
    int  GetWord(FILE *lex);
    int AddWord(char *word, int pos);
    void BuildDict(FILE *lex, FILE *dawg, int restore);

    void DumpDict();
    void BuildDict(int restore);
    void Free();
    void AllocateMem(long numnodes);
  public:
    DictionaryBuilder(class wxFrame*,FILE*,FILE*,int);
    void DoWord();
    ~DictionaryBuilder();
};

#endif

