// Convert Project Gutenberg Roget's Thesaurus into a list of
// lines, each representing a thesaurus topic. Each line consists
// of single words that fall under that topic.

// Essentially we parse topics as separated by "#number" strings,
// and then pull out any single words within a topic that are preceded 
// and followed by punctuation characters. Newlines must be ignored
// in this process.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
//#include "wx.h"
#include "register.h"
#include "dict.h"

#if !defined(W16) && !defined(W32) && !defined(__MSDOS__)
static void strlwr(char *s) { for (; *s; s++) if (isupper(*s)) *s = tolower(*s); }
static void strupr(char *s) { for (; *s; s++) if (islower(*s)) *s = toupper(*s); }
#endif
#if !defined(W16) && !defined(__MSDOS__)
#define far
#endif

Dictionary *dict = 0;

typedef struct _wrd
{
    struct _wrd *next;
    char word[32];
} wrdnode;

wrdnode *head = 0;

void FreeWords()
{
    while (head)
    {
        wrdnode *tmp = head;
	head = head->next;
	delete tmp;
    }
}

void AddWord(char *word)
{
    while (*word && !isalpha(*word)) word++;
    int l = strlen(word);
    while (l > 0 && !isalpha(word[l-1])) l--;
    word[l] = 0;
    if (l < 2 || word[0] < 'a') return;
    strupr(word);
    if (dict->Lookup(word)==0) return;
    wrdnode *tmp = head;
    while (tmp)
    {
        if (strcmp(tmp->word, word) == 0) return;
	tmp = tmp->next;
    }
    tmp = new wrdnode;
    strcpy(tmp->word, word);
    tmp->next = head;
    head = tmp;
}

static int maxlen = 0, maxtop = 0;

void DumpWords(int topic, FILE *fp, FILE *idxfp)
{
    char *sep = "";
    int len = 0, top = 0;
    fprintf(fp, "%d ", topic);
    while (head)
    {
        wrdnode *tmp = head;
	head = head->next;
	fprintf(fp, "%s%s", sep, tmp->word);
	len += strlen(sep) + strlen(tmp->word);
	if (tmp->next==0)
	    fprintf(idxfp, "%d %s\n", topic, tmp->word);
	sep = ",";
	delete tmp;
	top++;
    }
    fprintf(fp, "\n");
    if (len > maxlen) maxlen = len;
    if (top > maxtop) maxtop = top;
}

int FirstPass()
{
    dict = new Dictionary;
    char *err = LoadDictionary(dict, "wwbig.dic");
    if (err) { fprintf(stderr, "%s\n", err); return -1; }
    FILE *ifp = fopen("roget14a.txt", "r");
    if (ifp==0) return 0;
    FILE *ofp = fopen("roget14a.out", "w");
    if (ofp==0) { fclose(ifp); return 0; }
    FILE *idxfp = fopen("roget14a.idx", "w");
    if (idxfp==0) { fclose(ifp); fclose(ofp); return 0; }
    char buff[256], last[32];
    last[32]=0;
    int topic= -1;
    while (!feof(ifp))
    {
        if (fgets(buff, 256, ifp) == 0) break;
	char *s = buff;
	while (isspace(*s)) s++;
	int t;
	if (*s == '#' && sscanf(s+1, "%d", &t)==1) // start new topic
	{
	    if (last[0]) AddWord(last);
	    if (topic >=0) DumpWords(topic, ofp, idxfp);
	    last[0] = 0;
	    while (isdigit(*s)) s++;
	    topic = t;
	}
	else if (topic<0) continue;
	else if (last[0])// continue with previous topic
	{
	    if (!isalpha(*s)) AddWord(last);
	    else while (*s && (isalpha(*s) || isspace(*s))) s++;
	    last[0] = 0;
	}
	// now parse from s to end
	while (*s)
	{
	    while (*s && !isalpha(*s))
	    {
	        if (*s == '(')
		{
		    while (*s && *s != ')') s++;
		}
		else if (*s == '[')
		{
		    while (*s && *s != ']') s++;
		}
	        else s++;
	    }
	    char *b = s;
	    while (*s && isalpha(*s)) s++;
	    char *e = s;
	    while (*s && (isspace(*s) || *s=='-')) s++;
	    if (*s == 0)
	    {
	        if ((e-b)>1) { strncpy(last, b, e-b); last[e-b] = 0; }
		break;
	    }
	    else if (isalpha(*s))
	    {
		while (*s && (isalpha(*s) || isspace(*s) || *s=='-')) s++;
		if (*s == 0) strcpy(last, " "); // kludge
	    }
	    else if (e>b)
	    {
		if ((e-b)>1) 
		{ 
		    char tb[32];
		    strncpy(tb, b, e-b); 
		    tb[e-b] = 0; 
		    AddWord(tb);
		}
	    }
	}
    }
    if (last[0]) AddWord(last);
    DumpWords(topic, ofp, idxfp);
    fclose(ifp);
    fclose(ofp);
    fclose(idxfp);
    delete dict;
    printf("Max length: %d\n", maxlen);
    printf("Max words/topic: %d\n", maxtop);
    return 1;
}


//-------------------------------------------------------------------

typedef struct _topic
{
    struct _topic far *next;
    short idx;
} topicnode;

typedef struct _word
{
    struct _word far *left;
    struct _word far *right;
    struct _topic far *head;
    char word[32];
} wordnode;

wordnode far *whead = 0;

#if defined(__MSDOS__)
#define ALLOC_WORD_NODE(t)		t = (wordnode far*)farmalloc(sizeof(wordnode))
#define ALLOC_TOPIC_NODE(t)		t = (topicnode far*)farmalloc(sizeof(topicnode))
#define FREE_NODE(t)			farfree(t)
#else
#define ALLOC_WORD_NODE(t)		t = new wordnode
#define ALLOC_TOPIC_NODE(t)		t = new topicnode
#define FREE_NODE(t)			delete t
#endif

void AddRef(char *word, int topic)
{
    while (*word && !isalpha(*word)) word++;
    int l = strlen(word);
    while (l > 0 && !isalpha(word[l-1])) l--;
    word[l] = 0;
    wordnode far *tmp = whead;
    wordnode far *last = 0;
    int n;
    while (tmp)
    {
	last = tmp;
        n = strcmp(tmp->word, word);
        if (n == 0) break;
	else if (n > 0) tmp = tmp->left;
	else tmp = tmp->right;
    }
    if (tmp == 0)
    {
        ALLOC_WORD_NODE(tmp);
        strcpy(tmp->word, word);
	tmp->left = tmp->right = 0;
	tmp->head = 0;
	if (last)
	{
	    if (n > 0)
	    {
		last->left = tmp;
	    }
	    else
	    {
		last->right = tmp;
	    }
	}
	else
	{
	    printf("as root\n");
	    whead = tmp;
	}
    }
    topicnode far *t;
    ALLOC_TOPIC_NODE(t);
    t->idx = topic;
    t->next = tmp->head;
    tmp->head = t;
}

int DumpTopic(FILE *fp, topicnode far *n)
{
    int cnt = 1;
    if (n)
    {
	cnt += DumpTopic(fp, n->next);
	//fprintf(fp, " %d", n->idx);
	fwrite(&n->idx, sizeof(short), 1, fp);
	FREE_NODE(n);
    }
    return cnt;
}

int maxt = 0;

void DumpNode(FILE *fp, wordnode far *n)
{
    if (n == 0) return;
    DumpNode(fp, n->left);
    int l = strlen(n->word)+1;
    if ((l%2)!=0) { n->word[l] = 0; l++; }
    fwrite(n->word, sizeof(char), l, fp);
    //fprintf(fp, "%s", n->word);
    l = DumpTopic(fp, n->head);
    if (l > maxt) maxt = l;
    short w = 0;
    fwrite(&w, sizeof(w), 1, fp);
    //fprintf(fp, "\n");
    DumpNode(fp, n->right);
    FREE_NODE(n);
}

void DumpRefs()
{
    FILE *fp = fopen("roget.ref", "w");
    DumpNode(fp, whead);
    fclose(fp);
    printf("Max topics per word: %d\n", maxt);
}

char buff[5000];

void SecondPass()
{
    FILE *ifp = fopen("roget14a.out", "r");
    while (!feof(ifp))
    {
	if (fgets(buff, 5000, ifp) == 0) break;
	int topic;
	sscanf(buff, "%d", &topic);
	printf("TOPIC %d\n", topic);
	char *s = buff;
	while (isdigit(*s)) s++;
	while (isspace(*s)) s++;
	for (char *t = strtok(s, ","); t; t = strtok(0, ","))
	{
	    printf("\t%s\n", t);
	    AddRef(t, topic);
	}
    }
    fclose(ifp);
    DumpRefs();
}

//-------------------------------------------------------------------

main()
{
//    FirstPass();
    SecondPass();
}

