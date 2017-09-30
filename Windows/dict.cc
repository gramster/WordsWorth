#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#if __MSDOS__
#include <malloc.h>
#include <conio.h>
#include <alloc.h>
#else
#define huge
#define far
#endif
#include <assert.h>
#include <sys/stat.h>

#include "profile.h"
#include "blowfish.h"
//#include "register.h"
#include "dict.h"

#ifdef __MSDOS__
#  ifdef _Windows
#    include <windowsx.h>
#  endif
#endif

extern long MaxDicSize();

#define THASH		65536l	// topic hash bit vector

// External hooks to allow multitasking under Windoze, and to stop
// dictionary consults

int abortmatch = 0;

/* Node bit layout is now:
	XLTIIIIINNNNNNNNNNNNNNNNNNNNNNNN
   where X is reserved for future, I is the index, L the last child flag,
   T the terminal flag, and N the next node index
*/

#ifdef BIGDICT
#  define MAXNODES	91000l	/* big dictionary			*/
#else
#  define MAXNODES	65000	/* 16-bit index dictionary		*/
#endif

#define MAXADDNODES	500

extern void ShowProgress(int);

//---------------------------------------------------------------------
// Build a vector containing letter counts from a string. Used for
// anagram matches etc.

void BuildAnagramVector(char *anagram, short acnt[])
{
FPROF("BuildAnagramVector");
    for (int i = 0; i < 26; i++) 
        acnt[i] = 0;
    if (anagram)
    {
	while (*anagram)
	{
	    if (isupper(*anagram)) acnt[anagram[0]-'A']++;
	    else if (islower(*anagram)) acnt[anagram[0]-'a']++;
	    anagram++;
	}
    }
}

unsigned long BuildAnagramMask(short acnt[])
{
FPROF("BuildAnagramMask");
    unsigned long rtn = 0l;
    for (int i = 0; i < 26; i++) 
        if (acnt[i]>0) rtn |= 1l<<i;
    return rtn;
}

//---------------------------------------------------------------------
// simple incidence hash table

Hash::Hash(long size)
{
FPROF("Hash::Hash");
    sz = (size+15l)/16l;
    vector = new unsigned short[sz];
    Empty();
}

void Hash::Empty()
{
FPROF("Hash::Empty");
    memset(vector, 0, sz*sizeof(short));
    empty = 1;
}

unsigned long Hash::Key(char *word)
{
FPROF("Hash::Key");
    unsigned long h = 0l;
    while (*word)
	h = (h*26l + (*word++) - 'A') % 0xFFFFFFFEl;
    return h % (16l*(unsigned long)sz);
}

void Hash::Add(char *word)
{
FPROF("Hash::Add");
    unsigned long k = Key(word);
    vector[k>>4] |= (1 << (k&15));
    empty = 0;
}

int Hash::Lookup(char *word)
{
FPROF("Hash::Lookup");
    if (empty) return 1; // bypass if no matches
    unsigned long k = Key(word);
    return ((vector[k>>4] & (1 << (k&15))) != 0);
}

Hash::~Hash()
{
FPROF("Hash::~Hash");
    delete [] vector;
}



//---------------------------------------------------------------------

int Dictionary::GetThesaurusTopics(char *word, short topics[])
{
FPROF("Dictionary::GetThesaurusTopics");
    FILE *fp = fopen("thes.dat", "rb");
    if (fp == 0) return 0;
    int tnum = 0;
    while (!feof(fp))
    {
        char wordbuf[32];
	int ch, l = 0;
	for (;;) 
	{
	    char ch = fgetc(fp);
	    if (ch == 0) break;
	    else if (feof(fp)) goto done;
	    else wordbuf[l++] = ch;
	}
	wordbuf[l] = 0;
	if ((l%2) == 0) (void)fgetc(fp); // move to word boundary
	int match = (strcmp(wordbuf, word)==0);
	while (!feof(fp))
	{
	    short t;
	    if (fread(&t, sizeof(t), 1, fp) == 1 && t > 0)
	    {
	        if (match) topics[tnum++] = t;
	    }
	    else break;
	}
	if (match) break;
    }
  done:
    fclose(fp);
    return tnum;
}

Hash *Dictionary::GetThesaurusWords(short topics[], int nt, int wlen,
					unsigned long *mask)
{
FPROF("Dictionary::GetThesaurusWords1");
    Hash *hash = 0;
    if (nt > 0)
    {
        FILE *fp = fopen("thes.dat", "rb");
        if (fp == 0) return 0;
	hash = new Hash(THASH);
	if (hash == 0) { fclose(fp); return 0; }
	unsigned long *m = new unsigned long[wlen];
	for (int i = 0; i < wlen; i++) m[i] = 0l;
        while (!feof(fp))
        {
            char word[32];
	    int ch, l = 0;
	    while ((ch = fgetc(fp)) != 0)
	        if (feof(fp)) goto done_file;
	        else word[l++] = ch;
	    word[l] = 0;
	    if ((l%2) == 0) (void)fgetc(fp);
	    short t;
	    int done = 0;
	    while (fread(&t, sizeof(short), 1, fp)==1 && t != 0)
	    {
	        if (done) continue;
	        for (int i = 0; i < nt; i++)
		{
		    if (topics[i] == t)
		    {
		        if (wlen == 0 || strlen(word)==wlen)
			{
			    hash->Add(word);
			    for (int i = 0; i < wlen; i++)
			        m[i] |= 1l << (word[i]-'A');
			}
		        done = 1;
		        break;
		    }
		}
	    }
 	}
      done_file:
	fclose(fp);
        if (mask)
            for (int i = 0; i < wlen; i++)
	        mask[i] &= m[i];
    }
    return hash;
}

Hash *Dictionary::GetThesaurusWords(char *topiclist, int wlen, 
					unsigned long *mask)
{
FPROF("Dictionary::GetThesaurusWords2");
    int intersect = 1;
    if (topiclist[0]=='+' || topiclist[0]=='-')
    {
        intersect = (topiclist[0]=='-');
        topiclist++;
    }
    char tlist[256];
    strcpy(tlist, topiclist);
    strupr(tlist);
    int topiccnt = -1;
    short topics[128];
    for (char *s = strtok(tlist, ","); s; s = strtok(0, ","))
    {
        char w[32];
        if (sscanf(s, "%32s", w) != 1 || !isalpha(w[0]))
            continue;
        if (topiccnt<0)
            topiccnt = GetThesaurusTopics(w, topics);
        else
        {
            short nt[128];
            int tcnt = GetThesaurusTopics(w, nt);
	    if (intersect)
    	    {
    	        short res[128];
    	        int rescnt = 0;
    	        for (int i = 0; i < tcnt; i++)
    	        {
    	            for (int j = 0; j < topiccnt; j++)
    		        if (topics[j] == nt[i])
    		        {
    		            res[rescnt++] = nt[i];
    		            break;
    		        }
    	        }
    	        topiccnt = rescnt;
    	        for (int i = 0; i < rescnt; i++) 
    	            topics[i] = res[i];
    	    }
    	    else // union
    	    {
    	        for (int i = 0; i < tcnt; i++)
    	        {
    	            int j;
    	            for (j = 0; j < topiccnt; j++)
    		        if (topics[j] == nt[i])
    		            break;
    		    if (j == topiccnt && topiccnt<128)
    		        topics[topiccnt++] = nt[i];
    	    	}
    	    }
        }
    }
    return GetThesaurusWords(topics, topiccnt, wlen, mask);
}

//---------------------------------------------------------------------

Dictionary::Dictionary()
    : thash(0), anag(0), numnodes(0), edges(0), logfp(0)
{
FPROF("Dictionary::Dictionary");
}

void Dictionary::Free()
{
FPROF("Dictionary::Free");
    if (edges) 
    {
#if (defined(W32) || defined(W16))
	GlobalFree(edges);
#else // DOS
#  if __MSDOS__
	farfree(edges);
#  else // UNIX
	delete [] edges;
#  endif
#endif
        edges = 0;
    }
}

void Dictionary::AllocateMem(long numnodes)
{
FPROF("Dictionary::AllocateMem");
    Free();
#if (defined(W32) || defined(W16))
    edges = (Node FAR *)GlobalAlloc(GPTR, numnodes*((long)sizeof(Node)));
#else
#  if __MSDOS__
    edges = (Node huge *)farcalloc(numnodes,sizeof(Node));//DOS
#  else // UNIX
    edges = new Node[numnodes];
#  endif
#endif
}

char *Dictionary::Load(char *dictname, unsigned char *key, int keylen)
{
FPROF("Dictionary::Load");
    int i, n;
    FILE *dawg;
    dawg = fopen(dictname,"rb");
    if (dawg==0)
    {
    	sprintf(errormessage, "Failed to open dictionary %s! %.40s",
    			dictname, strerror(errno));
    	return errormessage;
    }
    struct stat st;
    if (fstat(fileno(dawg), &st) < 0)
    {
    	sprintf(errormessage, "fstat failed on dictionary file %s! %.40s",
    			dictname, strerror(errno));
	fclose(dawg);
    	return errormessage;
    }    
    numnodes = (st.st_size / sizeof(long))+1; /* add 1 for safety */
    if (numnodes > MaxDicSize())
    {
	fclose(dawg);
	return "Invalid format";
    }
    Blowfish *b = new Blowfish;
    if (b==0 || !b->SetKey(key, keylen)) 
    {
        delete b;
	fclose(dawg);
	return "Internal memory error";
    }
    AllocateMem(numnodes);
    if (edges==0)
    {
    	sprintf(errormessage,"Couldn't allocate dictionary of %ld bytes",
    		(long)(numnodes*sizeof(Node)));
	fclose(dawg);
    	return errormessage;
    }
    if ((n = fread(edges+1, sizeof(Node), numnodes-1, dawg)) >= 0)
    {
        for (int i = 0; i < n; i+=2)
	    b->DecipherBlock((unsigned long *)&edges[i+1],
			 (unsigned long *)&edges[i+2]);
    }
    delete b;
    // do a sanity check
    for (int v = 1; v <= 26; v++)
        if (edges[v].NodeIndex() != v || (edges[v].IsLastChild() ^ (v==26)))
	{
	    Free();
	    return "Invalid dictionary format";
    	}
    numnodes = n+1;
    fclose(dawg);
    return 0;
}

char *Dictionary::Translate(char *dictname, unsigned char *fromkey,
				unsigned char *tokey, int keylen)
{
FPROF("Dictionary::Translate");
    char *rtn = Load(dictname, fromkey, keylen);
    if (rtn) return rtn;
    else
    {
	Blowfish *b = new Blowfish;
	if (b==0) return "Memory allocation failure";
	if (!b->SetKey(tokey, keylen))
	{
	    delete b;
	    return "Internal error";
	}
#if (__MSDOS__ || defined(W16))
       	b->Encipher((char huge *)(edges+1),(numnodes-1)*sizeof(Node));
#else
       	b->Encipher((char *)(edges+1), (numnodes-1)*sizeof(Node));
#endif
	delete b;
	FILE *fp = fopen(dictname, "wb");
	if (fp == 0) return "Couldn't open file for writing";
	else
	{
	    fwrite(edges+1, sizeof(Node), numnodes-1, fp);
	    fclose(fp);
	}
	return 0;
    }
}

int Dictionary::Lookup(char *word)
{
FPROF("Dictionary::Lookup");
    if (word==0 || *word==0 || edges == 0) return 0;
    long n = 1l;
    Node last;
    while (*word)
    {
    	int v = *word-'A'+1;
    	if (n==0l) return 0;
    	for (;;)
    	{
	    Node NV = edges[n];
    	    int dif = NV.NodeIndex()-v;
    	    if (dif==0)
    	    {
    		last = NV;
    		n = NV.NextNode();
    		break;
    	    }
    	    else if (dif>0 || NV.IsLastChild())
    		return 0;
    	    n++;
    	}
    	word++;
    }
    return last.IsWordEnd();
}

char newWord[20];

void Dictionary::RecursiveDump(int pos, long n, FILE *ofp)
{
FPROF("Dictionary::RecursiveDump");
    int c;
    if (n==(long)0) return;
    for (;;)
    {
    	Node N = edges[n];
	c = N.NodeIndex()-1;
	/* Put the corresponding letter in the word */
	newWord[pos] = (char)(c+'a');
	/* If the node is terminal, we have a valid matched word. */
	if (N.IsWordEnd())
	{
	    newWord[pos+1]=0;
	    fprintf(ofp, "%s\n", newWord);
	}
	RecursiveDump(pos+1, N.NextNode(), ofp);
	/* Move to next edge */
	if (!N.IsLastChild()) n++;
	else break;
    }
}

void Dictionary::Dump(FILE *fp)
{
FPROF("Dictionary::Dump");
    if (edges) RecursiveDump(0, 1l, fp);
}

void Dictionary::RawDump()
{
FPROF("Dictionary::RawDump");
    unsigned long i;
    printf("Nodes: %ld\n", numnodes);
    for (i=0;i<numnodes;i++)
    {
	Node N = edges[i];
	printf("%-6ld %-6ld %c%c %2d (%c)\n", (long)i,
			(long)N.NextNode(), N.IsWordEnd()?'T':' ',
			N.IsLastChild()?'L':' ',
			(int)N.NodeIndex(), N.NodeChar());
    }
}

Dictionary::~Dictionary()
{
FPROF("Dictionary::~Dictionary");
    delete [] anag;
    delete thash;
    Free();
}

void Dictionary::SetAnagramConstraint(char *anagram)
{
FPROF("Dictionary::SetAnagramConstraint");
    delete [] anag;
    if (anagram)
    {
        anag = new char[strlen(anagram)+1];
	if (anag) 
	    strcpy(anag, anagram);
    }
    else anag = 0;
    BuildAnagramVector(anag, anagcnt);
}

int Dictionary::IsAnagram(char *word)
{
FPROF("Dictionary::IsAnagram");
    if (anag == 0 || anag[0]==0) return 1; // assume OK
    short acnt[26];
    BuildAnagramVector(word, acnt);
    for (int i = 0; i < 26; i++)
        if (acnt[i] != anagcnt[i])
	    return 0;
    return 1;
}

#ifdef DUMP

main()
{
    Dictionary *dict = 0
    LoadDictionary(dict, "demo.dic");
    if (dict)
    {
    	dict->RawDump();
    	dict->Dump();
	delete dict;
    }
}

#endif

void Dictionary::FindChoicesRecursively(int pos, long n, char *word_in,
	char *word_tmp, unsigned long *masks)
{
FPROF("Dictionary::FindChoicesRecursively");
    if (n==0l) return;
    for (;;)
    {
    	int v = word_in[pos]-'A'+1;
    	Node NV = edges[n];
    	if (word_in[pos] == ' ' || word_in[pos] == '_' || NV.NodeIndex()==v)
	{
	    v = NV.NodeIndex();
	    word_tmp[pos] = (char)(v+'A'-1);
	    if (word_in[pos+1] == 0)
	    {
	    	if (NV.IsWordEnd())
	    	{
		    word_tmp[pos+1] = 0;
		    for (int i = 0; i <= pos; i++)
		    	masks[i] |= (1l << (word_tmp[i]-'A'));
	    	}
	    }
	    else FindChoicesRecursively(pos+1, NV.NextNode(), word_in,
					word_tmp, masks);
	}
    	if (NV.IsLastChild()) break;
    	n++;
    }
}

// Given a word with spaces (blanks), this returns the position of
// the first blank and a bitmask of possible letters. It returns -1
// if there is no blank and -2 if there are no possible matches.
//
// When the user enters such a word, this should be called like:
//
//  while ((pos=dict->FindConstraintsLookup(word, mask)) >=0)
//	word[pos] = GetUserChoice(mask, pos);
//

int Dictionary::FindConstraintsLookup(char *word, unsigned long &mask)
{
FPROF("Dictionary::FindConstraintsLookup");
    int rtn = -1;
    if (edges && word && word[0])
    {
    	int len = strlen(word);
	unsigned long *masks = new unsigned long[len];
	assert(masks);
	for (int p = 0; p < len; p++)
	    masks[p] = 0l;
	char *buf = new char[len+1];
	assert(buf);
	FindChoicesRecursively(0, 1l, word, buf, masks);
	delete [] buf;
	for (int i = 0; i < len; i++)
	{
	    int choices = 0, ltr = 0;
	    for (int j = 0; j < 26; j++)
		if ((masks[i] & (1l<<j)) != 0l)
		{
		    ltr = j;
		    choices++;
		}
	    if (choices == 0) rtn = -2; // invalid word
	    else if (choices > 1)
	    {
		rtn = i;
		mask = masks[i];
		break;
	    }
	    else
	    {
		if (word[i]<'A' || word[i]>'Z')
		    word[i] = ltr + 'a'; // blank tile with only one choice
	    }    
	}
	delete [] masks;
    }
    return rtn;
}

//--------------------------------------------------------------------
// New dictionary consult code

#define ALL	    (0x3FFFFFFl)
#define VOWELS      ((1l<<0) | (1l<<4) | (1l<<8) | (1l<<14) | (1l<<20))
#define CONSONANTS  (ALL - VOWELS)

int Dictionary::FindPoolAllocation(unsigned long avail)
{
FPROF("Dictionary::FindPoolAllocation");
    long lmask = 1l;
    for (int i = 0; i < 26; i++, lmask <<= 1)
    {
	if (need[i]>0)
	{
	    if (avail == 0l) return -1;
	    unsigned long m = 1l;
	    for (int j = 0; j < plen; j++, m<<=1)
	    {
		if ((avail & m) != 0 && (lmask & varpool[j]) != 0)
		{
		    need[i]--;
		    if (FindPoolAllocation((avail & ~m)) == 0)
		        return 0;
		    need[i]++;
		}
	    }
	    // no unused matching varpool patterns 8-(
	    return -1;
	}
    }
    // no need for any more allocations
    return 0;
}

int Dictionary::RemoveFromPool(int c)
{
FPROF("Dictionary::RemoveFromPool");

    if (allocpool[c] >= (fixedpool[c]+varmax[c])) // no hope...
	return -1;

    allocpool[c]++;

    if (fixedpool[c]>=allocpool[c]) // no problem...
	return 0;

    // the letter must come from the var pool.
    // Figure out how many letters we need...

    for (int i = 0; i < 26; i++)
	if (allocpool[i] > fixedpool[i])
	    need[i] = allocpool[i] - fixedpool[i];
	else
	    need[i] = 0;

    // now try to find an allocation of the var pool that matches the
    // list in need[]...

    if (FindPoolAllocation(pmask) < 0) // no possible allocation...
    {
        allocpool[c]--; // put it back
	return -1;
    }
    return 0;
}

void Dictionary::ReplaceInPool(int c)
{
FPROF("Dictionary::ReplaceInPool");
    allocpool[c]--;
}

char *Dictionary::NextPat(char *pat)
{
FPROF("Dictionary::NextPat");
    while (isspace(*pat)) pat++;
    if (*pat)
    {
	unsigned long v;
        if (isupper(*pat) || *pat==':' || *pat == '*' || *pat == '=')
        {
	    // fixed
	    if (*pat==':') v = ALL;
	    else if (*pat=='*') v = VOWELS; 
	    else if (*pat==':') v = CONSONANTS;
	    else v = 1l << (*pat-'A');
	    vec[vlen++] = v;
        }
        else if (islower(*pat))
	{
	    fixedpool[*pat-'a']++;
	    vec[vlen++] = 0l;
	}
	else if (*pat=='.' || *pat == '+' || *pat == '-')
	{
	    if (*pat=='.')
	    {
		varpool[plen++] = ALL;
		for (int k = 0; k < 26; k++) varmax[k]++;
	    }
	    else if (*pat=='+')
	    {
		varpool[plen++] = VOWELS;
		varmax[0]++;
		varmax[4]++;
		varmax[8]++;
		varmax[14]++;
		varmax[20]++;
	    }
	    else if (*pat=='-')
	    {
		varpool[plen++] = CONSONANTS;
		for (int k = 0; k < 26; k++) varmax[k]++;
		varmax[0]--;
		varmax[4]--;
		varmax[8]--;
		varmax[14]--;
		varmax[20]--;
	    }
	    vec[vlen++] = 0l;
	}
	else if (*pat == '[')
	{
	    unsigned long mask = 0l;
	    int negate = 0;
	    pat++;
	    v = 0l;
	    if (*pat=='!' || *pat=='^')
	    {
		negate=1;
		pat++;
	    }
	    int upper = isupper(*pat);
	    while (*pat!=']')
	    {
	    	if (*pat==0) return 0; /* syntax error */
		int start = *pat;
		if (isupper(start)) 
		    mask = (1l<<(start-'A'));
		else if (islower(start))
		{
		    start = toupper(start);
		    mask = (1l<<(start-'a'));
		}
		else return 0; // syntax error
		pat++;
		if (*pat=='-')
		{
		    pat++;
		    if (!isalpha(*pat)) return 0; // syntax error
		    int end = *pat;
		    pat++;
		    if (islower(end)) end = toupper(end);
		    while (start<=end)
		    {
			v |= mask;
			start++;
			mask = (1l<<(start-'A'));
		    }
		}
		else v |= mask;
	    }
	    if (negate) v ^= ALL;
	    if (upper)
	        vec[vlen++] = v;
	    else
	    {
	        vec[vlen++] = 0l;
	        varpool[plen++] = v;
		for (int k = 0; k < 26; k++)
		    if (((1l<<k) & v) != 0l)
			varmax[k]++;
	    }
	}
	pat++;
    }
    return pat;
}

//----------------------------------------------------------------------
// Sort multi-word anagrams and compare with last; used to
// avoid duplication

void Dictionary::CheckMulti(char *word)
{
FPROF("Dictionary::CheckMulti");
   char *res = word;
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
    if (strcmp(tmp2, lastmulti) > 0)
    {
    	strcpy(lastmulti, tmp2);
    	res = lastmulti;
        match++;
        if (logfp) fprintf(logfp, "%-5ld %s\n", match, res);
    }
}

//----------------------------------------------------------------------
// Node stack for non-recursive consults.

int Dictionary::GrabLetter()
{
FPROF("Dictionary::GrabLetter");
    Node N = edges[nstk[sp]];
    int c = N.NodeIndex()-1;
    unsigned long mask = (1l<<c);
    unsigned long vmask = vec[sp+vecoff];
    if ((vmask==0 && RemoveFromPool(c) >= 0) || (vmask & mask) != 0l)
	return 0;
    return -1;
}

void Dictionary::ReplaceLetter()
{
FPROF("Dictionary::ReplaceLetter");
    Node N = edges[nstk[sp]];
    if (vec[sp+vecoff]==0)
	ReplaceInPool(N.NodeIndex()-1);
}

void Dictionary::InitStack(int len, int first_vec, int allow_multi)
{
FPROF("Dictionary::InitStack");
    smult = allow_multi;
    stop = len-1;
    nstk[sp = start = 0] = 1l;
    vecoff = first_vec;
    swcnt = 1;
}

void Dictionary::LogSingle(char *word)
{
FPROF("Dictionary::LogSingle");
    if (thash==0 || thash->Lookup(word))
    {
        match++;
        if (logfp) 
	    fprintf(logfp, "%-5ld %s\n", match, word);
    }
}

char *Dictionary::GetWord(int len)
{
FPROF("Dictionary::GetWord");
    static char word[80];
    int i, j;
    for (i = j = 0; i<=len;i++)
    {
	if (i && nstk[i] <= 26) word[j++] = ' ';
	word[j++] = edges[nstk[i]].NodeIndex()-1+'A';
    }
    word[j] = 0;
    return word;
}

void Dictionary::LogWord()
{
FPROF("Dictionary::LogWord");
    char *word = GetWord(stop);
    if (smult)
        CheckMulti(word);
    else
        LogSingle(word);
}

#define InRange(v, mn, mx)	((v) >= (mn) && (v) <= (mx))

int Dictionary::NextSWord()
{
FPROF("Dictionary::NextSWord");
    Node N = edges[nstk[sp]];

    // do we have the letter for the current node?

    if (GrabLetter() == 0)
    {
        int wlen = sp-1;
        if (sp == stop) // at the end?
        {
	    if (N.IsWordEnd() && InRange(wlen, minlen, maxlen))
	    {
	        LogWord();
	    }
        }
        else if (N.NextNode() && wlen<maxlen)
        {
	    // push on to next letter
	    nstk[++sp] = N.NextNode();
	    return 0;
        }
	ReplaceLetter();
    }

    // Determine whether to move to peer or bottom out

    for (;;)
    {
        if (!N.IsLastChild()) // move to peer
	{
	    nstk[sp]++;
	    return 0;
	}
	// no more peers; pop the stack
	if (sp==0) break;

	N = edges[nstk[--sp]];
	ReplaceLetter();
    }
    return -1; // no more
}

int Dictionary::NextMWord()
{
FPROF("Dictionary::NextMWord");
    Node N = edges[nstk[sp]];
    int wlen = sp-start+1;
    if (GrabLetter() == 0)
    {
        if (sp == stop) // at the end?
        {
	    if (N.IsWordEnd() && 
	        InRange(wlen, minlen, maxlen) &&
		InRange(swcnt, mincnt, maxcnt))
	    {
//puts(GetWord());
	        LogWord();
	    }
        }
	else if (N.IsWordEnd() && 
		 InRange(wlen, minlen, maxlen) &&
		 swcnt<=maxcnt)
	{
	    // push on to next word
	    nstk[start = ++sp] = 1l;
	    swcnt++;
	    return 0;
	}
        else if (N.NextNode() && wlen<maxlen)
        {
	    // push on to next letter
	    nstk[++sp] = N.NextNode();
	    return 0;
        }
	ReplaceLetter();
    }
    for (;;)
    {
        if (!N.IsLastChild()) // move to peer
	{
	    nstk[sp]++;
	    return 0;
	}
	// no more peers; pop the stack
	if (sp==0) break;
	int was_new_word;
	if (sp == start)
	{
	    was_new_word = 1;
	    // find last start point
	    while (start > 0 && nstk[--start] > 26);
	    swcnt--;
	}
 	else was_new_word = 0;
	N = edges[nstk[--sp]];
	wlen = sp-start+1;
	if (was_new_word && N.NextNode() && wlen<maxlen)
	{
	    nstk[++sp] = N.NextNode();
	    return 0;
	}
	else ReplaceLetter();
    }
    return -1; // no more
}

//-----------------------------------------------------------------------

int progress = 0; // kludge

void Dictionary::SingleRecurse(long n, int wordpos, int vecpos, int len)
{
FPROF("Dictionary::SingleRecurse");
    if (n==0l) return;
    int atend = (wordpos == (len-1));
    while (!abortmatch)
    {
	Node N = edges[n];
	int c = N.NodeIndex()-1;
	unsigned long mask = (1l<<c);
	if (vec[vecpos]==0) // use a pool tile
	{
	    if (RemoveFromPool(c) < 0) goto skipit;
	}
	else if ((vec[vecpos] & mask) == 0) // can't go in this position
	    goto skipit;

	// Put the corresponding letter in the word */

	word[wordpos] = (char)(c+'A');

	// If we have reached the end of the word, and the node
	// is terminal, we have a valid matched word.

	if (atend)
	{
	    if (N.IsWordEnd())
	    {
		word[wordpos+1]=0;
		LogSingle(word);
	    }
	}
	else SingleRecurse(N.NextNode(), wordpos+1, vecpos+1, len);

	// if we used a pool tile, replace it

	if (vec[vecpos]==0) // use a pool tile
	    ReplaceInPool(c);
skipit:
	/* Move to next edge */
	if (!N.IsLastChild()) n++;
	else
	{
	    if (wordpos>1)
	    {
	        int p = (100 * (word[0]-'A'+1) * (word[1]-'A'+1)) / (26*26);
		if (p > progress)
	            ShowProgress(progress = p);
	    }
	    break;
	}
    }
}

void Dictionary::MultiRecurse(long n, int vecpos, int respos, int wlen, int cnt)
{
FPROF("Dictionary::MultiRecurse");
    if (n==0l) return;
    while (!abortmatch)
    {
	Node N = edges[n];
	int c = N.NodeIndex()-1;
	unsigned long mask = (1l<<c);
	if (vec[vecpos]==0) // use a pool tile
	{
	    if (RemoveFromPool(c) < 0) goto skipit;
	}
	else if ((vec[vecpos] & mask) == 0) // can't go in this position
	    goto skipit;

	// Put the corresponding letter in the word */
	word[respos] = (char)(c+'A');

	// If we have reached the end of the word, and the node
	// is terminal, we have a valid matched word.

        if (N.IsWordEnd() && (wlen>=minlen && wlen<=maxlen))
        {
    	    if (cnt<=maxcnt)
    	    {
    	        if (vecpos == (vlen-1))
    	        {
		    if (cnt >= mincnt)
		    {
    		    	word[respos+1] = '\0';
			CheckMulti(word);
		    }
    	        }
    	        else
    	        {
    	    	    // try next word
    	    	    word[respos+1] = ' ';
    	    	    MultiRecurse(1l, vecpos+1, respos+2, 1, cnt+1);
    	        }
    	    }
        }

    	if (vecpos < (vlen-1)) // try next extension
	    MultiRecurse(N.NextNode(), vecpos+1, respos+1, wlen+1, cnt);

	// if we used a pool tile, replace it
	if (vec[vecpos]==0) 
	    ReplaceInPool(c);
skipit:
	/* Move to next edge */
	if (!N.IsLastChild()) n++;
	else break;
    }
}

void Dictionary::PrepPattern(char *pat, int type_in, 
			int minlen_in, int maxlen_in, 
			int mincnt_in, int maxcnt_in,
			char *topics_in)
{
    int i;
FPROF("Dictionary::PrepPattern");
    abortmatch = progress = 0;
    for (i = 0; i < 26; i++) allocpool[i] = fixedpool[i] = varmax[i] = 0;
    vlen = plen = 0;
    while (*pat) pat = NextPat(pat);
    minlen = minlen_in; maxlen = maxlen_in;
    mincnt = mincnt_in;	maxcnt = maxcnt_in;
    if (minlen < 1) minlen = 1;
    if (maxlen < 1 || maxlen > vlen) maxlen = vlen;
    if (minlen < 1) minlen = 1;
    if (mincnt < 1) mincnt = 1;
    if (type_in != MULTI) maxcnt = 1;
    else if (maxcnt < 1) maxcnt = vlen;
    match = pmask = 0l;
    lastmulti[0] = 0;
    for (i = 0; i < plen; i++) pmask |= (1l << i);
    delete thash;
    thash = 0;
    if (topics_in && topics_in[0])
	thash = GetThesaurusWords(topics_in);
}

//-------------------------------------------------------------------------

#if 1 // fast recursive version

void Dictionary::MatchPattern(char *pat, int type, 
			int minlen_in, int maxlen_in, 
			int mincnt_in, int maxcnt_in,
			char *topics_in)
{
    int i;
FPROF("Dictionary::MatchPattern1");
    if (edges==0) return;
    PrepPattern(pat, type, minlen_in, maxlen_in, mincnt_in, maxcnt_in,
    			topics_in);
    switch(type)
    {
    case MULTI:
        MultiRecurse(1l, 0, 0, 1, 1);
	break;
    case USEALL:
	SingleRecurse(1l, 0, 0, vlen);
	break;
    case PREFIX:
	for (i = minlen; i<=maxlen; i++)
	    SingleRecurse(1l, 0, 0, i);
	break;
    case SUFFIX:
	for (i = minlen; i<=maxlen; i++)
	    SingleRecurse(1l, 0, vlen-i, i);
	break;
    }
}

#else	// slow iterative version. Useful as it can be done as a
	// background task

void Dictionary::MatchPattern(char *pat, int type, 
			int minlen_in, int maxlen_in, 
			int mincnt_in, int maxcnt_in,
			char *topics_in)
{
    int i;
FPROF("Dictionary::MatchPattern2");
    if (edges==0) return;
    PrepPattern(pat, type, minlen_in, maxlen_in, mincnt_in, maxcnt_in,
    			topics_in);
    
    switch(type)
    {
    case MULTI:
	InitStack(vlen, 0, 1);
	while (NextMWord() == 0);
	break;
    case USEALL:
	InitStack(vlen, 0, 0);
	while (NextSWord() == 0);
	break;
    case PREFIX:
	for (i = minlen; i<=maxlen; i++)
	{
	    InitStack(i, 0, 0);
	    while (NextSWord() == 0);
	}
	break;
    case SUFFIX:
	for (i = minlen; i<=maxlen; i++)
	{
	    InitStack(i, vlen-i, 0);
	    while (NextSWord() == 0);
	}
	break;
    }
}
#endif

//-------------------------------------------------------------------------
// Different variant of the above, that can be used iteratively. 

void Dictionary::StartConsult(char *pat, int type, 
			int minlen_in, int maxlen_in, 
			int mincnt_in, int maxcnt_in,
			char *topics_in)
{
    int i;
FPROF("Dictionary::StartConsult");
    if (edges==0) return;
    PrepPattern(pat, mtyp = type, minlen_in, maxlen_in, 
    			mincnt_in, maxcnt_in, topics_in);
    switch(type)
    {
    case MULTI:
    case USEALL:
	wlen[0] = vlen;
	vpos[0] = 0;
	numlens = 1;
	break;
    case PREFIX:
	for (i = minlen; i<=maxlen; i++)
	{
	    wlen[i-minlen] = i;
	    vpos[i-minlen] = 0;
	}
	numlens = maxlen-minlen+1;
	break;
    case SUFFIX:
	for (i = minlen; i<=maxlen; i++)
	{
	    wlen[i-minlen] = i;
	    vpos[i-minlen] = vlen-i;
	}
	numlens = maxlen-minlen+1;
    }
    lennow = 0;
    InitStack(wlen[0], vpos[0], (mtyp==MULTI));
}

int Dictionary::NextConsult(int cnt)
{
FPROF("Dictionary::NextConsult1");
    if (abortmatch) return -1;
    else if (edges==0) return 0;
    if (mtyp == MULTI)
    {
        for (int i = 0; i < cnt; i++)
	{
	    if (NextMWord() != 0)
	    {
	    	if (++lennow >= numlens) return -1;
	    	InitStack(wlen[lennow], vpos[lennow], 1);
	    }
	}
    }
    else
    {
        for (int i = 0; i < cnt; i++)
	{
            if (NextSWord() != 0)
	    {
	    	if (++lennow >= numlens) return -1;
	    	InitStack(wlen[lennow], vpos[lennow], 0);
	    }
	}
    }
    return 0;
}

int Dictionary::NextConsult()
{
FPROF("Dictionary::NextConsult2");
    if (sp>0)
    {
	int pos = edges[nstk[0]].NodeIndex() * 26 + 
		  edges[nstk[1]].NodeIndex() - 1;
        ShowProgress((pos*100) / (26*26));
    }
    return NextConsult(2500);
}

void Dictionary::RecursiveLength(int pos, long n)
{
FPROF("Dictionary::RecursiveLength");
    int c;
    if (n==(long)0) return;
    for (;;)
    {
    	Node &N = edges[n];
	if (N.IsWordEnd())
	{
	    if (pos > longest) longest = pos;
	}
	RecursiveLength(pos+1, N.NextNode());
	/* Move to next edge */
	if (!N.IsLastChild()) n++;
	else break;
    }
}

int Dictionary::Longest()
{
FPROF("Dictionary::Longest");
    longest = 0;
    if (edges) RecursiveLength(0, 1l);
    return longest;
}

