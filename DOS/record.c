
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLOBAL extern

#include "dict.h"
#include "ww.h"

static void freeMoveRecord(MOVEREC mp)
{
	MOVEREC nxt;
	while (mp)
	{
		nxt = mp->next;
		free(mp);
		mp = nxt;
	}
}

void freeMoveRecords(void)
{
	int i;
	for (i=0;i<2;i++)
	{
		freeMoveRecord(moveRecHead[i]);
		moveRecHead[i] = moveRecTail[i] = moveRecNow[i] = NULL;
	}
}

void resetData(int clearAll)
{
	int i, j;
	moveRecNow[0] = moveRecHead[0];
	moveRecNow[1] = moveRecHead[1];
	computedMoves = tilesleft = 0;
	for (i=0;i<27;i++)
	{
		PoolCnt[i] = 0;
		addToHeap('A'+i,PoolStart[i],0);
	}
	Score[HUMAN] = Score[COMPUTER] = 0;
	if (clearAll)
	{
		for (i=1;i<=Rows;i++)
			for (j=1;j<=Cols;j++)
			{
				boardLetters[i][j] = initBoardLetters[i][j];
				screenLetters[i][j] = (uchar)-1;
			}
		for (i=0;i<27;i++) Tiles[2][i] = Tiles[3][i] = 0;
	}
	for (i=0;i<27;i++) Tiles[0][i] = Tiles[1][i] = 0;
	player = firstPlayer;
}

MOVEREC nextMove(int movenum, int player, char action)
{
	MOVEREC mr= calloc(1,sizeof(struct _MOVEREC));
	if (mr==NULL)
	{
		showInfo(1,"Can't record!");
		return NULL; /* out of memory */
	}
	if (moveRecNow[player])
	{
		moveRecNow[player]->next = mr;
		mr->prev = moveRecNow[player];
	} else
	{ // new list
		moveRecHead[player] = mr;
		mr->prev = NULL;
	}
	mr->next = NULL;
	mr->movenum = movenum;
	mr->action = action;
	return moveRecNow[player] = moveRecTail[player] = mr;
}

void recordMove(int player, char action, int row, int col,
		char *mask, int score, int movenum, int first)
{
	MOVEREC mr = nextMove(movenum,player,action);
	if (mr)
	{
		mr->row = (char)('A'+row-1);
		mr->col = (char)('A'+col-1);
		strcpy(mr->mask,mask);
		mr->score = score;
		mr->first = (char)first;
#if DEBUG
		fprintf(debug, "Record move %c %d %d [%s] for player %d\n",
			action, row, col, mask, player);
#endif
	}
}

void recordDraw(int player, char *tiles)
{
	MOVEREC mr = moveRecNow[player];
#if DEBUG
	fprintf(debug, "Record draw [%s] for player %d\n", tiles, player);
	showTiles(player);
#endif
	if (mr) strcpy(mr->draw,tiles);
}

void recordDrop(int player, char *tiles, int movenum, int first)
{
	MOVEREC mr = nextMove(movenum,player,'P');
#if DEBUG
	fprintf(debug, "Record drop [%s] for player %d\n", tiles, player);
#endif
	if (mr)
	{
		strcpy(mr->mask,tiles);
		mr->first = (char)first;
	}
}

static MOVEREC undo(int player, MOVEREC mr)
{
	/* We must do the following:
		1. Replace all drawn tiles in the pool
		2. Replace tiles from mask to the rack.
		IN ADDITION, IF A DROP MOVE:
		3. Remove tiles from mask from pool
		IN ADDITION, IF NOT A DROP MOVE:
		3. Remove tiles from mask from board
		4. Subtract score

	  On top of all this, if the previous move record in
	  the list has the same move number, we do it too, (etc)
	*/
	char *p, r, c, rd, cd, msg[34];;
start:
	r = mr->row-'A'+1;
	c = mr->col-'A'+1;
	if (mr->action=='X')
	{
		takeFromRack(player,'Z'+1,1,1001);
		addToRack(player,mr->mask[0],1,1001);
		boardLetters[r][c] = 0x80 | (mr->mask[0]-'A');
		sprintf(msg,"Undoing exchange %s",mr->mask);
		showInfo(1,msg);
		goto check;
	}
	/* replace the drawn tiles */
	p = mr->draw;
	if (numtiles>1)
		while (*p)
		{
			char t = (*p=='_') ? ('Z'+1) : (*p);
			addToHeap(t,1,800);
			takeFromRack(player,t,1,800);
			p++;
		}
//	if (mr->action=='X') goto check;
	p = mr->mask;
	if (mr->action=='A') { cd=1; rd=0; }
	else { cd=0; rd=1; }
	if (mr->action!='P')
	{
		sprintf(msg,"Undoing move %s",mr->mask);
		showInfo(1,msg);
	} else if (mr->draw)
	{
		int l = (int)strlen(mr->draw);
		if (l) sprintf(msg,"Undoing draw of %d tiles",l);
		showInfo(1,msg);
	}
	while (*p)
	{
		if (*p!='-')
		{
			char t = (*p=='_' || *p>'Z') ? ('Z'+1) : (*p);
			if (numtiles>1) addToRack(player,t,1,700);
			if (mr->action!='P') boardLetters[r][c] = EMPTY;
			else takeFromHeap(t,1,700);
		}
		r += rd;
		c += cd;
		p++;
	}
	if (mr->action != 'P') Score[player] -= mr->score;
check:
	if (mr->prev && mr->prev->movenum==mr->movenum)
	{	// got another?
		mr = mr->prev;
		DisplayBoard(SLOW);
		goto start;
	}
	moves--;
	return mr->prev;
}

int undo1Move(int trim, int *first)
{
	// trim is 1 if we are just undoing the tail
	// exchanges, in which case they are removed
	// from the move record.
	if (moves<=1) return -1;
	player = 1-player;
	if ((moveRecNow[player] = undo(player,moveRecNow[player]))==NULL)
	{
		moveRecNow[player]=moveRecHead[player];
		moveRecNow[player]->movenum *=-1;
	}
	if (trim)
	{
		while (moveRecTail[player]!=moveRecNow[player])
		{
			moveRecTail[player] = moveRecTail[player]->prev;
			free(moveRecTail[player]->next);
			moveRecTail[player]->next = NULL;
		}
		moves++; // not actually a whole move back
	}
	DisplayUndo();
	clearUndoJunk();
	DisplayBoard(SLOW);
	*first = (moves==0 || moveRecNow[player]->first);
	return 0;
}

int undo2Moves(int *first)
{
	if (moves<=1) return -1;
	if (moves==2)
	{
		if (override2("Redo info will be lost!"))
		{
			(void)undo1Move(0,first);
			freeMoveRecords();
		}
		else return -1;
	} else
	{
		if (undo1Move(0,first)==0)
		{ 
			if (undo1Move(0,first)!=0)
				return -1;
		}
		else return -1;
	}
	// Both of the next two get updated again back in main
	player = 1-player;
	moves--;
	return 0;
}

static MOVEREC replayMove(MOVEREC mp, int *first) {
	int i, j, rd, cd;
	char *w, msg[34];
	if (mp == NULL) return NULL;
	/* Process move */
nextOne:
	if (mp->action=='P')
	{
		sprintf(msg,"%s pass",player==COMPUTER?"My":"Your");
		if (mp->mask)
		{
			int l = (int)strlen(mp->mask);
			if (l) sprintf(msg,"%s draw %d",
				player==COMPUTER?"My":"Your", l);
		}
	}
	else sprintf(msg,"%s move %c %s",player==COMPUTER?"My":"Your",
		mp->action,mp->mask);
#if DEBUG
	fprintf(debug, "%s\n", msg);
#endif
	showInfo(0,msg);
	w = mp->mask;
	if (mp->action=='X')
	{
		int c = *w - 'A';
		boardLetters[mp->row-'A'+1][mp->col-'A'+1] = ((uchar)c) | ((player==HUMAN) ? 0x40 : 0);
		takeFromRack(player,*w,1,1111);
		addToRack(player,'Z'+1,1,1111);
		if (mp->next && mp->movenum==mp->next->movenum)
		{
			DisplayBoard(SLOW);
			mp = mp->next;
			goto nextOne;
		}
	}
	else if (mp->action!='P')
	{
	 	if (mp->action=='A') { cd=1; rd=0; }
	 	else { cd=0; rd=1; }
		if (first) *first = 0;
		i = mp->row-'A'+1;
		j = mp->col-'A'+1;
 		while (*w)
		{
 			if (*w!='-') putLetter(player, i,j,*w);
	  		w++;
	  		i += rd;
	  		j += cd;
	  	}
	}
	else if (numtiles>1)
	{
 		/* Discard tiles */
	  	while (*w)
		{
	  		char t = (*w >='a') ? ('Z'+1) : (*w);
	  		takeFromRack(player,t,1,500);
			addToHeap(t,1,500);
#if DEBUG
			fprintf(debug, "Discarding <%c>\n", t);
#endif
	  		w++;
	  	}
	}
	Score[player] += mp->score;
#if DEBUG
	fprintf(debug, "Move scores %d\n", mp->score);
#endif
	if (numtiles>1)
	{
	  	/* Draw tiles */
	  	w=mp->draw;
	  	while (*w)
		{
	  		char t = (*w =='_') ? ('Z'+1) : (*w);
			takeFromHeap(t,1,400);
	  		addToRack(player,t,1,400);
#if DEBUG
			fprintf(debug, "Drawing <%c>\n", t);
#endif
	  		w++;
	  	}
	}
#if DEBUG
	fprintf(debug, "At end: "); showTiles(player);
	getch();
#endif
	return mp;
}

int doRedo(int player)
{
	int rtn = 1;
	if (moveRecNow[player]==NULL) rtn = 0;
	else if (moveRecNow[player]->movenum<0)
	{
		moveRecNow[player]->movenum *=-1;
		moveRecNow[player] = replayMove(moveRecNow[player],NULL);
	}
	else if (moveRecNow[player]->next)
		moveRecNow[player] = replayMove(moveRecNow[player]->next,NULL);
	else rtn = 0;
	return 1;
}

static void replayGame(int *first)
{
	int i, j;
	*first = 1;
	resetData(0);
	if (numtiles>1)
	{
		for (i=0;i<2;i++)
		{
			for (j=0;j<27;j++)
			{
				addToRack(i, 'A'+j, Tiles[i+2][j], 200);
				takeFromHeap('A'+j,Tiles[i][j],200);
			}
		}
	}
	/* Clear the display by just repainting tiles as empty
		squares */
	DisplayUndo();
	clearUndoJunk();
	DisplayBoard(FAST);
	moveRecNow[0] = moveRecHead[0];
	moveRecNow[1] = moveRecHead[1];
	for (moves=1;;player=1-player)
	{
		if (moves<=2 && moveRecNow[player])
		{
			// playing player's first move
			moveRecNow[player] =
				replayMove(moveRecNow[player],first);
		}
		else if (moves>2 && moveRecNow[player]->next)
		{
			// playing player's next move
			moveRecNow[player] =
				replayMove(moveRecNow[player]->next,first);
		}
		else break;
		moves++;
		DisplayBoard(SLOW);
	}
	showInfo(1,"Finished replay");
	moves--; // will be incremented again later
}

static int loadGame(FILE *cfg, int gotHeader, int *first)
{
	// returns: -1 (fail), 0 OK
	int i, row, col, c, p, action, move=1, score;
	freeMoveRecords();
	*first = 1;
	resetData(1);
	linenum = 1;
	if (gotHeader)
	{
		if (getChar(cfg,"@ marker")!='@') i = 0 /* should be error */ ;
		i = getNumber(cfg,"load game flag",0,0,7);
		if (i==0) return -1;
		if (i&4) *first = loadBoard(cfg);
	}
	if (numtiles>1)
	{
		for (p=0; p<2;p++)
		{
			for (i=0;i<27;i++) Tiles[p+2][i] = 0;
			for (i=0;i<numtiles;i++)
			{
				c = getChar(cfg,"rack tiles");
				if (c!='_' && (c<'A' || c>'Z'))
				{
					fprintf(stderr,"Line %d - character expected, not ASCII %d!\n",linenum,c);
					fprintf(stderr,"Error happened while scanning %s rack tiles\n",
						p?"your":"my");
					abort();
				}
				if (c=='_') c = BLANK;
				else c -= 'A';
				Tiles[p+2][c]++;
			}
		}
	}
	/* now load moves */
	showInfo(0,"Loading moves");
	firstPlayer = -1;
	moves = 1;
	while ((move=getNumber(cfg,"Move number",0,0,999))>0 && !feof(cfg))
	{
		player=getNumber(cfg,"Player",0,0,1);
		if (firstPlayer == -1) firstPlayer = player;
		action=getChar(cfg,"Move action");
		if (action=='A' || action=='D' || action=='X')
		{
			row = getChar(cfg,"starting row of move");
			col = getChar(cfg,"starting column of move");
		} /* else if (action!='P') error! */ ;
		i = 0;
		while ((c = getChar(cfg,"Played tiles"))!='+')
			myPlay[i++] = (char) c;
		myPlay[i] = 0;
		i = 0;
		while ((c = getChar(cfg,"Drawn tiles"))!='*') myMask[i++] = (char)c;
		myMask[i] = 0;
		score = getNumber(cfg,"Move score",1,0,0);
		if (action=='P') recordDrop(player,myPlay,move, *first);
		else
		{
			recordMove(player, (char)action, row-'A'+1, col-'A'+1, myPlay,score,move,*first);
			if (action!='X') *first = 0;
		}
		recordDraw(player,myMask);
	}
	if (firstPlayer!=-1)
	{
		showInfo(0,"Starting replay");
		replayGame(first);
	}
	return 0;
}

int loadGameFile(char *name, int gotHeader, int *first)
{
	FILE *fp = fopen(name,"rt");
	int rtn;
	if (fp==NULL)
	{
		showInfo(1,"No such game file!");
		return -1;
	}
	else
	{
		if (loadGame(fp,gotHeader,first)) return -1;
		fclose(fp);
	}
	return 0;
}
	
void saveGame(char *fname)
{
	int i, j, player, dumpboard = 0;
	MOVEREC mp, mptr[2];
	FILE *fp = fopen(fname,"wt");
	if (fp==NULL) return;
	for (i=1;i<=Rows;i++)
	{
		for (j=1;j<=Cols;j++)
		{
			char ch = initBoardLetters[i][j];
			if ((ch & 0x1F) < 26)
			{
				dumpboard = 1;
				break;
			}
		}
	}
	if (!dumpboard)
		fprintf(fp,"# No letters already on start board\n\n@1\n\n");
	else
	{
		fprintf(fp,"# Some letters already on start board\n\n@5\n\n");
		for (i=1;i<=Rows;i++)
		{
			for (j=1;j<=Cols;j++)
			{
				char ch = initBoardLetters[i][j];
				if (ch == BLACKSQ || ch == EMPTY)
					fputc('.', fp);
				else if (ch & 0x80)
					fputc('a' + (ch&0x1F), fp);
				else
					fputc('A' + (ch&0x1F), fp);
			}
			fputc('\n', fp);
		}
		fputc('\n', fp);
	}
	/* start racks */
	fprintf(fp,"# computer's starting rack\n\n");
	for (i=0;i<27;i++)
	{
		int cnt = Tiles[COMPUTER+2][i];
		while (cnt--) fputc(i==BLANK?'_':(i+'A'),fp);
	}
	fprintf(fp,"\n\n# human's starting rack\n\n");
	for (i=0;i<27;i++)
	{
		int cnt = Tiles[HUMAN+2][i];
		while (cnt--) fputc(i==BLANK?'_':(i+'A'),fp);
	}
	/* moves:
		action (A,D,P)
		row
		col (alphabetic)
		mask
		draw
		score
	*/
	mptr[0] = moveRecHead[0];
	mptr[1] = moveRecHead[1];
	player = firstPlayer;
	fprintf(fp,"\n\n# Move record\n\n");
	for (;;)
	{
		mp = mptr[player];
		if (mp==NULL) break;
		if (mp->movenum<0) break; // undone first move
		if (mp->action=='P') fprintf(fp,"%d %d P %s +%s *0\n",
			mp->movenum,player, mp->mask, mp->draw);
		else fprintf(fp,"%d %d %c %c %c %s +%s *%d\n",
			mp->movenum,player, mp->action,mp->row,
			mp->col,mp->mask,mp->draw,mp->score);
		if ((mptr[player] = mp->next)!=NULL)
			if (mp->movenum==mp->next->movenum)
				continue;
		player = 1-player;
	}
	/* terminating 0 */
	fprintf(fp,"\n\n# terminating marker\n\n0\n\n");
	fclose(fp);
}
