/*
 * File:	windic.cc
 * Purpose:	WordsWorth dictionary maintenance
 * Author:	Graham Wheeler
 * Created:	December 1996
 * Updated:	
 * Copyright:	(c) 1996, Graham Wheeler
 */

#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif
#include "wx_timer.h"
#include "gdialog.h"
#include "blowfish.h"
#include "register.h"
#include "winids.h"
#include "dict.h"
#include "dicbuild.h"

#if !defined(W16)
#define huge
#endif

#ifdef wx_x
void strupr(char *s)
{
    for ( ; *s; s++) if (islower(*s)) *s = toupper(*s);
}
#endif

int must_abort = 0;

//--------------------------------------------------------------------

void DictionaryBuilder::Free()
{
#if (defined(W32) || defined(W16))
    if (node) 
    {
	GlobalUnlock(nodehandle);
	GlobalFree(nodehandle);
    }
    if (Index) 
    {
	GlobalUnlock(indexhandle);
	GlobalFree(indexhandle);
    }
#else // DOS
#  if __MSDOS__
    if (node) farfree(node);
    if (Index) farfree(Index);
#  else // UNIX
    delete [] node;
    delete [] Index;
#  endif
#endif
    node = 0;
    Index = 0;
}

void DictionaryBuilder::AllocateMem(long numnodes)
{
#if (defined(W32) || defined(W16))
    indexhandle = GlobalAlloc(GPTR, numnodes*((long)sizeof(nodenum)));
    nodehandle = GlobalAlloc(GPTR, numnodes*((long)sizeof(TreeNode)));
    Index = (nodenum FAR *)GlobalLock(indexhandle);
    node = (TreeNode FAR *)GlobalLock(nodehandle);
#else
#  if __MSDOS__
    Index = (nodenum huge *)farcalloc(numnodes,sizeof(nodenum));
    node = (TreeNode huge *)farcalloc(numnodes,sizeof(TreeNode));
#  else // UNIX
    Index = new nodenum[MAXNODES];
    node = new TreeNode[MAXNODES];
#  endif
#endif
}

void DictionaryBuilder::SaveState()
{
    if (!IsRegistered()) return;
    FILE *stf = fopen("windic.tmp", "wb");
    if (stf == 0) return;
    fwrite(&botnode, sizeof(botnode), 1, stf);
    fwrite(&topnode, sizeof(topnode), 1, stf);
    fwrite(&freeNodes, sizeof(freeNodes), 1, stf);
    fwrite(&usednodes, sizeof(usednodes), 1, stf);
    fwrite(word, sizeof(char), MAXLEN+1, stf);
    fwrite(lastword, sizeof(char), MAXLEN+1, stf);
    fwrite(&wcnt, sizeof(wcnt), 1, stf);
    fwrite(&stkTop, sizeof(stkTop), 1, stf);
    fwrite(nodeStk, sizeof(nodenum), stkTop+1, stf);
    TreeNode huge *n = node;
    for (int i = 0; i <= topnode; i++, n++)
	fwrite(n, sizeof(TreeNode), 1, stf);
    nodenum huge *idx = Index;
    for (int i = 0; i <= topnode; i++, idx++)
	fwrite(idx, sizeof(nodenum), 1, stf);
    fclose(stf);
}

void DictionaryBuilder::RestoreState(FILE *lex)
{
    if (!IsRegistered()) return;
    FILE *stf = fopen("windic.tmp", "rb");
    if (stf == 0) return;
    fread(&botnode, sizeof(botnode), 1, stf);
    fread(&topnode, sizeof(topnode), 1, stf);
    fread(&freeNodes, sizeof(freeNodes), 1, stf);
    fread(&usednodes, sizeof(usednodes), 1, stf);
    fread(word, sizeof(char), MAXLEN+1, stf);
    fread(lastword, sizeof(char), MAXLEN+1, stf);
    fread(&wcnt, sizeof(wcnt), 1, stf);
    fread(&stkTop, sizeof(stkTop), 1, stf);
    fread(nodeStk, sizeof(nodenum), stkTop+1, stf);
    TreeNode huge *n = node;
    for (int i = 0; i <= topnode; i++, n++)
	fread(n, sizeof(TreeNode), 1, stf);
    nodenum huge *idx = Index;
    for (int i = 0; i <= topnode; i++, idx++)
	fread(idx, sizeof(nodenum), 1, stf);
    fclose(stf);
    /* Now skip forward in lex file to next word */
    char buf[MAXLEN];
    for (int i = wcnt; i != 0; )
    {
	if (fgets(buf, MAXLEN, lex)==0) break;
	if (buf[0]>='A' && buf[1]>='A') i--;
    }
}

void DictionaryBuilder::Setup(FILE *restore)
{
    for (int j=0; j<27; j++) 
        node[0][j] = (nodenum)0;
    /* set up free list */
    for (nodenum i=1; i<MAXNODES; i++)
    {
	node[i][0] = FREE;
	node[i][1] = i+1;
    }
    /* clear the stack containing last word */
    for (int j=0; j<=MAXLEN; j++) nodeStk[j]=0;
    word[0] = lastword[0]=0;
    must_abort = 0;
    if (restore) 
        RestoreState(restore);
}

nodenum DictionaryBuilder::AllocNode()
{
    nodenum i;
    int j;
    if (botnode >= MAXNODES)
    {
	fprintf(stderr,"Out of nodes!\n");
	return (nodenum)-1;
    }
    i = botnode;
    freeNodes--;
    botnode = node[i][1];
    if (i>topnode) 
        topnode = i;
    for (j=0;j<27;j++) 
        node[i][j] = (NODE)0;
    return i;
}

#define nodeCmp(n1,n2)	(memcmp(node[n1], node[n2], 27*sizeof(NODE))==0)

/* change all refs to n2 to refer to n1 */

void DictionaryBuilder::Relink(NODE n1, NODE n2)
{
    for (nodenum i=0l; i<=topnode; i++)
    {
	int j;
	NODE huge *N = node[i];
	if (N[0] & FREE) continue;
	for (N++,j=27 ; j>0 ; j--, N++)
	{
	    if (*N==n2) *N=n1;
	}
    }
}

void DictionaryBuilder::PrintGraph(FILE *fp)
{
    for (nodenum i=0; i<=topnode; i++)
    {
	if (node[i][0] & FREE) continue;
	if (node[i][0] & TERMINAL) fprintf(fp, "Terminal ");
	fprintf(fp, "Node %ld\n",(ulong)i);
	for (int j=1; j<27; j++)
	{
	    if (node[i][j])
	    	fprintf(fp, "    Child %ld (%c)\n",
			(ulong)node[i][j], (char)(j+'A'-1));
	}
    }
}

void DictionaryBuilder::MarkGraph(nodenum n)
{
    int i;
    if (node[n][0] & FREE)
    {
	node[n][0] &= ~FREE;
	usednodes++;
    }
    for (i=1; i<27; i++)
        if (node[n][i])
	   MarkGraph(node[n][i]);
}


/* S-coder encryption from Dr Dobbs Jan 1990 */

NODE buff[1500];
static char 	*cryptext;	/* Key				*/
static int	crypt_ptr=0;	/* Circular pointer into key	*/
static int	crypt_length;	/* Key length			*/

static void crypt(char *buf)
{
    *buf ^= cryptext[crypt_ptr]^(cryptext[0]*crypt_ptr);
    cryptext[crypt_ptr] += ((crypt_ptr < (crypt_length - 1)) ?
			  cryptext[crypt_ptr+1] : cryptext[0]);
    if (!cryptext[crypt_ptr]) cryptext[crypt_ptr]++;
    if (++crypt_ptr >= crypt_length) crypt_ptr = 0;
}

static void encrypt(char *buf, int len, char *key) 
{
    cryptext = key;
    crypt_length = strlen(key);
    while (len--) crypt(buf++);
}

void DictionaryBuilder::WriteDawg(FILE *dawg, unsigned char *key, int keylen)
{
    if (!IsRegistered()) return;
    frame->SetStatusText("Writing dictionary");
    unsigned short j, chld, last;
    nodenum i, index = 1;
    Blowfish *b = new Blowfish;
    if (b==0)
    {
	frame->SetStatusText("Memory allocation failure");
	return;
    }
    else if (!b->SetKey(key, keylen))
    {
	frame->SetStatusText("Internal error");
	return;
    }
    // Reindex the nodes
    for (nodenum i=0; i<=topnode; i++)
    {
	if (node[i][0]&FREE) continue;
	chld = 0;
	for (j=1; j<27; j++)
	    if (node[i][j])
		chld++;
	if (chld)
	{
	    Index[i] = index;
	    index += chld;
	}
	else Index[i] = (nodenum)0;
    }
    unsigned long vbuf[2];
    int vp = 0;
    for (nodenum i=0; i<=topnode; i++)
    {
	if (node[i][0]&FREE) continue;
	if (Index[i])
	{
#ifdef DEBUG
	    printf("Writing node %ld, index %d\n",(ulong)i, Index[i]);
#endif
	    for (j=26; j; j--)
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
#ifdef DEBUG
		    printf("  Writing edge for %c (%08lX)\n",(char)(j+'A'-1),v);
#endif
		    vbuf[vp] = v;
		    vp = 1 - vp;
		    if (vp == 0)
		    {
		        b->EncipherBlock(&vbuf[0], &vbuf[1]);
			fwrite(vbuf,2,sizeof(unsigned long),dawg);
		    }
		}
	    }
	}
    }
    if (vp)
    {
        vbuf[1] = 0;
	b->EncipherBlock(&vbuf[0], &vbuf[1]);
	fwrite(vbuf,2,sizeof(unsigned long),dawg);
    }
    delete b;
}

int DictionaryBuilder::MergeNodes(nodenum first, nodenum last, NODE n)
{
    nodenum i;
    for (i=first; i<=last; i++)
    {
	if (node[i][0]&FREE) continue;
	if (nodeCmp(i,n))
	{
	    Relink(i,n);
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

int DictionaryBuilder::MergeNodes(NODE n)
{
    /* This funny way of doing it is (hopefully!) a performance improvement
       	as we no longer have to check for equality in nodeCmp */
    if (MergeNodes(1, n-1, n))
        return 1;
    else
        return MergeNodes(n+1, topnode, n);
}

int DictionaryBuilder::AddWord(char *word, int pos)
{
    frame->SetStatusText(word);
    nodenum n = nodeStk[pos];
    ++wcnt;
    //printf("%d: %-16s Free %d\n", wcnt, word, freeNodes);
    while (word[pos])
    {
	int j = word[pos] - 'A' + 1;
	node[n][0]++;
	n = nodeStk[++pos] = node[n][j] = AllocNode();
	if (n == (nodenum)-1) return -1;
    }
    node[n][0] = TERMINAL;
    stkTop = pos;
    return 0;
}

/* compare two differing strings, return position of first difference*/

int cmpstr(char *s1, char *s2)
{
    int pos = 0;
    while ((s1[pos]==s2[pos]) && s1[pos]) pos++;
    if (s1[pos] && s1[pos]<s2[pos])
    {
//	fprintf(stderr,"ERROR: input must be sorted - unlike %s and %s\n",
//			s2,s1);
	return -1;
//	exit(0);
    }
    return pos;	
}

int DictionaryBuilder::GetWord(FILE *lex)
{
    fgets(word,MAXLEN,lex);
    if (feof(lex)) return -1;
    word[MAXLEN] = 0;
    strupr(word);
    int l = strlen(word)-1;
    // strip trailing garbage
    while (l>0 && (word[l]<'A' || word[l]>'Z')) l--;
    /* l now indexes the last letter */
    word[l+1] = 0;
    return l;
}

void DoWord(void *arg)
{
    ((DictionaryBuilder*)arg)->DoWord();
}

void DictionaryBuilder::DoWord()
{
    unsigned short l;
    int i, pos;
    nodenum n;

    /* add words */

    if (!must_abort)
    {
        int l = GetWord(lex);
	if (l < 0) // finished
	{
	    fclose(lex);
	    lex = 0;

	    // Make sure we've bottomed out... 

	    for (int i = stkTop; i > 0; i--)
	        MergeNodes(nodeStk[i]);

	    // mark graph to determine which nodes are reachable. I think
	    // this is just a sanity check if I remember correctly...

	    for (n=0; n<=topnode; n++)
                if (node[n][0] & FREE)
    	            node[n][1] = 0;	/* clear next free pointer */
                else 
	    	    node[n][0] |= FREE;
	    usednodes = (nodenum)0;
	    MarkGraph((nodenum)0);

	    // Save the DAWG

	    WriteDawg(dawg);
	    fclose(dawg);
	    dawg = 0;

	    wxMessageBox("Dictionary Built!", "Done", wxOK|wxCENTRE);
	    goto stopcycle;
	}
	else if (l < 1 || l >= MAXLEN) 
	    return;

	/* Find the point of departure from last word */

	pos = cmpstr(word,lastword);
	if (pos < 0) return; // violation of sort ordering

	// `bottom-out' of recursion to point of departure, merging
	// any completed subtrees

	for (int i = stkTop; i > pos; i--)
	    if (MergeNodes(nodeStk[i]) == 0)
	        break;

	// Add the new suffix

	if (AddWord(word, pos) < 0) // out of memory
	{
	    wxMessageBox("Out of Memory!", "Error", wxOK|wxCENTRE);
	    goto stopcycle;
	}
	strcpy(lastword,word);
    }
    else 
    {
        SaveState();
	wxMessageBox("Dictionary build interrupted!", "Interrupt", wxOK|wxCENTRE);
	goto stopcycle;
    }
    return;
  stopcycle:
    RemoveCycler(::DoWord);
    running = 0;
    fclose(dawg); dawg = 0;
    fclose(lex); lex = 0;
    frame->wx_menu_bar->Enable(WXWW_BUILDDICT, 1);
    frame->wx_menu_bar->Enable(WXWW_INTERRUPT, 0);
    frame->wx_menu_bar->Enable(WXWW_RESUME, 1);
}

DictionaryBuilder::DictionaryBuilder(wxFrame *frame_in, 
					FILE *lex_in, FILE *dawg_in,
					int restore)
    : frame(frame_in), 
      botnode((nodenum)1),
      topnode((nodenum)0),
      freeNodes(MAXNODES-1l),
      wcnt(0),
      stkTop(0),
      lex(lex_in), dawg(dawg_in),
      running(1)
{
    AllocateMem(MAXNODES);
    Setup(restore ? lex : 0);
    if (!IsRegistered()) return;
    AddCycler(::DoWord, this);
    frame->wx_menu_bar->Enable(WXWW_BUILDDICT, 0);
    frame->wx_menu_bar->Enable(WXWW_INTERRUPT, 1);
    frame->wx_menu_bar->Enable(WXWW_RESUME, 0);
}

DictionaryBuilder::~DictionaryBuilder()
{
    if (running && IsRegistered())
    {
	RemoveCycler(::DoWord);
        must_abort = 1;
	DoWord();
    }
    Free();
}


