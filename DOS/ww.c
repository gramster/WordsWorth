#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <conio.h>
#include <dos.h>
#include <time.h>
#include <ctype.h>

#include "version.h"
#include "dict.h"

#define GLOBAL

#include "ww.h"

static int loadedDawg=0;
static char trash[MAXBOARD+1];
static char *cfgname, *gamname;
static int gameCount;
static int edit;

#ifdef DEBUG
extern FILE *debug;
#endif

/* evalMove should be static but is needed for debugging at present */

extern int evalMove(char *word, char *mask, int erow, int ecol, int *sc);

static void myAssert(char ch, int id)
{
	saveGame("core");
 	ioAbort();
	if (ch) fprintf(stderr,"Fatal error! Assertion violation; do not have <%c>\n",
			(ch==BLANKVAL)?'_':ch);
	else if (id) fprintf(stderr,"Fatal error! Bad rack/heap operation, id %d!\n",id);
	else fprintf(stderr,"Fatal error! Failed to draw a tile!\n");
	fprintf(stderr,"Game in progress dumped to file `core'\n");
	fprintf(stderr,"Please report this error, with the `core' file, to the author.\n");
	exit(0);
}

#if 0
#define check(p, t, cnt, id) \
		if (p<0 || p>1 || tilesleft<0 || cnt<0 || t<'A' || t>('Z'+1))\
					myAssert(0,id)
#else
#define check(p, t, cnt, id) 		(void)id
#endif

void takeFromRack(int p, char t, int cnt, int id)
{
	check(p,t,cnt,id);
	Tiles[p][t-'A'] -= cnt;
}

void addToRack(int p, char t, int cnt, int id)
{
	check(p,t,cnt,id);
	Tiles[p][t-'A'] += cnt;
}

void takeFromHeap(char letter, int cnt, int id)
{
	check(0,letter,cnt,id);
	PoolCnt[letter-'A']-=cnt;
	tilesleft-=cnt;
}

void addToHeap(char letter, int cnt, int id)
{
	check(0,letter,cnt,id);
	PoolCnt[letter-'A']+=cnt;
	tilesleft+=cnt;
}

/******************/
/* INITIALISATION */
/******************/

static int getTile(void)
{
	int i, r = random(tilesleft);
	for (i=0;i<27;i++)
	{
		if (PoolCnt[i]>r)
		{
			takeFromHeap('A'+i,1,350);
			return i;
		}
		else r -= PoolCnt[i];
	}
	myAssert((char)0,0);
	return 0;
}

/*
 * DrawTiles - draw tiles for player
 *
 * this routine takes a `player' argument. It determines how
 * many tiles need to be drawn, and calls getTile to draw them.
 * The draw is recorded.
 */

static int DrawTiles(int player) /*  return number of tiles in rack */
{
	int i, have = 0, draw;
	char tiles[MAXBOARD+1] = { '\0' };
	if (numtiles<2)
		return tilesleft; /* we play the pool or unlimited */
	/* See how many we must draw */
	for (i=0;i<27;i++)
		have += Tiles[player][i];
	draw = numtiles - have;
	/* Draw some new tiles */
	i=0;
	while (--draw>=0 && tilesleft>0)
	{
		int t = getTile();
		addToRack(player,'A'+t,1,450);
		tiles[i++] = (t==BLANK) ? '_' : (char)(t+'A');
	}
	tiles[i] = 0;
	recordDraw(player,tiles);
	return (numtiles-draw-1); /* tiles left */
}   

void shutdown(void)
{
	int q = endGameMessage();
	freeMoveRecords();
	firstPlayer = random(2);
	if (q)
	{
		/* turn off caps lock */
		*((uchar far *)0x417) &= 0xBF;
#ifdef DEBUG
		fclose(debug);
#endif
		exit(0);
	}
}

static void Setup(char *cfgname)
{
	int i;
	extern char Register[];
	(void)setcbrk(0);
	(void)signal(SIGABRT,SIG_IGN);
	(void)signal(SIGTERM,SIG_IGN);
	freeMoveRecords();
	loadCFG(cfgname);
	resetData(1); /* clear score, board, etc */
	/* Draw and save the initial tiles */
	(void)DrawTiles(HUMAN);
	(void)DrawTiles(COMPUTER);
	for (i=0;i<27;i++)
	{
		Tiles[HUMAN+2][i] = Tiles[HUMAN][i];
		Tiles[COMPUTER+2][i] = Tiles[COMPUTER][i];
	}
	showCopyright();
	if (scrInit()!=0) exit(0);
	scrDraw();
	DisplayBoard(FAST);
	if (!loadedDawg)
	{
		showInfo(0,"Loading words...");
		loadedDawg = 1;
		if (loadDawg((char *)Register)==0) dawgLoadFail();
	}
	// turn on caps lock
	*((uchar far *)0x417) |= (uchar)0x40;
	moves = 0;
	stalemate = 0;
}

/****************/
/* MAIN PROGRAM */
/****************/

#ifdef DEBUG
void showCrossCheck(void)
{
	int i, lim;
	if (isAcross) lim = Cols;
	else lim = Rows;
	for (i=1;i<=lim;i++)
	{
		if (CrossCheck[i] != 0x3FFFFFFl)
		{
			char c = 'A';
			fprintf(debug,"%s %d crosscheck {",isAcross?"col":"row",i);
			while (c<='Z')
			{
				if (CrossCheck[i]&(1l<<(c-'A'))) fputc(c,debug);
				c++;
			}
			fprintf(debug,"}\n");
		}
	}
}

void showTiles(int player)
{
	int i;
	fprintf(debug,"Tiles: { ");
	for (i=0;i<27;i++)
		if (Tiles[player][i])
			fprintf(debug,"%d %c's ",Tiles[player][i],i==27?'_':(i+'A'));
	fprintf(debug," }\n");
}

#endif

void putLetter(int player, int r, int c, char ch)
{
	if (isEmpty(r,c))
	{
#ifdef DEBUG
		fprintf(debug, "In putLetter(%d, %d, %d, %c)\n",
			player, r, c, ch);
#endif
		if (ch<'a')
		{
			if (!GetTileCount(player,ch)) myAssert(ch,0);
			useLetter(player,ch);
			boardLetters[r][c] = (uchar)(ch-'A');
		}
		else
		{
			if (!GetTileCount(player,BLANKVAL)) myAssert(BLANKVAL,0);
			useLetter(player,BLANKVAL);
			boardLetters[r][c] = (uchar)(0x80 | (ch-'a'));
		}
		boardLetters[r][c] |= ( (player==HUMAN) ? 0x40 : 0 );
	}
}

static void useage(void)
{
	fputs("Usage:\n\nww [-D] [-B0|-B1] [-s|-S|-Q] [-M] [-H] [-c] [-f <cfgfile>] [-E|<gamefile>]\n\n",stderr);
	fputs("\n\t-D shows computer's tiles (only if a board edge is 15 or more)\n\t-H shows this message\n\t",stderr);
	fputs("-s, -S and -Q are used for strategy testing. The computer plays\n",stderr);
	fputs("\t   against a basic maximal score opponent. -S is faster than -s.\n",stderr);
	fputs("\t   -Q is fastest (no graphics).\n",stderr);
	fputs("\t-c draws your tiles in a different colour so you can distinguish them\n",stderr);
	fputs("\t-B0 specifies that you want the program to play first\n",stderr);
	fputs("\t-B1 specifies that you want to play first\n",stderr);
	fputs("\t-M disables mouse processing\n",stderr);
	fputs("\t-E enters board editor at start of game\n",stderr);
	fputs("\t<cfgfile> can be used to load a different configuration\n",stderr);
	fputs("\t\t(the default is to load \"ww.cfg\")\n",stderr);
	fputs("\t<gamefile> can be used to start up from a saved game\n",stderr);
	exit(0);
}

static int addToTrash(char *tiles, int ltr, int cnt, int pos)
{
	addToHeap('A'+ltr,cnt,650);
	while (cnt--) tiles[pos++] = (char)('A'+ltr);
	return pos;
}

int breakHandler(void)
{
	ioAbort();
	return 0; /* terminate */
}

static void swapTiles(int player)
{
	int dcnt = 0, doU=1, doQ=0, tpos = 0, i, pass=0;
	/* We do a few passes, stopping when we have trashed at least 1/3
		The rules are:
		* a Q won't be discarded if we have a U or blank
		* a U won't be discarded if we have a Q
		* blanks are never discarded
		* otherwise we discard letters with positive weights,
			starting with those we have the most of.
	*/
	if (GetTileCount(player,'Q'))
	{
		if (GetTileCount(player,'U'))
			doU = 0;
		else if (!GetTileCount(player,BLANKVAL))
			doQ = 1;
	}
	while (dcnt <= (numtiles/3))
	{
		pass++;
		for (i=0; i<BLANK; i++) // never discard blank
		{
			int n = Tiles[player][i];
			char ch = 'A' + i;

			if (n==0 || (ch=='U' && !doU) || (ch=='Q' && !doQ))
				continue;

			if (Weights[i] >= 0)
			{
				/* throw in those we have the most of first;
				   temper this choice with the weight */
				if ( (pass+n+(Weights[i]*pass)/3) > 5)
					n = 1;
				else n = 0;
			}

			if (n)
			{
				dcnt += n;
				tpos = addToTrash(trash, i, n, tpos);
				takeFromRack(player, ch, n, 980);
			}
		}
	}
	if (dcnt)
	{
		char msg[40];
		if (player)
			sprintf(msg,"Control drops %d tiles",dcnt);
		else
			sprintf(msg,"I drop %d tiles",dcnt);
		showInfo(1,msg);
	}
	else showInfo(1, player ? "Control passes" : "I pass");
	trash[tpos]=0;
}

static void ProcessArgs(int argc, char *argv[])
{
	int gmarg = 1;
	firstPlayer = -1;
	hasmouse = 1;
	strategyTest = NOTEST;
	noGraphics = 0;
	Zippy = 0;
	edit = 0;
	colourTiles = showCompTiles=0;
	cfgname = "ww.cfg";
	while (gmarg<argc)
	{
		if (argv[gmarg][0]=='-')
		{
			switch (argv[gmarg][1])
			{
			case 'E': edit=1; break;
			case 'R': makeRegInfo(); /* doesn't return if demo version */
			case 'D': showCompTiles=1; break;
			case 'M': hasmouse = 0; break;
			case 'H': useage();
			case 'B': if (argv[gmarg][2]=='0')
					firstPlayer = 0;
				else if (argv[gmarg][2]=='1')
					firstPlayer = 1;
				else useage();
				break;
			case 'Z':
				/* back door to fast demo version */
				if (strcmp(argv[gmarg]+2, "ip")==0)
					Zippy = 1;
				break;
			case 'Q': if (strategyTest) useage();
				noGraphics = 1;
				/* fall thru... */
			case 'S': if (strategyTest) useage();
				strategyTest=FASTTEST;
				if (isdigit(argv[gmarg][2]))
				{
					gameCount = atoi(argv[gmarg]+2);
					if (gameCount<1) gameCount=1;
				}
				else gameCount = 1;
				break;
			case 's': if (strategyTest) useage();
				strategyTest=SLOWTEST;
				break;
			case 'c': colourTiles = 1;
				break;
			case 'f': gmarg++;
				cfgname = argv[gmarg];
				break;
			default: useage();
			}
		}
		else break;
		gmarg++;
	}
	if (strategyTest)
		showCompTiles=1;
	if (gmarg<argc)
	{
		gamname=argv[gmarg++];
		if (gmarg<argc || edit)
			useage();
	}
	else gamname = NULL;
}

void main(int argc, char *argv[])
{
	int first=1, playNext, xchd;
	int oldStrategyTest = 1;
	char *wrd;
	ctrlbrk(breakHandler);
	inGrafxMode = 0;
	randomize();
	ProcessArgs(argc, argv);
#ifdef DEBUG
	debug = fopen("debug.ww","wt");
#endif
restart:
	first = 1;
	Setup(cfgname);
	if (gamname)
	{
		if (loadGameFile(gamname, 1, &first) != 0)
		{
			ioAbort();
			fprintf(stderr,"Can't load saved game %s!\n",gamname);
			exit(0);
		}
		gamname = NULL; // only load it the first time around
	}
	computedMoves = playNext = 0;
	if (edit)
		EditBoard();
	for (;;player = 1 - player)
	{
		moves++;
	restart_move:
		DisplayBoard(SLOW);
		/* flush the keyboard buffer, disabling auto play
			if a key was pressed */
		while (kbhit())
		{
			if (getch() != 13 && strategyTest != NOTEST)
			{
				oldStrategyTest = strategyTest;
				strategyTest = NOTEST;
			}
		}
		if (strategyTest == SLOWTEST)
			getKeyPress();
		if (player == HUMAN && strategyTest == NOTEST)
		{
			switch (getUserMove(&first))
			{
			case RESTART:
				goto restart;
			case PLAYNEXT:
				playNext = 1;
				goto restart_move;
			case WATCH:
				strategyTest = oldStrategyTest;
				showCompTiles = 1;
				goto restart_move;
			case REDO:
			case LOADEDNEW:
			case UNDONE:
				continue;
			case PLAYEDMOVE:
				stalemate = first = 0;
				break;
			case PASS:
				stalemate++;
				break;
			}
			if (DrawTiles(player)==0) // finished
				break;
		}
		else
		{
			int r, c;
			char ch;
			/* If we don't have two or more blanks, try swap */
			if (NumBlanks(player)<2 && canSwap(&r, &c, &ch))
			{
				doSwap(r, c, ch, first);
				if (exchangeAllowed==XCHANGEONLY)
					continue; // that's it
				else xchd = 1;
			}
			else xchd = 0;
			if (!xchd || exchangeAllowed==XCHANGEANY)
			{
				/* If this is a play next, we select the
				   next move from the movelist, else we
				   build the movelist */

				if (playNext && numHints>0)
				{
					/* select next best move */
					playNext = 0;
					lastShownHint = (lastShownHint+numHints-1) % numHints;
					bestmove.score = moveList[lastShownHint].score;
					bestmove.weight = moveList[lastShownHint].weight;
					bestmove.r = moveList[lastShownHint].r;
					bestmove.c = moveList[lastShownHint].c;
					bestmove.isAcross = moveList[lastShownHint].isAcross;
					strcpy(bestmove.word,moveList[lastShownHint].word);
					strcpy(bestmove.mask,moveList[lastShownHint].mask);
				}
				else
				{
					if (hasmouse)
					{
						MouseCursor(1);
						ShowMouse();
					}
					findMove(first); /* build movelist */
					if (hasmouse)
					{
						HideMouse();
						MouseCursor(0);
					}
					lastShownHint = numHints-1;
					computedMoves = 1;
				}
			}
			else bestmove.score = 0; /* must pass after xchange */
			if (bestmove.score == 0) /* No move so swap tiles */
			{
				if (strategyTest != FASTTEST)
				{
				  	sound(2000);
					delay(500);
					nosound();
				}
				stalemate++;
				/* Trash the tiles */
				if (tilesleft && numtiles>1)
				{
					swapTiles(player);
					recordDrop(player,trash,moves,first);
				}
			}
			else
			{
				char msg[40];
				sprintf(msg,player?"Ctl plays %c %c %c %s":
					           "I play %c %c %c %s",
					bestmove.isAcross?'A':'D',
					bestmove.r+'A'-1,bestmove.c+'A'-1,
					bestmove.word);
				showInfo(1,msg);
#if 0 /* debug code */
{
	int l = strlen(bestmove.word);
	int sc = 0;
	isAcross = bestmove.isAcross;
	(void)evalMove(bestmove.word, NULL,
			bestmove.r+(bestmove.isAcross?0:(l-1)),
			bestmove.c+(bestmove.isAcross?(l-1):0), &sc);
}
#endif
				Score[player] += bestmove.score;
				stalemate = 0;
				/* Put the word in and remove from tile set */
				r = bestmove.r;
				c = bestmove.c;
				wrd = bestmove.word;
				strcpy(lastCompWord,wrd);
				recordMove(player,bestmove.isAcross?'A':'D',r,c,bestmove.mask,bestmove.score,moves,first);
				first = 0;
				if (bestmove.isAcross)
					while (*wrd)
						putLetter(player, r,c++,*wrd++);
				else
					while (*wrd)
						putLetter(player, r++,c,*wrd++);
			}

			/* Check for stalemate. If there are still
				tiles in the pool, we expect more
				passes than if the tiles are few
				or exhausted */

			if (stalemate>8) break;
			if (stalemate>4 && tilesleft<10) break;
			if (stalemate>2 && tilesleft==0) break;

			/* Draw new tiles and update display */

			if (DrawTiles(player)==0)
			{
				DisplayBoard(SLOW);
				break;
			}
		} /* end of computer move */
	}
	DisplayBoard(FAST);
	shutdown();
	if (strategyTest != FASTTEST)
	{
		printf("\n\nPress any key to play again\n");
		(void)getch();
		goto restart;
	}
	else if (--gameCount)
		goto restart;
	freeDict();
}

