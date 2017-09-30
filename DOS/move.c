#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <conio.h>

#define GLOBAL extern

#include "dict.h"
#include "ww.h"

/*******************/
/* MOVE GENERATION */
/*******************/

static int movesWithThisScore,
		stopSearchNow,
		thisCount;

int HasTile(r, c)
{
	register char x=(char)(boardLetters[r][c] & 0x1F);
	return (x!=EMPTY && x!=BLACKSQ);
}

int GotAbove(int r, int c)	{ return (r>1	 && HasTile(r-1,c)); }
int GotBelow(int r, int c)	{ return (r<Rows && HasTile(r+1,c)); }
int GotLeft(int r, int c)	{ return (c>1	 && HasTile(r,c-1)); }
int GotRight(int r, int c)	{ return (c<Cols && HasTile(r,c+1)); }

static int HasNeighbour(int r, int c)
{
	return (GotAbove(r,c) || GotBelow(r,c) ||
		GotLeft(r,c) || GotRight(r,c));
}

/* return HUMAN or COMPUTER, -1 for empty, -2 for illegal */

static int Owner(int r, int c)
{
	if (r<1 || c<1 || r>Rows || c>Cols)
		return -2;
	if (!HasTile(r,c))
		return -1;
	if ((boardLetters[r][c]&0x40) == 0)
		return COMPUTER;
	return HUMAN;
}

static int HasTilesOnBoard(void)
{
	int i, j;
	for (i=1;i<=Rows;i++)
	{
		for (j=1;j<=Cols;j++)
		{
			if (Owner(i,j)==player)
				return 1;
		}
	}
	return 0;
}

static int IsTerritory(int r, int c)
{
	/* If player doesn't have any tiles on the board,
	   we consider the square as turf; else we check
	   for a neighbour of the same player */
	if (!HasTilesOnBoard() ||
	    Owner(r-1,c)==player || Owner(r+1,c)==player ||
	    Owner(r,c-1)==player || Owner(r,c+1)==player)
	    	return 1;
	return 0;
}

static int IsAnchor(int r, int c)
{
	if (isEmpty(r,c))
	{
		switch(freeForm)
		{
		case 0:
			return HasNeighbour(r,c);
		case 1:
			return IsTerritory(r,c);
		default:
			return 1;
		}
	}
	return 0;
}

/*
 * FindWildTile - find a blank on board for exchange moves
 */

int FindWildTile(int c, int *R, int *C)
{
	int i, j;
	/* convert character to board code for a blank representing
		that character, and search for such an entry in the
		board */
	c = (c-'A')|0x80;
	for (i = 1; i <=Rows; i++)
	{
		for (j=1; j<=Cols; j++)
		{
			if (boardLetters[i][j]==c) /* success! */
			{
				*R = i;
				*C = j;
				return 1;
			}
		}
	}
	return 0;
}

void useLetter(int p, char c)
{
	if (numtiles==1)
		takeFromHeap(c,1,950);
	else if (numtiles>1)
		takeFromRack(p,c,1,950);
}

void replaceLetter(int p, char c)
{
	if (numtiles==1)
		addToHeap(c,1,850);
	else if (numtiles>1)
		addToRack(p,c,1,850);
}

int GetTileCount(int p, char c)
{
	if (numtiles==0) /* infinite pool */
		return 999;
	else if (numtiles==1) /* common pool */
		return PoolCnt[c-'A'];
	else	/* playing from rack */
		return (Tiles[p][(c)-'A']);
}

/*
 * ComputeCrossCheck works out constraints on the board.
 * Given a row/col position, it returns a set (as a bitmask)
 * of characters that can be played in that position. If there
 * is already a tile there, it is the only letter returned.
 * Else we check if playing a letter here will result in a
 * new word being formed with existing tiles. In this case
 * we use the dictionary to determine which letters make
 * valid words. If no word is formed, the universal set is
 * returned. If the board square is a non-playable square,
 * the empty set is returned. The routine works out constraints
 * in one direction only, determined by the isAcross flag.
 */

static ulong ComputeCrossCheck(int row, int col)
{
	ulong rtn=0;
	if (boardScores[row][col]==BLACKCH)
		return 0l; // black square
	if (isEmpty(row,col))
	{
		char c, word[16];
		int pos=-1, i=0;
		/* We have an empty square. We now want to find the first
		   letter of the new word that would be formed perpendicular
		   to the placed word. We build up the word, and note the
		   position of the variable letter corresponding to the
		   empty square. We then invoke the dictionary to find all
		   the words that exist for this pattern.
		*/
		if (isAcross)
		{
			/* new word is vertical. Skip up to the first
				letter */
			while (GotAbove(row, col))
				row--;
			/* now move down to the end. When we hit the
				empty square save its word position.
				When we hit the bottom of the board
				or a second empty square, we're at the
				end... */
			while (row <= Rows)
			{
				if (isEmpty(row,col))
				{
					if (pos==-1) pos = i;
					else break;
					word[i] = ' ';
				}
				else if (HasTile(row,col))
					word[i] = letterAt(row,col);
				else break;
				row++;
				i++;
			}
		}
		else
		{
			/* new word is horizontal; symmetrical to above */
			while (GotLeft(row,col))
				col--;
			while (col<=Cols)
			{
				if (isEmpty(row,col))
				{
					if (pos==-1) pos = i;
					else break;
					word[i] = ' ';
				}
				else if (HasTile(row,col))
					word[i] = letterAt(row,col);
				else break;
				col++;i++;
			}
		}
		word[i]=0;
		(void)assert(pos!=-1);
		if (i==1) /* no word on board; just the empty space */
		{
			/* No neighbours */
			rtn = 0x3FFFFFFl; /* anything will do */
		}
		else for (c='A';c<='Z';c++)
		{
			/* check which chars are OK by plugging them
			   into the word at the empty position and
			   looking up the result in the dictionary */
			word[pos] = c;
			if (lookup(word)) /* Yes, there are matches */
			{
				rtn |= (1l<<(c-'A')); /* Add letter to mask */
			}
		}
		return rtn; /* return the set of valid letters */
	}
	else /* must be letter already in place */
		return 1l<<(boardLetters[row][col]&0x1F);
}

/*
 * Work out all constraints based on new across-moves
 */

static void ComputeAcrossCheck(int row)
{
	int col;
	isAcross = 1;
	XChkAcross[row][0] = XChkAcross[row][Cols+1] = 0l;
	for (col=1; col<=Cols; col++)
		XChkAcross[row][col] = ComputeCrossCheck(row,col);
}

/*
 * Work out all constraints based on new down-moves
 */

static void ComputeDownCheck(int col)
{
	int row;
	isAcross = 0;
	XChkDown[col][0] = XChkDown[col][Rows+1] = 0l;
	for (row=1; row<=Rows; row++)
		XChkDown[col][row] = ComputeCrossCheck(row,col);
}

static int IsSpecial(int r, int c, int mainmove)
{
	return (isEmpty(r,c) || (hotSquares + mainmove)>=2);
}

static int ComputeWeightAndScore(char *word, int srow, int scol,
	int isAcross, int *score, int mainmove)
{
	int r, c, l = (int)strlen(word), scr = 0, used = 0, w8,
		ltrW8=0;
	if (isAcross)
	{
		r = srow;
		/* calculate normal/double/triple letter scores */
		for (c=scol; c<(scol+l); c++)
		{
			char ch = word[c-scol];
			int tl = ch - 'A';
			char spec = boardScores[r][c];
			if (ch>='a')
			{
				ch = BLANKVAL;
				tl = BLANK;
			}
			if (IsSpecial(r,c,mainmove) &&
				spec>='a' && spec<='z')
					scr += (spec-'a'+1) * Scores[tl];
			else scr += Scores[tl];
			if (isEmpty(r,c))
			{
				ltrW8 += Weights[tl];
				used++;
			}
		}
		/* calculate double/triple word scores */
		for (c=scol;c<(scol+l);c++)
		{
			if (IsSpecial(r,c,mainmove))
			{
				char spec = boardScores[r][c];
				if (spec>='A' && spec<='Z')
					scr *= (spec-'A'+1);
			}
		}
	}
	else
	{
		c = scol;
		for (r=srow; r<(srow+l); r++)
		{
			char ch = word[r-srow];
			int tl = ch - 'A';
			char spec = boardScores[r][c];
			if (ch>='a')
			{
				ch = BLANKVAL;
				tl = BLANK;
			}
			if (IsSpecial(r,c,mainmove) &&
				spec>='a' && spec<='z')
					scr += (spec-'a'+1) * Scores[tl];
			else scr += Scores[tl];
			if (isEmpty(r,c))
			{
				ltrW8 += Weights[tl];
				used++;
			}
		}
		for (r=srow; r<(srow+l); r++)
		{
			if (IsSpecial(r,c,mainmove))
			{
				char spec = boardScores[r][c];
				if (spec>='A' && spec<='Z')
					scr *= (spec-'A'+1);
			}
		}
	}
	/* Add all-tile bonus */
	if (numtiles>1 && used==numtiles)
		scr+=bonus;
	*score += (w8=scr); /* scr is this word; score the whole move */
	if (!useLetterWeights)
		ltrW8=0;
	if (adaptiveStrategy)
	{
		int diff = Score[player] - Score[1-player];
		if (diff>25)		/* current player is way ahead	*/
			w8 = l*2;	/* just play by length 		*/
		else if (diff>=0)	/* somewhat ahead		*/
			w8 = ltrW8 + l *lengthWeight; /* combine	*/
		else if (diff<-25)	/* way behind			*/
			w8 = scr;	/* maximise score		*/
		else 			/* somewhat behind		*/
			w8 = ltrW8 + scr + l*lengthWeight; /* hybrid	*/
	}
	/* if this is the primary word (and not a cross word)
		or if the IncludeCrossWeights option is set,
		add the other factors to the weight, but only if there
		are tiles left to draw (this admits more short moves
		at the end of the game, I think!). */
	else if (mainmove || weightWhat)
	{
		if ((player==COMPUTER || controlStrategy) && tilesleft)
		{
			w8 = scr + ltrW8 + l*lengthWeight;
		}
	}
	return w8;
}

/*
 *  Yep, its a bubblesort!
 */

static void sortHints(void)
{
	int i, j;
	for (i=0;i<(numHints-1);i++)
	{
		for (j=i+1;j<numHints;j++)
		{
			if ((player==HUMAN && (moveList[i].score>moveList[j].score)) ||
			    (player==COMPUTER && (moveList[i].weight>moveList[j].weight)))
			{
				MOVE m = moveList[i];
				moveList[i] = moveList[j];
				moveList[j] = m;
			}
		}
	}
}

/*
 * Check if a possible valid move is good enough to store
 * in the list of moves, and if so, store it
 */

static void CheckCandidateMove(char *word, char *mask, int row, int col,
			int score, int weight, int isAcross)
{
	int w, i, worst, min=0x7FFF;
	/* Check if we already have this word. If so, then
	   we will displace it only if the new move is better. */
	for (i=0; i<numHints; i++)
	{
		w = moveList[i].weight;
		if (w < min)
		{
			lowestHintWeight = min = w;
			worst = i;
		}
		if (numHints >= MAX_HINTS && strcmp(moveList[i].word,word)==0)
		{
			if (player==COMPUTER && moveList[i].weight >= weight)
				return;
			if (player==HUMAN && moveList[i].score >= score)
				return;
		}
	}
	/* Got empty space still so accept it */
	if (numHints < MAX_HINTS)
		i = numHints++;
	/* No empty space; see if its better than the worts we have */
	else if (weight <= min)
		return; /* nope, forget about it */
	else
		i = worst; /* yep; displace the weaker one */
	strcpy(moveList[i].word,word);
	if (mask)
		strcpy(moveList[i].mask, mask);
	else
		moveList[i].mask[0]=0;
	moveList[i].r = (char)row;
	moveList[i].c = (char)col;
	moveList[i].isAcross = (char)isAcross;
	moveList[i].score = score;
	moveList[i].weight = weight;
}

/* Compute a penalty for giving away a double/triple/etc word square */
/* Who knows if this really works! */

static int GiveawayPenalty(int srow, int scol, int l, char *word)
{
	/* For each piece placed, move both up and down up to six squares:
		- if square is nonempty, stop
		- else if square is a double/triple word, and
			the square beyond is empty, subtract
			(square weight) * (7 - distance)
	*/
	int r, c, n, rtn=0;
	if (isAcross)
	{
		for (c = scol-1;c < (scol+l+1); c++)
		{
			if (c<1 || c>Cols || !isEmpty(srow,c))
				continue; /* not playing anything here */
			if (c<scol || c>=(scol+l) || (srow==Rows || isEmpty(srow+1,c)))
				/* if a down word can end on the played letter, see if there
					are newly exposed double/trips.. above */
			{
				for (r = srow-1, n=1; n<6 && r>0 ; r--, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (r==1 || isEmpty(r-1,c)))
						rtn += (boardScores[r][c]-'A'+1) * (6-n);
				}
			}
			if (c<scol || c>=(scol+l) || (srow==1 || isEmpty(srow-1,c)))
			{
				for (r = srow+1, n=1; n<6 && r<=Rows ; r++, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (r==Rows || isEmpty(r+1,c)))
						rtn += (boardScores[r][c]-'A'+1) * (6-n);
				}
			}
		}
		/* Do the end chars */
		if (isEmpty(srow,scol))
		{
			for (r = srow-1; r <= (srow+1) ; r++)
			{
				if (r<1 || r>Rows) continue;
				if (r != srow)
				{
					if (!isEmpty(r,scol) && !isEmpty(r,scol+1))
						continue;
				}
				for (c = scol-1, n=1; n<3 && c>0 ; c--, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (c==1 || isEmpty(r,c-1)))
						rtn += (boardScores[r][c]-'A'+1) * (3-n);
				}
			}
		}
		if (isEmpty(srow,scol+l-1))
		{
			for (r = srow-1; r <= (srow+1) ; r++)
			{
				if (r<1 || r>Rows) continue;
				if (r != srow)
				{
					if (!isEmpty(r,scol+l-1) && !isEmpty(r,scol+l-2))
						continue;
				}
				for (c = scol+l, n=1; n<3 && c<=Cols ; c++, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (c==Cols || isEmpty(r,c+1)))
						rtn += (boardScores[r][c]-'A'+1) * (3-n);
				}
			}
		}
	}
	else
	{
		for (r = srow-1;r < (srow+l+1); r++)
		{
			if (r<1 || r>Rows || !isEmpty(r,scol))
				continue; /* not playing anything here */
			if (r<srow || r>=(srow+l) || (scol==Cols || isEmpty(r,scol+1)))
			{
				for (c = scol-1, n=1; n<6 && c>0 ; c--, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (c==1 || isEmpty(r,c-1)))
						rtn += (boardScores[r][c]-'A'+1) * (6-n);
				}
			}
			if (r<srow || r>=(srow+l) || (scol==1 || isEmpty(r,scol-1)))
			{
				for (c = scol+1, n=1; n<6 && c<=Cols ; c++, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (c==Cols || isEmpty(r,c+1)))
						rtn += (boardScores[r][c]-'A'+1) * (6-n);
				}
			}
		}
		/* Do the end chars */
		if (isEmpty(srow,scol))
		{
			for (c = scol-1; c <= (scol+1) ; c++)
			{
				if (c<1 || r>Cols) continue;
				if (c != scol)
				{
					if (!isEmpty(srow,c) && !isEmpty(srow+1,c))
						continue;
				}
				for (r = srow-1, n=1; n<3 && r>0 ; r--, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (r==1 || isEmpty(r-1,c)))
						rtn += (boardScores[r][c]-'A'+1) * (3-n);
				}
			}
		}
		if (isEmpty(srow+l-1,scol))
		{
			for (c = scol-1; c <= (scol+1) ; c++)
			{
				if (c<1 || c>Cols) continue;
				if (c != scol)
				{
					if (!isEmpty(srow+l-1,c) && !isEmpty(srow+l-2,c))
						continue;
				}
				for (r = srow+l, n=1; n<3 && r<=Rows ; r++, n++)
				{
					if (!isEmpty(r,c)) break;
					else if ((boardScores[r][c]>'A' && boardScores[r][c]<='Z')
							&& (r==Rows || isEmpty(r+1,c)))
						rtn += (boardScores[r][c]-'A'+1) * (3-n);
				}
			}
		}
	}
#if 0
	// temp DEBUG
	if (rtn)
	{
		showPossibleMove(srow,scol,word);
		showPenalty(rtn);
	}
#else
	(void)word; /* avoid warning */
#endif
	return (rtn / noGiveaway);
}

/* Evaluate move weight and score. Assumes cross-checking
   has been done
*/

int evalMove(char *word, char *mask, int erow, int ecol, int *sc)
{
	int c, r, srow, scol, l = (int)strlen(word), score=0, weight, ncw=0;

	if (isAcross)
	{
		scol = ecol-l+1;
		srow = erow;
	}
	else
	{
		srow = erow-l+1;
		scol = ecol;
	}

	/* Evaluate the main move */

	weight = ComputeWeightAndScore(word,srow,scol,isAcross, &score,1);
#if 0 /* debug code */
	if (mask == NULL)
	{
	    fprintf(stderr, "Computing score for %s %s at %d, %d\n",
		word, (isAcross?"across":"down"), srow, scol);
	    fprintf(stderr, "Base score is %d\n", score);
	}
#endif

	/* Now take into account new cross words */

	if (isAcross)
	{
		int sr;
		for (c=scol; c<(scol+l);c++)
		{
			if (isEmpty(srow,c))
			{
				if (GotAbove(srow,c) || GotBelow(srow,c))
				{
					char Word[MAXBOARD+1];
					int i;
					r = srow;
					while (GotAbove(r,c)) r--;
/*					if (r!=srow) r++; *//* start of word */
					sr = r;
					i = 0;
					while (r<=Rows && (r<=srow || HasTile(r,c)))
					{
						Word[i++] = letterAt(r,c);
						r++;
					}
					Word[i]=0;
					Word[srow-sr]=word[c-scol];
#if 0 /* debug code */
{
	int os = score;
     	weight += ComputeWeightAndScore(Word,sr,c,0, &score,0);
	if (mask == NULL)
	    fprintf(stderr, "Subword down %s at %d, %d: score %d\n",
		Word, sr, c, score - os);
}
#else
					weight += ComputeWeightAndScore(Word,sr,c,0, &score,0);
#endif
					ncw++;
				}
			}
		}
	}
	else
	{
		int c, r, sc;
		for (r=srow; r<(srow+l);r++)
		{
			if (isEmpty(r,scol))
			{
				if (GotLeft(r,scol) || GotRight(r,scol))
				{
					char Word[MAXBOARD+1];
					int i;
					c = scol;
					while (GotLeft(r,c)) c--;
/*					if (c!=scol) c++;*/ /* start of word */
					sc = c;
					i = 0;
					while (c<=Cols && (c<=scol || HasTile(r,c)))
					{
						Word[i++] = letterAt(r,c);
						c++;
					}
					Word[i]=0;
					Word[scol-sc]=word[r-srow];
#if 0 /* debug code */
{
	int os = score;
	weight += ComputeWeightAndScore(Word,r,sc,1, &score,0);
	if (mask == NULL)
	    fprintf(stderr, "Subword across %s at %d, %d: score %d\n",
		Word, r, sc, score - os);
}
#else
					weight += ComputeWeightAndScore(Word,r,sc,1, &score,0);
#endif
					ncw++;
				}
			}
		}
	}
	if (mask==NULL)
	{
		/* Just evaluating score */
		if (sc) *sc = score;
		return bestmove.score = score;
	}
	if (tilesleft==0  && numtiles)
		weight = score; // just get rid of them without all the subtleties
	else if (noGiveaway && (player==COMPUTER || controlStrategy))
		weight -= GiveawayPenalty(srow,scol,l,word);

	if (weight >= bestmove.weight || weight>=lowestHintWeight)
	{
		if ((player==COMPUTER || controlStrategy) && (tilesleft || numtiles==0))
		{
			int i = 0;
			if (maxScoreLimit && score>maxScoreLimit)
				goto done;
			if (minScoreLimit && score<minScoreLimit)
				goto done;
			if (l<minLength || l>maxLength)
				goto done;
			if (MaxNewXWord>0 && ncw>MaxNewXWord)
				goto done;
			while (word[i])
			{
				if (word[i]>'Z')
					if (weight<blankThreshold)
						goto done;
				i++;
			}
		}
		if (weight>=lowestHintWeight) /* good enuf for hints */
		{
			CheckCandidateMove(word,mask,srow,scol,score,weight,isAcross);
			if (weight < bestmove.weight) goto done;
		}
		/* new best move */
#if DEBUG
		fprintf(debug,"NEW MOVE FOUND [%s], score %d, weight %d\n",
			word, score, weight);
#endif
		if (weight > bestmove.weight)
		{
			bestmove.score = score;
			bestmove.weight = weight;
			movesWithThisScore=1;
		}
		else
		{
			movesWithThisScore++;
			if (random(movesWithThisScore)!=0)
				goto done;
		}
		strcpy(bestmove.word,word);
		if (mask) strcpy(bestmove.mask,mask);
		bestmove.r = (char)srow;
		bestmove.c = (char)scol;
		bestmove.isAcross = (char)isAcross;
		if (strategyTest != NOTEST)
			showPossibleMove(srow,scol,word);
	}
	if (sc) *sc = score;
done:
	return weight;
}

static void extendRight(char *word, char *mask, nodenum n, int row, int col)
{
	if (n==(nodenum)0 || (word-myPlay)>maxLength)
		return;
	if (isEmpty(row,col))
	{
		for (;;)
		{
			NODE N = Nodes(n);
			char c = NodeChar(N), usingblank = 0;
			if ( (GetTileCount(player,c) || GetTileCount(player,BLANKVAL))
				&& (CrossCheck[isAcross?col:row]&(1l<<(c-'A'))) )
			{
#if DEBUG>3
				fprintf(debug,"Player has %c for row %d, col %d, and in crosscheck at %s %d\n",
					c,row,col, isAcross?"col":"row",isAcross?col:row);
#endif
				if (GetTileCount(player,c))
				{
					useLetter(player,c);
					*word = *mask = c;
				}
				else
				{
					useLetter(player,BLANKVAL);
					*word = *mask = c-'A'+'a';
					usingblank = 1;
				}
				if (IsWordEnd(N))
				{
					int finish = 0;
					if (isAcross) finish = !GotRight(row,col);
					else finish = !GotBelow(row,col);
					if (finish)
					{
						int w8;
						word[1] = mask[1] = 0;
						if (!excluded(myPlay)) {
#if DEBUG>1
							fprintf(debug,"MOVE %s, anchor (%d,%d) end (%d,%d)\n",
								myPlay, Arow, Acol, row, col);
#endif
							w8 = evalMove(myPlay,myMask,row,col,NULL);
							if (searchCnt && player==COMPUTER && tilesleft)
							{
								if (w8 >= searchMin) thisCount++;
								if (thisCount >=searchCnt)
								{
									stopSearchNow = 1;
									goto skip_extend;
								}
							}
						}
					}
				}
#if DEBUG>3
				fprintf(debug,"Trying to extend right with %c\n",c);
#endif
				extendRight(word+1,mask+1,NextNode(N),isAcross?row:(row+1),isAcross?(col+1):col);
skip_extend:
				if (usingblank) replaceLetter(player,BLANKVAL);
				else replaceLetter(player,c);
#if DEBUG>3
				fprintf(debug, "Replacing letter %c\n",usingblank?BLANKCH:c);
#endif
			}
			if (IsLastChild(N)) break;
			n++;
		}
	}
	else
	{
		char l = (char)boardLetters[row][col], ll;
		ll = l&0x1F;
		for (;;)
		{
			NODE N = Nodes(n);
			if ((char)NodeIndex(N) == (ll+1))
			{
				if (l&0x80) *word = ll+'a'; /* blank */
				else *word = ll+'A';
				*mask = '-';
				extendRight(word+1,mask+1,NextNode(N),isAcross?row:(row+1),isAcross?(col+1):col);
			}
			if (IsLastChild(N)) break;
			n++;
		}
	}
}

static void leftPart(char *word, char *mask, nodenum n, int limit)
{
	if (n==(nodenum)0)
		return;
	extendRight(word, mask, n, Arow, Acol);
	if (limit>0 && !stopSearchNow)
	{
		for (;;)
		{
			NODE N = Nodes(n);
			char c = NodeChar(N), usingblank=0;
			if ( (CrossCheck[(isAcross?Acol:Arow)-limit] & (1l<<(c-'A'))) != 0l)
			{
				if (GetTileCount(player,c))
				{
					useLetter(player,c);
					*word = *mask = c;
					leftPart(word+1,mask+1,NextNode(N),limit-1);
					replaceLetter(player, c);
				}
				else if (GetTileCount(player,BLANKVAL))
				{
					useLetter(player,BLANKVAL);
					*word = *mask = c-'A'+'a';
					leftPart(word+1,mask+1,NextNode(N),limit-1);
					replaceLetter(player, BLANKVAL);
				}
			}
			if (IsLastChild(N)) break;
			else n++;
		}
	}
}

void findMove(int first)
{
	int row, col;
	float prcnt=0., pinc=100./((float)(Rows+Cols));
	lowestHintWeight = numHints = 0;
	bestmove.score = stopSearchNow = thisCount = 0;
	bestmove.weight = -0x7F00;
	putStringAt(14, "Examining...");
	for (row=1;row<=Rows;row++)
		ComputeAcrossCheck(row);
	for (col=1;col<=Cols;col++)
		ComputeDownCheck(col);

	/* work out across moves */
	isAcross = 1;
	if (first && firstMoveDir)
		goto doDown;
	for (row=1;row<=Rows && !stopSearchNow;row++)
	{
#if DEBUG
		fprintf(debug,"========================================\nChecking row %d\n",row);
#endif
		showPercent((double)(prcnt+=pinc));
		Arow = row;
		CrossCheck = XChkAcross[row];
		for (col=1;col<=Cols;col++)
			if (first)
			{
				if (firstMoveCol)
				{
					Anchor[col] = (col==firstMoveCol);
				}
				else Anchor[col] = 1;
				if (firstMoveRow && row!=firstMoveRow)
					Anchor[col] = 0;
			}
			else Anchor[col] = IsAnchor(row,col);
#if DEBUG
		showCrossCheck();
#endif
		/* We now go through all anchors. If the left part is already
		// on the board, we put it in the word and call extendRight;
		// else we call leftPart to generate all left parts. */
		for (col=1;col<=Cols && !stopSearchNow;col++)
		{
			if (Anchor[col])
			{
				int ctmp = col;
				char *word = myPlay, *mask = myMask;
				Acol = col;
#if DEBUG>1
				fprintf(debug,"Checking for words anchored at (%d,%d)\n",row,col);
#endif
				if (ctmp==1)
					leftPart(word,mask,(nodenum)1,0);
				else
				{
					ctmp--;
					if (HasTile(row,ctmp))
					{
						/* We have an existing left part */
						nodenum n = 1;
						while (GotLeft(row,ctmp))
							ctmp--;
						while (ctmp<col)
						{
							NODE N = Nodes(n);
							char c = letterAt(row,ctmp);
							*word++ = c;
							*mask++ = '-';
							ctmp++;
							while (NodeChar(N) != c)
							{
								n++;
								N = Nodes(n);
							}
							n = NextNode(N);
						}
						*word = *mask = 0;
#if DEBUG>2
						fprintf(debug,"Existing left part %s\nRecursing leftPart node %ld",
							myPlay,(long)n);
#endif
						leftPart(word,mask, n,0);
					}
					else
					{
						/* We have an open left part */
						int limit = 0;
						while (ctmp>0 && isEmpty(row,ctmp))
						{
							if (Anchor[ctmp]) break;
							limit++;
							ctmp--;
						}
#if DEBUG>2
						fprintf(debug,"Empty left part length %d\nRecursing leftPart node 1",limit);
#endif	
						leftPart(word,mask,(nodenum)1,limit);
					}
				}
			}
		}
	}
doDown:
	/* work out down moves */
	if (first && firstMoveDir==2)
		goto done; // down move not allowed
	isAcross = 0;
	for (col=1;col<=Cols;col++)
	{
#if DEBUG
		fprintf(debug,"========================================\nChecking col %d\n",col);
#endif
		showPercent((double)(prcnt+=pinc));
		Acol = col;
		CrossCheck = XChkDown[col];
		for (row=1;row<=Rows;row++)
			if (first)
			{
				if (firstMoveRow)
					Anchor[row] = (row==firstMoveRow);
				else Anchor[row] = 1;
				if (firstMoveCol && col!=firstMoveCol)
					Anchor[row] = 0;
			}
			else Anchor[row] = IsAnchor(row,col);
#if DEBUG
		showCrossCheck();
#endif
		/* We now go through all anchors. If the left part is already
		// on the board, we put it in the word and call extendRight;
		// else we call leftPart to generate all left parts. */
		for (row=1;row<=Rows;row++)
		{
			if (Anchor[row])
			{
				int rtmp = row;
				char *word = myPlay, *mask = myMask;
				Arow = row;
#if DEBUG>1
				fprintf(debug,"Checking for words anchored at (%d,%d)\n",row,col);
#endif
				if (rtmp==1) leftPart(word,mask,(nodenum)1,0);
				else
				{
					rtmp--;
					if (HasTile(rtmp,col))
					{
						/* We have an existing left part */
						nodenum n = 1;
						while (GotAbove(rtmp,col)) rtmp--;
						while (rtmp<row)
						{
							NODE N = Nodes(n);
							char c = letterAt(rtmp,col);
							*word++ = c;
							*mask++ = '-';
							rtmp++;
							while (NodeChar(N) != c)
							{
								n++;
								N = Nodes(n);
							}
							n = NextNode(N);
						}
						*word = *mask = 0;
#if DEBUG>2
						fprintf(debug,"Existing left part %s\nRecursing leftPart node %d",myPlay,n);
#endif
						leftPart(word,mask, n,0);
					}
					else
					{
						/* We have an open left part */
						int limit = 0;
						while (rtmp>0 && isEmpty(rtmp,col))
						{
							if (Anchor[rtmp]) break;
							limit++;
							rtmp--;
						}
#if DEBUG>2
						fprintf(debug,"Empty left part length %d\nRecursing leftPart node 1",limit);
#endif
						leftPart(word,mask,(nodenum)1,limit);
					}
				}
			}
		}
	}
done: sortHints();
#if DEBUG
	fprintf(debug,"Done\n");
	fprintf(debug,"Best move is %s starting at (%d,%d) going %s with score %d\n",
		bestmove.word, bestmove.r, bestmove.c, bestmove.isAcross?"across":"down",bestmove.score);
#endif
}

int canSwap(int *row, int *col, char *tile)
{
	int r, c;
	char ch;
	if (exchangeAllowed==NOXCHANGE)
		return 0;
	for (r=1;r<=Rows;r++)
	{
		for (c=1;c<=Cols;c++)
		{
			if ((ch=(char)(boardLetters[r][c]))&0x80) // got blank?
			{
				if (Tiles[player][ch&0x3F])
				{
					*row = r;
					*col = c;
					*tile = ch&0x3F;
					return 1;
				}
			}
		}
	}
	return 0;
}

void doSwap(int r, int c, char ch, int first)
{
	char msg[40];
	takeFromRack(player,'A'+ch,1,1300);
	addToRack(player,'Z'+1,1,1300);
	boardLetters[r][c] &= 0x40;
	boardLetters[r][c] += (uchar)ch;
	sprintf(msg,"%s exchange %c %c",
		player==HUMAN ? (strategyTest?"Ctl":"You") : "I",
		r+'A'-1,c+'A'-1);
	showInfo(1,msg);
	DisplayBoard(SLOW);
	msg[0] = ch+'A';
	msg[1]=0;
	recordMove(player,'X',r,c,msg,0,moves,first);
}

static int checkLetter(int r, int c, char ch, ulong mask, int *Override)
{
	ulong msk;
	if (ch>'Z')
	{
		if (!GetTileCount(player,BLANKVAL))
			return NOBLANK;
		msk = 1l << (ch-'a');
	}
	else
	{
		if (!GetTileCount(player,ch))
			return NOLETTER;
		msk = 1l << (ch-'A');
	}
	if ((mask&msk)==0)
	{
	     	if (!*Override)
		{
			char buff[24];
		   	sprintf(buff,"Bad Xword at %c %c!", r+'A'-1, c+'A'-1);
	     		if (!override(buff))
				return BADCROSS;
			*Override =1;
		}
	}
	return 0;
}

int checkMove(char *word, int row, int col, int first, int *status)
{
	char wrd[20], ch;
	int r, c, l, OK, pos, rtn, re, ce, ri, ci, xChkOverride;
	bestmove.score = bestmove.weight = 0;
#if DEBUG
	fprintf(debug,"In checkMove(%s,%d,%d,%d,%d)\n",word,row,col,first,*status);
#endif
	if (first)
	{
		if (isAcross)
		{
			if (firstMoveRow && row!=firstMoveRow)
				return NOTINROW;
		}
		else
		{
			if (firstMoveCol && col!=firstMoveCol)
				return NOTINCOL;
		}
	}
	strcpy(wrd,word);
	(void)strupr(wrd);
	if (lookup(wrd)==0 || excluded(wrd)) 
		if (!override("Word not in dictionary!"))
			return NOTINDICT;
	l = (int)strlen(word);
	if (isAcross)
	{
		ComputeAcrossCheck(row);
		CrossCheck = XChkAcross[row];
		for (c=1;c<=Cols;c++)
			Anchor[c] = IsAnchor(row,c);
	}
	else
	{
		ComputeDownCheck(col);
		CrossCheck = XChkDown[col];
		for (r=1;r<=Rows;r++)
			Anchor[r] = IsAnchor(r,col);
	}
	/* to check the move, we ensure that it includes
		an anchor, as well as traversing the word,
		ensuring that every letter corresponds to
		either a piece on the board, or a piece in
		the tile rack. In the latter case, we remove it.
		We also do cross checks in the appropriate direction
		for each tile placed.
		If we find a problem, we must replace any tiles
		that we used. If the move is OK, we work out
		the score. */
	OK = 0;
	if (isAcross)
	{
		for (c=col;c<(col+l);c++)
			if (Anchor[c])
			{
				OK=1;
				break;
			}
	}
	else
	{
		for (r=row;r<(row+l);r++)
			if (Anchor[r])
			{
				OK=1;
				break;
			}
	}
	if (OK==0 && !first)
		return NOANCHOR;
	strcpy(wrd,"                  ");
	pos = rtn = xChkOverride = 0;
	if (isAcross)
	{
		ce = col+l;
		re = row+1;
		ci = 1;
		ri = 0;
	}
	else
	{
		ce = col+1;
		re = row+l;
		ci = 0;
		ri = 1;
	}
	for (c=col, r=row; c<ce && r<re; c+=ci, r+=ri, pos++)
	{
		ch = word[pos];
		if (isEmpty(r,c)) /* ensure we have letter and cross check */
		{
			rtn = checkLetter(r, c, ch, CrossCheck[isAcross?c:r], &xChkOverride);
			if (rtn)
				goto End;
			if (ch>'Z')
				useLetter(player,BLANKVAL);
			else
				useLetter(player,ch);
			wrd[pos] = ch;
		}
		else /* ensure word agrees with what is on the board */
		{
			char l = (char)boardLetters[r][c], ll;
			ll = (l&0x1F)+'A';
			if (ch>='a')
				if (l&0x80) ch -= 'a'-'A';
				else rtn = CONFLICT;
			if (ch!=ll)
				rtn = CONFLICT;
			if (rtn)
				goto End;
		}
	}
	/* check whether word starts/ends where user implies */
	if (isAcross)
	{
		if (c<Cols && HasTile(row,c))
			rtn = BADEND;				/* can't end there */
		else if (col>1 && HasTile(row,col-1))
			rtn = BADSTART;	/* can't start there */
	}
	else
	{
		if (r<Rows && HasTile(r,col))
			rtn = BADEND;		/* can't end there */
		else if (row>1 && HasTile(row-1, col))
			rtn = BADSTART;		/* can't start there */
	}

End:;
	if (rtn)
	{
		if (rtn==NOLETTER)
			*status = ch;
		// replace any tiles used
		while (pos>=0)
		{
			if (wrd[pos]>'Z')
				replaceLetter(player,BLANKVAL);
			else if (wrd[pos]!=' ')
				replaceLetter(player,wrd[pos]);
			pos--;
		}
	}
	else
	{
		char mask[MAXBOARD+1];
		int sc;
		(void)evalMove(word, NULL, row+(isAcross?0:(l-1)), col+(isAcross?(l-1):0),&sc);
		for (pos = 0;pos<l;pos++)
		{
			int c = col+(isAcross?pos:0),
			    r = row+(isAcross?0:pos);
			if (isEmpty(r, c))
			{
				if (word[pos]>'Z')
					boardLetters[r][c] = (uchar)(0x80|(word[pos]-'a'));
				else
					boardLetters[r][c] = (uchar)(word[pos]-'A');
				mask[pos] = word[pos];
			}
			else mask[pos]='-';
		}
		mask[pos]=0;
		Score[player] += sc;
		recordMove(player,isAcross?'A':'D',row,col,mask,sc,moves,first);
	}
	return rtn;
}


