// Convert Project Gutenberg Roget's Thesaurus into a list of
// lines, each representing a thesaurus topic. Each line consists
// of single words that fall under that topic.

// Essentially we parse topics as separated by "#number" strings,
// and then pull out any single words within a topic that are preceded 
// and followed by punctuation characters. Newlines must be ignored
// in this process.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <assert.h>

#if !defined(W16) && !defined(W32) && !defined(__MSDOS__)
static void strlwr(char *s) { for (; *s; s++) if (isupper(*s)) *s = tolower(*s); }
static void strupr(char *s) { for (; *s; s++) if (islower(*s)) *s = toupper(*s); }
#endif
#if !defined(W16) && !defined(__MSDOS__)
#define far
#endif

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
	assert(tmp);
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
	    whead = tmp;
	}
    }
    topicnode far *t;
    ALLOC_TOPIC_NODE(t);
    assert(t);
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
	    //printf("\t%s\n", t);
	    AddRef(t, topic);
	}
    }
    fclose(ifp);
    DumpRefs();
}

//-------------------------------------------------------------------

main()
{
    SecondPass();
}

