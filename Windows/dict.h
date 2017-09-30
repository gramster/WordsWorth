/*
 * The DAWG definitions
 *
 * (c) Graham Wheeler, 1993-1996
 */

#ifndef _DICT_H
#define _DICT_H

#if (defined(W32) || defined(W16))
#include <windows.h>
#endif
#if !defined(W16)
#  if (defined(W32) || !defined(__MSDOS__))
#    define huge
#  endif
#endif

#define PAT_USED	1
#define PAT_MANDATORY	2

#define USED	(0x80000000l)

#define USEALL	0
#define PREFIX	1
#define SUFFIX	2
#define MULTI	3

#define MAXPATLEN	20 // in places, not the length of the pat string

void BuildAnagramVector(char *anagram, short acnt[]);
unsigned long BuildAnagramMask(short acnt[]);

class Node
{
    unsigned long v;
  public:
    inline Node() : v(0l) {}
    inline Node(unsigned long v_in) : v(v_in) {}
    inline Node(const Node &n) : v(n.v) {}
    inline unsigned long NextNode() const
    {
#ifdef BIGDICT
	return (v & 0xFFFFFFL);	// 24-bit indices
#else
	return (v & 0xFFFF); // 16-bit indices 
#endif
    }
    inline int NodeIndex() const
    {
	return ((v >> 24) & 0x1F);
    }
    inline int NodeChar() const
    {
	return (NodeIndex()+'A'-1);
    }
    inline int IsWordEnd() const
    {
	return ((v >> 29) & 1);
    }
    inline int IsLastChild() const
    {
	return ((v >> 30) & 1);
    }
};

//---------------------------------------------------------------------
// simple incidence hash table

class Hash
{
    int sz, empty;
    unsigned short *vector;
    unsigned long Key(char *word);
  public:
    Hash(long size);
    void Empty();
    void Add(char *word);
    int Lookup(char *word);
    ~Hash();
};

class Dictionary
{
    Hash *thash;
    char *anag;
    short anagcnt[26];

    FILE *logfp;
#if (defined(W32) || defined(W16))
    HGLOBAL memhandle;
#endif
    Node huge *edges;
    char errormessage[256];
    long numnodes;

    int GetThesaurusTopics(char *word, short topics[]);
    Hash *GetThesaurusWords(short t[], int nt, int wlen=0, 
    				unsigned long *mask = 0);

    // used in pattern matching and anagrams
    unsigned long vec[MAXPATLEN], varpool[MAXPATLEN], pmask;
    int vlen, plen;
    int mincnt, maxcnt, minlen, maxlen;
    int allocpool[26], fixedpool[26], need[26], varmax[26];
    char word[256], lastmulti[256];
    long match;
    // node stack for non-recursive matching
    unsigned long nstk[80];
    int sp, start, stop, smult, vecoff, swcnt;
    // state variables for successively calling the iterative
    // matching routines.
    int numlens, lennow, wlen[32], vpos[32], mtyp;
    int longest;

    void RecursiveDump(int pos, long n, FILE *fp = stdout);
    void CheckMulti(char *word);

    // prepare dictionary for consultation

    void PrepPattern(char *pat, int type_in, 
			int minlen_in, int maxlen_in, 
			int mincnt_in, int maxcnt_in,
			char *topics_in = 0);

    // write out a match. Also does thesaurus based exclusions

    void LogSingle(char *word);

    // Non recursive matching routines. These are slower but easier
    // to do as background task

    int GrabLetter();
    void ReplaceLetter();
    void InitStack(int len, int first_vec, int allow_multi);
    char *GetWord(int len);
    void LogWord();
    int NextSWord();
    int NextMWord();

    // Recursive matching routines

    void FindChoicesRecursively(int pos, long n, char *word_in,
				char *word_tmp, unsigned long *masks);
    void Recurse(int pos, long n);
    void RecursiveChoose(int l, int pos);
    int FindPoolAllocation(unsigned long used);
    int RemoveFromPool(int c);
    void ReplaceInPool(int c);
    char *NextPat(char *pat);
    void SingleRecurse(long n, int wordpos, int vecpos, int len);
    void MultiRecurse(long n, int vecpos, int respos, int wlen, int cnt);
    void Free();
    void AllocateMem(long numnodes);
    void RecursiveLength(int pos, long n);
public:
    Dictionary();
    char *Load(char *dictname,
    		   unsigned char *key,
		   int keylen = 8);
    char *Translate(char *dictname, unsigned char *fromkey,
				unsigned char *tokey, int keylen = 8);
    int Lookup(char *word);
    int FindConstraintsLookup(char *word, unsigned long &mask);
    void MatchPattern(char *pat, int type, int minlen_in=0, int maxlen_in=0, 
			int mincnt_in = 0, int maxcnt_in = 0,
			char *topics_in = 0);
    void StartConsult(char *pat, int type, int minlen_in=0, int maxlen_in=0, 
			int mincnt_in=0, int maxcnt_in=0,
			char *topics_in = 0);
    int NextConsult();
    int NextConsult(int count);
    void Dump(FILE *fp = stdout);
    void RawDump();
    inline long Matches() const
    {
	return match;
    }
    inline void SetLogFile(FILE *fp_in)
    {
	logfp = fp_in;
    }
    inline FILE *GetLogFile() const
    {
	return logfp;
    }
    inline Node &GetNode(long n) const
    {
	return edges[n];
    }
    int Longest();
    void SetAnagramConstraint(char *anagram);
    int IsAnagram(char *word);
    Hash *GetThesaurusWords(char *topiclist, int wlen=0,
    				unsigned long *mask = 0);
    ~Dictionary();
};

extern int abortmatch;

#endif _DICT_H

