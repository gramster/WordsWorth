/*
 * File:	xword.h
 * Purpose:	XWord for WxWindows crossword class
 * Author:	Graham Wheeler
 * Created:	1996
 * Updated:	
 * Copyright:	(c) 1996, Graham Wheeler
 */

#ifndef XWORD_H
#define XWORD_H

const int MAX_XWORD = 25; // largest allowed crossword (affects sliders)

/* xword flags */

#define AUTOPUT		1
#define ANYA		2
#define ANYD		4
#define STARTA 		8
#define STARTD 		16
#define RECOMPUTE	32

#define MAX_BUF_WORDS	8
#define MAX_WORD_LEN	20

class Hash;

class CrossWord : public Game
{
    char choices[28];
    int width, height;	// dimensions
    int rnow, cnow;	// logical cursor position
    char **letters;	// letter grid
    unsigned **flags;	// flag grid
    unsigned long **aChk,// alphabet constraint grids
		  **dChk,
		  **initChk,
		  **globalChk,
		  *tmpChk;
    char	***topicsD, ***topicsA;
    char	***anagD, ***anagA;
    // a small word buffer kludge for the user interface
    char	dwords[MAX_BUF_WORDS][MAX_WORD_LEN];
    char	awords[MAX_BUF_WORDS][MAX_WORD_LEN];
    int		autoskip;	// cursor advance direction
    FILE	*logfp;
    int		xword_len, xword_dir, xword_r, xword_c, 
		xword_apos, xword_dpos;
    char	xWord[MAX_WORD_LEN];
    long	xword_acnt, xword_dcnt;
    int		crunch;
    int		lock;
    int		symmetry;
    Dictionary	*dict;
    Hash	*thash;
    int		longest_dict_word;

    int Allocate(int width_in, int height_in);
    void Free();
    void AddCrossWord(char *word);
    // logical cursor routines

    int AtLeft() const;
    int AtRight() const;
    int AtTop() const;
    int AtBottom() const;

    // board queries

    int IsDecideable(int r, int c);
    void ClearLetter(int r, int c);
    int SetLetter(int r, int c, char ch);
    void SingleInsert(int r, int c, char ch);
    void ClearGrid();

    int RecursiveSearch(int pos, long n);
    int XWordSearch();
    void ComputeCrossCheck(int r, int c, unsigned dir);
  public:

    // crossword dimensions
    int Width() const;
    int Height() const;

    // cursor movement
    int Row() const;
    int Col() const;
    void SetRow(int r);
    void SetCol(int c);
    void Home();
    void End();
    void Top();
    void Bottom();
    void Up();
    void Down();
    void Left();
    void Right();
    void ToggleAutoSkip();
    void AdvanceCursor();

    // grid queries
    int GetSymmetry() const;
    int IsTopmostLetter(int r, int c) const;
    int IsLeftmostLetter(int r, int c) const;
    int IsBottommostLetter(int r, int c) const;
    int IsRightmostLetter(int r, int c) const;
    int IsBlack(int r, int c) const;
    int IsEmpty(int r, int c) const;
    int IsStartOfAcrossWord(int r, int c) const;
    int IsStartOfDownWord(int r, int c) const;
    int BypassDictionaryAcross(int r, int c) const;
    int BypassDictionaryDown(int r, int c) const;
    int BypassDictionaryWordAcross(int r, int c) const;
    int BypassDictionaryWordDown(int r, int c) const;
    char Letter(int r, int c) const;
    int AutoPlaced(int r, int c) const;
    void ClearAutoPlaced(int r, int c);
    int IsInAcrossWord(int r, int c) const;
    int IsInDownWord(int r, int c) const;
    char *GetDownWord(int n) const;
    char *GetAcrossWord(int n) const;

    // grid modification
    void SetSymmetry(int symtype);
    void Lock();
    void Unlock();
    void Flip();
    void ReflectVertical();
    void ReflectHorizontal();
    void StartWordAcross(int r, int c);
    void StartWordDown(int r, int c);
    void AllowAnyAcross(int r, int c);
    void AllowAnyDown(int r, int c);
    void Insert(int r, int c, char ch);
    int AddLetter(int r, int c, char ch);

    // constraints
    int GetCrunch() const;
    void SetCrunch(int v);
    void SetInitialConstraints(int r, int c, char *l);
    char *GetInitialConstraints(int r, int c);
    void ClearChecks(int doGlobals, int doInits = 0);
    void ComputeGlobalChecks();
    int ComputeLocalCheck(int r, int c, int dir);
    void SetLogFile(char *fname);
    char *GetPossibleLetters(int r, int c, int &acnt, int &dcnt);
    void ClearFlags(int r, int c);
    void ClearAcrossFlags(int r, int c);
    void ClearDownFlags(int r, int c);
    void SetAcrossTopic(int r, int c, char *topic);
    void SetDownTopic(int r, int c, char *topic);
    char *GetAcrossTopic(int r, int c);
    char *GetDownTopic(int r, int c);
    void SetAcrossAnagram(int r, int c, char *anagram);
    void SetDownAnagram(int r, int c, char *anagram);
    char *GetAcrossAnagram(int r, int c);
    char *GetDownAnagram(int r, int c);


    // Save and load
    virtual void Save(FILE *fp);
    virtual int Load(FILE *fp);

    // construct/destruct
    CrossWord(int width_in, int height_in, Dictionary *dict_in);
    CrossWord(Dictionary *dict_in); // must load from file
    ~CrossWord();
};

inline void CrossWord::ToggleAutoSkip()
{
    if (autoskip >= 0) autoskip = -1;
    else autoskip = 1;
}

inline void CrossWord::Lock()
{
    lock = 1;
    symmetry = 0;
}

inline void CrossWord::Unlock()
{
    lock = 0;
}

inline int CrossWord::Width() const
{
    return width;
}

inline int CrossWord::Height() const
{
    return height;
}

inline int CrossWord::Row() const 
{
    return rnow;
}

inline int CrossWord::Col() const 
{
    return cnow;
}

inline void CrossWord::SetRow(int r)
{
    rnow = r;
}

inline void CrossWord::SetCol(int c)
{
    cnow = c;
}

inline int CrossWord::AtLeft() const
{
    return (cnow == 0);
}

inline int CrossWord::AtRight() const
{
    return (cnow == (width-1));
}

inline int CrossWord::AtTop() const
{
    return (rnow == 0);
}

inline int CrossWord::AtBottom() const
{
    return (rnow == (height-1));
}

inline void CrossWord::Home()
{
    cnow = 0;
}

inline void CrossWord::End()
{
    cnow = width-1;
}

inline void CrossWord::Top()
{
    rnow = 0;
}

inline void CrossWord::Bottom()
{
    rnow = height-1;
}

inline void CrossWord::Up()
{
    if (!AtTop()) rnow--;
}

inline void CrossWord::Down()
{
    if (!AtBottom()) rnow++;
    if (autoskip==1) autoskip = 0;
}

inline void CrossWord::Left()
{
    if (!AtLeft()) cnow--;
}

inline void CrossWord::Right()
{
    if (!AtRight()) cnow++;
    if (autoskip==0) autoskip = 1;
}

inline char CrossWord::Letter(int r, int c) const
{
    return letters ? letters[r][c] : '?';
}

inline int CrossWord::IsBlack(int r, int c) const
{
    return (letters && letters[r][c] == '#');
}

inline int CrossWord::AutoPlaced(int r, int c) const
{
    return (flags && (flags[r][c] & AUTOPUT) == AUTOPUT);
}

inline void CrossWord::ClearAutoPlaced(int r, int c)
{
    if (flags) flags[r][c] &= ~AUTOPUT;
    if (letters) letters[r][c] = '?';
}

inline int CrossWord::IsEmpty(int r, int c) const
{
    return (letters && letters[r][c] == '?');
}

inline int CrossWord::IsLeftmostLetter(int r, int c) const
{
    return (c==0 || letters[r][c-1] == '#');
}

inline int CrossWord::IsTopmostLetter(int r, int c) const
{
    return (r==0 || letters[r-1][c] == '#');
}

inline int CrossWord::IsRightmostLetter(int r, int c) const
{
    return (c==(width-1) || letters[r][c+1] == '#');
}

inline int CrossWord::IsBottommostLetter(int r, int c) const
{
    return (r==(height-1) || letters[r+1][c] == '#');
}

inline void CrossWord::ClearFlags(int r, int c)
{
    flags[r][c] &= ~(ANYA|ANYD|STARTA|STARTD);
}

inline void CrossWord::ClearAcrossFlags(int r, int c)
{
    flags[r][c] &= ~(ANYA|STARTA);
}

inline void CrossWord::ClearDownFlags(int r, int c)
{
    flags[r][c] &= ~(ANYD|STARTD);
}

inline void CrossWord::AllowAnyAcross(int r, int c)
{
    flags[r][c] |= ANYA;
}

inline void CrossWord::AllowAnyDown(int r, int c)
{
    flags[r][c] |= ANYD;
}

inline void CrossWord::StartWordAcross(int r, int c)
{
    flags[r][c] |= STARTA;
}

inline void CrossWord::StartWordDown(int r, int c)
{
    flags[r][c] |= STARTD;
}

inline char *CrossWord::GetDownWord(int n) const
{
    return (char *)&dwords[n][0];
}

inline char *CrossWord::GetAcrossWord(int n) const
{
    return (char *)&awords[n][0];
}

inline void CrossWord::SetCrunch(int v)
{
    crunch = v;
}

inline int CrossWord::GetCrunch() const
{
    return crunch;
}

inline int CrossWord::GetSymmetry() const
{
    return symmetry;
}

inline void CrossWord::SetSymmetry(int symtyp)
{
    symmetry = symtyp;
}

inline int CrossWord::BypassDictionaryAcross(int r, int c) const
{
    return ((flags[r][c] & ANYA) == ANYA);
}

inline int CrossWord::BypassDictionaryDown(int r, int c) const
{
    return ((flags[r][c] & ANYD) == ANYD);
}

#endif

