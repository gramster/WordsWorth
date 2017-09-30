/*
 * File:	xword.cc
 * Purpose:	XWord for WxWindows crossword class
 * Author:	Graham Wheeler
 * Created:	1996
 * Updated:	
 * Copyright:	(c) 1996, Graham Wheeler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#ifdef __GNUG__
#pragma implementation
#endif

#include "dict.h"
#include "ggame.h"
#include "xword.h"

template <class T> int AllocGrid(T **&grid, int width, int height)
{
    grid = new T *[height];
    if (grid)
    {
        for (int i = 0; i < height; i++)
	{
	    grid[i] = new T[width];
	    if (grid[i])
	        memset(grid[i], 0, width*sizeof(T));
	    else
	    {
	        while (--i >= 0) delete [] grid[i];
		delete [] grid;
		grid = 0;
		return -1;
	    }
	}
    }
    else return -1;
    return 0;
}

template <class T> void FreeGrid(T **&grid, int height)
{
    if (grid)
    {
        for (int i = 0; i < height; i++)
	    delete [] grid[i];
        delete [] grid;
	grid = 0;
    }
}

void FreeStringGrid(char ***&grid, int width, int height)
{
    if (grid)
    {
        for (int i = 0; i < height; i++)
	{
	    if (grid[i])
	    {
	        for (int j = 0; j < width; j++)
		    delete [] grid[i][j];
	        delete [] grid[i];
	    }
	}
        delete [] grid;
	grid = 0;
    }
}

//-----------------------------------------------------------------------
// Note that we should really make a square class which has each of
// these attributes, rather than doing the grid allocation we do at
// present...

int CrossWord::Allocate(int width_in, int height_in)
{
    height = height_in;
    width = width_in;
    if (AllocGrid(letters, width, height) == 0	&&
        AllocGrid(flags, width, height) == 0	&&
        AllocGrid(aChk, width, height) == 0	&&
        AllocGrid(dChk, width, height) == 0	&&
        AllocGrid(initChk, width, height) == 0	&&
        AllocGrid(globalChk, width, height) == 0&&
        AllocGrid(topicsA, width, height) == 0	&&
	AllocGrid(topicsD, width, height) == 0	&&
	AllocGrid(anagA, width, height) == 0	&&
	AllocGrid(anagD, width, height) == 0	&&
        (tmpChk = new unsigned long[(height>width?height:width)]) != 0)
	    return 0;
    Free();
    return -1;
}

        
void CrossWord::Free()
{
    FreeGrid(letters, height);
    FreeGrid(flags, height);
    FreeGrid(aChk, height);
    FreeGrid(dChk, height);
    FreeGrid(initChk, height);
    FreeGrid(globalChk, height);
    FreeStringGrid(topicsA, width, height);
    FreeStringGrid(topicsD, width, height);
    FreeStringGrid(anagA, width, height);
    FreeStringGrid(anagD, width, height);
    delete [] tmpChk;
    tmpChk = 0;
}

int CrossWord::IsStartOfAcrossWord(int r, int c) const
{
    if (IsBlack(r, c)) return 0;
    if ((flags[r][c] & STARTA) == STARTA) return 1;
    if (!IsLeftmostLetter(r,c)) return 0;
    return (c < (width-1) && !IsBlack(r, c+1));
}

int CrossWord::IsStartOfDownWord(int r, int c) const
{
    if (IsBlack(r, c)) return 0;
    if ((flags[r][c] & STARTD) == STARTD) return 1;
    if (!IsTopmostLetter(r,c)) return 0;
    return (r < (height-1) && !IsBlack(r+1, c));
}

int CrossWord::IsInAcrossWord(int r, int c) const
{
    if (IsBlack(r, c)) return 0;
    return ((c>0 && !IsBlack(r, c-1)) ||
	(c<(width-1) && !IsBlack(r, c+1)));
}

int CrossWord::IsInDownWord(int r, int c) const
{
    if (IsBlack(r, c)) return 0;
    return ((r>0 && !IsBlack(r-1, c)) ||
	(r<(height-1) && !IsBlack(r+1, c)));
}

int CrossWord::BypassDictionaryWordAcross(int r, int c) const
{
    int tc = c, ba = 1;
    while (tc < Width() && !IsBlack(r, tc))
    {
        if (IsEmpty(r, tc) && !BypassDictionaryAcross(r, tc))
	{
  	    ba = 0;
  	    break;
        }
  	else tc++;
    }
    return ba;
}

int CrossWord::BypassDictionaryWordDown(int r, int c) const
{
    int tr = r, ba = 1;
    while (tr < Height() && !IsBlack(tr, c))
    {
        if (IsEmpty(tr, c) && !BypassDictionaryDown(tr, c))
	{
  	    ba = 0;
  	    break;
        }
  	else tr++;
    }
    return ba;
}

void CrossWord::SetDownTopic(int r, int c, char *topic)
{
    if (topicsD && topicsD[r])
    {
        delete [] topicsD[r][c];
	if (topic && topic[0])
	{
	    topicsD[r][c] = new char[strlen(topic)+1];
	    strcpy(topicsD[r][c], topic);
	}
	else topicsD[r][c] = 0;
    }
}

void CrossWord::SetAcrossTopic(int r, int c, char *topic)
{
    if (topicsA && topicsA[r])
    {
        delete [] topicsA[r][c];
	if (topic && topic[0])
	{
	    topicsA[r][c] = new char[strlen(topic)+1];
	    strcpy(topicsA[r][c], topic);
	}
	else topicsA[r][c] = 0;
    }
}

char *CrossWord::GetAcrossTopic(int r, int c)
{
    if (topicsA && topicsA[r] && topicsA[r][c]) return topicsA[r][c];
    return "";
}

char *CrossWord::GetDownTopic(int r, int c)
{
    if (topicsD && topicsD[r] && topicsD[r][c]) return topicsD[r][c];
    return "";
}

void CrossWord::SetDownAnagram(int r, int c, char *anagram)
{
    if (anagD && anagD[r])
    {
        delete [] anagD[r][c];
	if (anagram && anagram[0])
	{
	    anagD[r][c] = new char[strlen(anagram)+1];
	    strcpy(anagD[r][c], anagram);
	}
	else anagD[r][c] = 0;
    }
}

void CrossWord::SetAcrossAnagram(int r, int c, char *anagram)
{
    if (anagA && anagA[r])
    {
        delete [] anagA[r][c];
	if (anagram && anagram[0])
	{
	    anagA[r][c] = new char[strlen(anagram)+1];
	    strcpy(anagA[r][c], anagram);
	}
	else anagA[r][c] = 0;
    }
}

char *CrossWord::GetAcrossAnagram(int r, int c)
{
    if (anagA && anagA[r] && anagA[r][c]) return anagA[r][c];
    return "";
}

char *CrossWord::GetDownAnagram(int r, int c)
{
    if (anagD && anagD[r] && anagD[r][c]) return anagD[r][c];
    return "";
}

void CrossWord::SingleInsert(int r, int c, char ch)
{
    flags[r][c] &= ~AUTOPUT;
    if (letters[r][c]!=ch)
    {
        if (ch=='?') globalChk[r][c] = 0x3FFFFFFFl;
        if (letters[r][c]!='?' || (ch<'A' || ch>'Z'))
            ClearChecks(1);
        letters[r][c] = ch;
    }
}

void CrossWord::Insert(int r, int c, char ch)
{
    rnow = r;
    cnow = c;
    // normalise character
    if (ch>='a' && ch<='z') ch -= 'a'-'A';
    if (ch == ' ') ch = '?';
    if (ch=='?' || ch=='#' || (ch>='A' && ch<='Z'))
    {
	if (lock) // grid locked?
	{
	    if (ch!='#' && letters[rnow][cnow]!='#')
		SingleInsert(rnow, cnow, ch);
	    return;
	}
	SingleInsert(rnow, cnow, ch);
	if (ch=='?' || ch=='#')
	{
	    switch(symmetry)
	    {
	    case 1: // left to right
		SingleInsert(rnow, width-cnow-1, ch);
		break;
	    case 2: // top to bottom
		SingleInsert(height-rnow-1, cnow, ch);
		break;
	    case 3: // top left quadrant
		SingleInsert(rnow, width-cnow-1, ch);
		SingleInsert(height-rnow-1, cnow, ch);
		SingleInsert(height-rnow-1, width-cnow-1, ch);
		break;
	    case 4: // top left quadrant
		SingleInsert(height-rnow-1, width-cnow-1, ch);
	    }
	}
    }
    else if (rnow>=0)
	AddLetter(rnow, cnow, ch); // handle flag modifier chars
}

void CrossWord::AdvanceCursor()
{
    if (autoskip<0) return;
    if (lock)
    {
        if (autoskip)
        {
	    if (IsRightmostLetter(rnow, cnow)) 
	    {
		if (!IsBottommostLetter(rnow, cnow))
		{
		    autoskip = 0;
	            Down();
		}
	    }
	    else Right();
	}
        else
        {
	    if (IsBottommostLetter(rnow, cnow)) 
	    {
		if (!IsRightmostLetter(rnow, cnow))
		{
		    autoskip = 1;
	            Right();
		}
	    }
	    else Down();
        }
    }
    else
    {
        if (autoskip)
        {
	    if (AtRight())
	    {
		autoskip = 0;
	        Down();
	    }
	    else Right();
	}
        else
        {
	    if (AtBottom())
	    {
		autoskip = 1;
	        Right();
	    }
	    else Down();
        }
    }
}

void CrossWord::ClearChecks(int doGlobals, int doInits)
{
    int r, c;
    for (r=0; r<height; r++)
    {
	for (c=0; c<width; c++)
	{
	    aChk[r][c] = dChk[r][c] = 0l;
	    if (doInits)
		initChk[r][c] = 0x3FFFFFFFl;
	    if (doGlobals)
	    {
		globalChk[r][c] = initChk[r][c];
		if (flags[r][c] & AUTOPUT)
		{
		    letters[r][c] = '?';
		    flags[r][c] &= ~AUTOPUT;
		}
	    }
	}
    }
}

int CrossWord::IsDecideable(int r, int c) // ok
{
    int p, v = -1;
    unsigned long chk = globalChk[r][c];
    for (p=0; p<26; p++)
        if (chk & (1l << p))
        {
            if (v==-1) v = p;
            else return 0;
        }
    if (v>=0 && letters[r][c] != v+'A')
    {
        letters[r][c] = v+'A'; /* Only one choice */
        return 1;
    }
    return 0;
}

void CrossWord::SetLogFile(char *fname)
{
    if (logfp) fclose(logfp);
    if (fname == 0) logfp = 0;
    else logfp = fopen(fname, "w");
}

void CrossWord::AddCrossWord(char *word)
{
    if (xword_dir) // down?
    {
        /* Down word */
	if (xword_dcnt < MAX_BUF_WORDS)
	    strcpy(dwords[xword_dcnt], word);
	if (logfp) fprintf(logfp,"%-7ld%s\n", xword_dcnt+1, word);
        for (int r = xword_r, l = xword_len; l--; r++)
            dChk[r][xword_c] |= 1l << ( (*word++) - 'A' );
        xword_dcnt++;
    }
    else
    {
        /* Across word */
	if (xword_acnt < MAX_BUF_WORDS)
	    strcpy(awords[xword_acnt], word);
	if (logfp) fprintf(logfp,"%-7ld%s\n", xword_acnt+1, word);
        for (int c = xword_c, l = xword_len; l--; c++)
            aChk[xword_r][c] |= 1l << ( (*word++) - 'A' );
        xword_acnt++;
    }
}

void CrossWord::ClearLetter(int r, int c)
{
    if (lock==0 || letters[r][c] != '#')
    {
        letters[r][c] = '?';
        flags[r][c] = 0;
    }
}

int CrossWord::AddLetter(int r, int c, char ch)
{
    if (lock)
    {
	if (letters[r][c] == '#' || ch=='#')
	    return -1;
    }
    switch(ch)
    {
    case '.':
    case '?':
        letters[r][c] = '?';
        break;
    case '#':
        letters[r][c] = '#';
        break;
    case '^':
        if ((flags[r][c] & STARTD) != 0)
	    flags[r][c] &= ~STARTD;
	else
	    flags[r][c] |= STARTD;
        break;
    case '>':
        if ((flags[r][c] & STARTA) != 0)
	    flags[r][c] &= ~STARTA;
	else
	    flags[r][c] |= STARTA;
        break;
    case '+':
        if ((flags[r][c] & ANYA|ANYD) == (ANYA|ANYD))
            flags[r][c] &= ~(ANYA|ANYD);
	else
            flags[r][c] |= ANYA|ANYD;
        break;
    case '|':
        if ((flags[r][c] & ANYD) != 0)
	    flags[r][c] &= ~ANYD;
	else
	    flags[r][c] |= ANYD;
        break;
    case '-':
        if ((flags[r][c] & ANYA) != 0)
	    flags[r][c] &= ~ANYA;
	else
	    flags[r][c] |= ANYA;
        break;
    default:
        if (ch>='A' && ch<='Z')
            letters[r][c] = ch;
        else return -1;
        break;
    }
    return 0;
}

int CrossWord::SetLetter(int r, int c, char ch)
{
    ClearLetter(r,c);
    return AddLetter(r,c,ch);
}

void CrossWord::Save(FILE *outfp)
{
    int r, c;
    fprintf(outfp,"%d %d\n\n; Created by XWord\n\n", height, width);
    for (r=0; r<height; r++)
    {
        for (c=0; c<width; c++)
        {
            if (letters[r][c]=='?' || (flags[r][c]&AUTOPUT))
                fputc('.', outfp);
            else fputc(letters[r][c], outfp);
        }
        fputc('\n', outfp);
    }
    fputc('\n', outfp);
    fputc('\n', outfp);
    for (r=0; r<height; r++)
    {
        for (c=0; c<width; c++)
        {
            if (flags[r][c]&ANYA)
                fprintf(outfp, "%d %d -\n", r+1, c+1);
            if (flags[r][c]&ANYD)
                fprintf(outfp, "%d %d |\n", r+1, c+1);
            if (flags[r][c]&STARTD)
                fprintf(outfp, "%d %d ^\n", r+1, c+1);
            if (flags[r][c]&STARTA)
                fprintf(outfp, "%d %d >\n", r+1, c+1);
	    if (initChk[r][c] != 0x3FFFFFFl)
                fprintf(outfp, "%d %d %ld\n", r+1, c+1, initChk[r][c]);
	    if (topicsA && topicsA[r] && topicsA[r][c])
                fprintf(outfp, "%d %d =%s\n", r+1, c+1, topicsA[r][c]);
	    if (topicsD && topicsD[r] && topicsD[r][c])
                fprintf(outfp, "%d %d :%s\n", r+1, c+1, topicsD[r][c]);
	    if (anagA && anagA[r] && anagA[r][c])
                fprintf(outfp, "%d %d &%s\n", r+1, c+1, anagA[r][c]);
	    if (anagD && anagD[r] && anagD[r][c])
                fprintf(outfp, "%d %d *%s\n", r+1, c+1, anagD[r][c]);
        }
    }
    fputc('\n', outfp);
}

int CrossWord::Load(FILE *fp)
{
    int ch, r = 0, c = 0;
    int oldlock = lock;
    lock = 0;
    if (fscanf(fp, "%d %d", &height, &width)!=2)
        return -1;
    //Free();
    if (Allocate(width, height) != 0)
	return -1;
    ch = fgetc(fp);
    while (!feof(fp))
    {
        if (ch==';') /* comment */
        {
            do  ch = fgetc(fp);
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
            if (SetLetter(r,c,ch)==0) c++;
            if (c==width)
            {
                r++;
                if (r==height) break;
                c=0;
            }
            ch = fgetc(fp);
        }
    }
    ClearChecks(1, 1);
    while (!feof(fp))
    {
        int r2, c2;
	long msk;
	char buff[256];
	char topic[128];
	char c;
	if (fgets(buff, 80, fp)==0) break;
        if (sscanf(buff, "%d %d %ld", &r2, &c2, &msk)==3)
	    initChk[r2-1][c2-1] = msk;
        else if (sscanf(buff, "%d %d %c%s", &r2, &c2, &c, topic)>=3)
	{
	    if (c == '=')
	        SetAcrossTopic(r2-1,c2-1,topic);
	    else if (c == ':')
	        SetDownTopic(r2-1,c2-1,topic);
	    else if (c == '&')
	        SetAcrossAnagram(r2-1,c2-1,topic);
	    else if (c == '*')
	        SetDownAnagram(r2-1,c2-1,topic);
	    else 
	        AddLetter(r2-1,c2-1,c);
	}
    }
    xword_acnt = xword_dcnt = 0l;
    rnow = cnow = 0;
    lock = oldlock;
    return 0; //width*height;
}

void CrossWord::Flip()
{
    /* flip top half */
    if (lock) return;
    for (int r=0; r<=(height/2); r++)
        for (int c=0; c<=(width/2); c++)
        {
            char ch = letters[r][c];
            letters[r][c] = letters[r][width-c-1];
            letters[r][width-c-1] = ch;
        }
    ClearChecks(1);
}

void CrossWord::ReflectVertical()
{
    if (lock) return;
    /* reflect left half to right half */
    for (int r=0; r<height; r++)
        for (int c=0; c<(width/2); c++)
            letters[r][width-c-1] = (letters[r][c]=='#') ? '#' : '?';
    ClearChecks(1);
}

void CrossWord::ReflectHorizontal()
{
    if (lock) return;
    /* reflect top half to bottom half */
    for (int r=0; r<(height/2); r++)
        for (int c=0; c<width; c++)
            letters[height-r-1][width-c-1] = (letters[r][c]=='#') ? '#' : '?';
    ClearChecks(1);
}

int CrossWord::RecursiveSearch(int pos, long n)
{
    while (n)
    {
        Node N = dict->GetNode(n);
        int c = N.NodeIndex()-1;
        if (((1l<<c) & tmpChk[pos]) != 0l)
        {
            xWord[pos] = (char)(c+'A');
            if (pos==(xword_len-1))
            {
                if (N.IsWordEnd())
                {
                    xWord[pos+1]=0;
		    if ((thash==0 || thash->Lookup(xWord)==1) &&
			dict->IsAnagram(xWord))
			    AddCrossWord(xWord);
                }
            }
            else RecursiveSearch(pos+1, N.NextNode());
        }
        /* Move to next edge */
        if (!N.IsLastChild()) n++;
        else break;
    }
    return 0;
}

void CrossWord::ClearGrid()
{
    if (lock) return;
    for (int r=0; r<height; r++)
        for (int c=0; c<width; c++)
        {
            letters[r][c] = '#';
            flags[r][c] = 0;
        }
}

int CrossWord::XWordSearch()
{
    // Prep the thesaurus
    char *t = 0, *a = 0;
    if (xword_dir==0) // across?
    {
        t = topicsA[xword_r][xword_c];
	a = anagA[xword_r][xword_c];
    }
    else
    {
        t = topicsD[xword_r][xword_c];
	a = anagD[xword_r][xword_c];
    }
    dict->SetAnagramConstraint(a);
    if (t) thash = dict->GetThesaurusWords(t, xword_len, tmpChk);
    int rtn = RecursiveSearch(0, 1l);
    delete thash;
    thash = 0;
    return rtn;
}

void CrossWord::ComputeCrossCheck(int r, int c, unsigned dir)
{
    int start, end;
    ClearChecks(0);
    xword_acnt = 0l;
    xword_dcnt = 0l;
    if (flags[r][c] & ANYA)
        globalChk[r][c] = aChk[r][c] = initChk[r][c];
    else if ((dir & ANYA)==0)
        aChk[r][c] = globalChk[r][c];
    else
    {
        /* find across words */
        xword_r = r;
        xword_dir = 0;
        for (start = c; start>0; start--)
            if (letters[r][start-1]=='#' || (flags[r][start]&STARTA))
                break;
        for (end = c; end<(width-1); end++)
            if (letters[r][end+1]=='#' || (flags[r][end+1]&STARTA))
                break;
        xword_len = end-start+1;
        if (xword_len>longest_dict_word)
	{
            while (start<=end)
	    {
                aChk[r][start] = globalChk[r][start];
		start++;
	    }
	}
        else if (xword_len>1)
        {
            xword_c = start;
	    char *a = GetAcrossAnagram(r, start);
	    if (a[0]==0) a = 0;
	    short acnt[26];
	    if (a) BuildAnagramVector(a, acnt);
            while (start<=end)
            {
                if (letters[r][start] == '?')
		{
		    unsigned long v = globalChk[r][start];
		    if (a) v &= BuildAnagramMask(acnt);
                    tmpChk[start-xword_c] = v;
		}
                else
		{
		    int n = (letters[r][start] - 'A');
                    tmpChk[start-xword_c] = 1l << n;
		    if (a)
		        if (acnt[n]<=0) SetDownAnagram(r, xword_c, 0);
			else acnt[n]--;
		}
                start++;
            }
            if (XWordSearch())
            {
                dChk[r][c] = aChk[r][c] = globalChk[r][c];
                goto done;
            }
        }
        else aChk[r][c] = globalChk[r][c];
    }
    if (flags[r][c]&ANYD)
        dChk[r][c] = initChk[r][c];
    else if ((dir & ANYD)==0)
        dChk[r][c] = globalChk[r][c];
    else
    {
        /* find down words */

        xword_c = c;
        xword_dir = 1;
        for (start = r; start>0; start--)
            if (letters[start-1][c]=='#' || (flags[start][c]&STARTD))
                break;
        for (end = r; end<(height-1); end++)
            if (letters[end+1][c]=='#' || (flags[end+1][c]&STARTD))
                break;
        xword_len = end-start+1;
        if (xword_len>longest_dict_word)
	{
            while (start<=end)
	    {
                dChk[start][c] = globalChk[start][c];
		start++;
	    }
	}
        else if (xword_len>1)
        {
            xword_r = start;
	    char *a = GetDownAnagram(start, c);
	    if (a[0]==0) a = 0;
	    short acnt[26];
	    if (a) BuildAnagramVector(a, acnt);
            while (start<=end)
            {
                if (letters[start][c] == '?')
		{
		    unsigned long v = globalChk[start][c];
		    if (a) v &= BuildAnagramMask(acnt);
                    tmpChk[start-xword_r] = v;
		}
                else
		{
                    int n = (letters[start][c] - 'A');
                    tmpChk[start-xword_r] = 1l << n;
		    if (a)
		        if (acnt[n]<=0) SetDownAnagram(xword_r, c, 0);
			else acnt[n]--;
		}
                start++;
            }
            if (XWordSearch())
	    {
                dChk[r][c] = globalChk[r][c];
		goto done;
	    }
        }
        else dChk[r][c] = globalChk[r][c];
    }
done:
    globalChk[r][c] = (aChk[r][c] & dChk[r][c]);
    if (IsDecideable(r, c))
        flags[r][c] |= AUTOPUT;
}

void CrossWord::ComputeGlobalChecks()
{
    int pass=0, change = 0;
    ClearChecks(0);
    pass = 0;
    for (int r=0; r<height; r++)
        for (int c=0; c<width; c++)
            if (letters[r][c]=='?')
                flags[r][c] |= RECOMPUTE;
            else if (letters[r][c]=='#')
                globalChk[r][c] = 0l;
            else if (letters[r][c]!='?')
                globalChk[r][c] = 1l << (letters[r][c]-'A');
    do
    {
        change = 0;
        pass++;
        for (int r=0; r<height; r++)
            for (int c=0; c<width; c++)
            {
                unsigned long mask = globalChk[r][c];
                if (flags[r][c]&RECOMPUTE)
                {
                    ComputeCrossCheck(r, c, (ANYA|ANYD));
                    flags[r][c] &= ~RECOMPUTE;
                }
                if (mask != globalChk[r][c])
                {
                    int r2, c2;
                    change++;
                    /* Try to limit the effects a bit */
                    r2 = r; c2 = c;
                    while (r2>=0 && letters[r2][c2]!='#')
                    {
                        if (letters[r2][c2]=='?')
                            flags[r2][c2] |= RECOMPUTE;
                        r2--;
                    }
                    r2 = r;
                    while (r2<height && letters[r2][c2]!='#')
                    {
                        if (letters[r2][c2]=='?')
                            flags[r2][c2] |= RECOMPUTE;
                        r2++;
                    }
                    r2 = r;
                    while (c2>=0 && letters[r2][c2]!='#')
                    {
                        if (letters[r2][c2]=='?')
                            flags[r2][c2] |= RECOMPUTE;
                        c2--;
                    }
                    c2 = c;
                    while (c2<width && letters[r2][c2]!='#')
                    {
                        if (letters[r2][c2]=='?')
                            flags[r2][c2] |= RECOMPUTE;
                        c2++;
                    }
                }
            }
    }
    while (change);
}

int CrossWord::ComputeLocalCheck(int r, int c, int dir)
{
    for (int i = 0; i < MAX_BUF_WORDS; i++)
	dwords[i][0] = awords[i][0] = 0;
    unsigned long mask = globalChk[r][c];
    if (letters[r][c] == '#')
        globalChk[r][c] = aChk[r][c] = dChk[r][c] = 0l;
    else if (letters[r][c] != '?' && (flags[r][c]&AUTOPUT)==0)
        globalChk[r][c] = aChk[r][c] = dChk[r][c] = 1l << (letters[r][c]-'A');
    else if (crunch)
    {
        if (crunch==2)
        {
            globalChk[r][c] = initChk[r][c];
            dir = ANYA|ANYD;
        }
        ComputeCrossCheck(r,c,dir);
    }
    else aChk[r][c] = dChk[r][c] = globalChk[r][c];
    return (mask!=globalChk[r][c]);
}

char *CrossWord::GetPossibleLetters(int r, int c, int &acnt, int &dcnt)
{
    (void)ComputeLocalCheck(r, c, ANYA|ANYD);
    unsigned long mask = globalChk[rnow][cnow];
    int p = 0;
    for (int i=0; i<26; i++)
        if (mask & (1l << i))
	    choices[p++] = 'A'+i;
    choices[p] = 0;
    acnt = xword_acnt;
    dcnt = xword_dcnt;
    return choices;
}

void CrossWord::SetInitialConstraints(int r, int c, char *l)
{
    initChk[r][c] = 0l;
    while (*l)
    {
	char c = *l;
	if (islower(c)) c = toupper(c);
	if (isupper(c))	initChk[r][c] |= (1l << (c - 'A'));
	l++;
    }
}

char *CrossWord::GetInitialConstraints(int r, int c)
{
    int p = 0;
    for (int i = 0; i < 26; i++)
    {
        if ((initChk[r][c] & (1l << i)) != 0l)
	    choices[p++] = 'A' + i;
    }
    choices[p] = 0;
    return choices;
}

CrossWord::CrossWord(int width_in, int height_in, Dictionary *dict_in)
    : width(0), height(0), rnow(0), cnow(0), letters(0), flags(0),
	aChk(0), dChk(0), initChk(0), globalChk(0), tmpChk(0),
	topicsA(0), topicsD(0), anagA(0), anagD(0),
        autoskip(1), logfp(0), xword_acnt(0), xword_dcnt(0),
	crunch(0), lock(0), symmetry(0), dict(dict_in), thash(0)
{
    (void)Allocate(width_in, height_in);
    ClearGrid();
    ClearChecks(1,1);
    longest_dict_word = dict->Longest();
}

CrossWord::CrossWord(Dictionary *dict_in) // must load from file
    : width(0), height(0), rnow(0), cnow(0), letters(0), flags(0),
	aChk(0), dChk(0), initChk(0), globalChk(0), tmpChk(0),
	topicsA(0), topicsD(0), anagA(0), anagD(0),
        autoskip(1), logfp(0), xword_acnt(0), xword_dcnt(0),
	crunch(0), lock(0), symmetry(0), dict(dict_in), thash(0)
{
    longest_dict_word = dict->Longest();
}

CrossWord::~CrossWord()
{
    Free();
}
