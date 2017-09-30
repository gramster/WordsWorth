#define MAX_HINTS	20

#ifdef UNIX

enum COLORS {
    BLACK,          /* dark colors */
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,       /* light colors */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

#define clrscr	ScreenClear
#define gotoxy
#define putch
#define textcolor
#define textbackground

#define huge
#define far
#define farcalloc	calloc
#define farfree		free
#define farcoreleft()	0l

#else /* not UNIX */

#define PutCH		PutVideoChar
#define CursorAt	cursor
#define Randomize	randomize


#endif /* UNIX */

/* The following objects each have a colour and texture */

#define CLEAR_AREA	0
#define BOARD_AREA	1
#define LEFT_BSHADOW	2
#define TOP_BSHADOW	3
#define LEFT_TSHADOW	4
#define TOP_TSHADOW	5
#define COMPUTER_TILES	6
#define HUMAN_TILES	7	
#define BLACK_SQUARES	8	
#define RACK_VERT	9
#define RACK_HORIZ	10

#define NUM_FILLS	11

#define BLANK_TILE	11	/* color only */
#define NORMAL_TILE	12	/* color only */
#define MESSAGES	13	/* color only */
#define LABELS		14	/* color only */
#define TITLE		15	/* Color only */
#define AUTHOR		16	/* color only */

#define NUM_COLORS	17

/* Help Entries */

#define HLP_PATTERN	0	/* enter a consult pattern */
#define HLP_DROP	1	/* enter letters to drop */
#define HLP_WORD	2	/* enter word to play */
#define HLP_LFNAME	3	/* load file name */
#define HLP_SFNAME	4	/* save file name */
#define HLP_RLABEL	5	/* enter row */
#define HLP_CLABEL	6	/* enter col */
#define HLP_MOVE	7	/* specify move */
#define HLP_ANAGRAM	8	/* Order or anagram */
#define HLP_SOME	9	/* All or some */
#define HLP_ANYKEY	10	/* Press any key */
#define HLP_QUIT	11	/* Quit are you sure? */
#define HLP_HINT	12	/* Play hint ynh? */
#define HLP_XCHANGE	13	/* Exchange blank */
#define HLP_XLETTER	14	
#define HLP_EXWORD	15	/* Exclude word from play */
#define HLP_DIRECT	16	/* Head or tail suffix */

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#define MAXBOARD	21	/* Max # of rows, columns	   */
#define MAXTILES	15	/* Max tiles per player; usually 7 */
#define MAXEXWORDS	16	/* Max new excluded words per session */

#define COMPUTER 0
#define HUMAN	 1

#define SQ_NORMAL	0
#define SQ_LETTER2	1
#define SQ_LETTER3	2
#define SQ_WORD2	3
#define SQ_WORD3	4

typedef struct {
	char word[MAXBOARD+1];
	char mask[MAXBOARD+1];
	char r, c, isAcross;
	short score, weight;
} MOVE; /* move */

typedef struct _MOVEREC {
	char action, row, col, first;
	char mask[MAXBOARD+1];
	char draw[MAXBOARD+1];
	short score, movenum;
	struct _MOVEREC *next, *prev; /* doubly linked list */

} *MOVEREC; /* move history, for save/load/undo/redo */

#define BLANKCH		'_'
#define BLACKCH		'@'	/* boardscores value for black	*/

/* Codes for boardletters */

#define BLANKVAL	('Z'+1)
#define EMPTYVAL	('Z'+2)	
#define BLANK		26
#define EMPTY		27		/* boardLetter code */
#define BLACKSQ		28

#define isEmpty(r,c)	((boardLetters[r][c]&0x1F)==EMPTY)
#define letterAt(r,c)	((boardLetters[r][c]&0x1F)+'A')
#define isLetter(c)	(((c)>='A' && (c)<='Z') || ((c)==BLANKCH))

/*
 * Return values for getUserMOve
 */

#define RESTART		1
#define PLAYNEXT	2
#define WATCH		3
#define REDO		4
#define LOADEDNEW	5
#define PLAYEDMOVE	6
#define PASS		7
#define UNDONE		8

/*
 * Return values for CheckMove
 */

#define MOVEOK		0	/* move is OK			*/
#define NOTINROW	1	/* word doesn't cross start row	*/
#define NOTINDICT	2	/* Not in dictionary		*/
#define NOANCHOR	3	/* no anchor square		*/
#define CONFLICT	4	/* conflicts with board 	*/
#define NOBLANK		5	/* word has blank; player not	*/
#define NOLETTER	6	/* word has letter; player not	*/
#define BADCROSS	7	/* Bad cross word		*/
#define BADEND		8	/* Can't end word where it does	*/
#define NOTINCOL	9	/* word doesn't cross start col	*/
#define BADSTART	10	/* Can't start where it does	*/

/*
 * Argument flags for DisplayBoard
 */

#define FAST		0
#define SLOW		1

/*
 * Strategy test types
 */

#define FASTTEST	3
#define SLOWTEST	2
#define NOTEST		0

/*
 * exchangeAllowed values
 */

#define NOXCHANGE	0	/* no exchanges			*/
#define XCHANGEONLY	1	/* exchange is whole move	*/
#define XCHANGEPASS	2	/* exchange is part of pass	*/
#define XCHANGEANY	3	/* exchange OK before any move	*/

/*
 * Useful macro `functions'
 */

#define NumBlanks(p)		(Tiles[p][BLANK])
#define NumBlanks(p)		(Tiles[p][BLANK])

/**************/
/* PROTOTYPES */
/**************/

/* From move.c */

extern int	GotAbove(int r, int c);
extern int	GotBelow(int r, int c);
extern int	GotLeft(int r, int c);
extern int	GotRight(int r, int c);
extern void	useLetter(int p, char c);
extern void	replaceLetter(int p, char c);
extern int	GetTileCount(int p, char c);
extern int	FindWildTile(int c,int *R, int *C);
extern void	findMove(int first);
extern int	HasTile(int r, int c);
extern int	canSwap(int *row, int *col, char *tile);
extern void	doSwap(int r, int c, char ch, int first);
extern void	matchPattern(char *pat, int anagrams, int allLengths,
						int repeats, int showThink);
extern int	checkMove(char *word, int row, int col, int first, int *status);

/* From screen.c */

extern void	autodetectVCard(int *card);
extern void	putStringAt(int row, char *msg);
extern void	printOutline(void);
extern void	clearUndoJunk(void);
extern int	scrInit(void);
extern void	scrDraw(void);
extern void	DisplayBoard(int slowly);
extern void	showPercent(double v);
extern void	showPossibleMove(int row, int col, char *word);
extern void	EditBoard(void);
extern int	getUserMove(int *first);
extern int	override(char *msg);
extern int	override2(char *msg);
extern void	ioAbort(void);
extern int 	sayBye(void);
extern void	showInfo(int paws, char *msg);
extern void	drawGrid(void);
extern int	showMatch(char *word);
extern void	getKeyPress(void);
extern void	showCopyright(void);
extern void	showDropMessage(int cnt);
extern void	printIntro(void);
extern int	endGameMessage(void);
extern void	dawgLoadFail(void);
extern void	makeRegInfo(void);

extern void	MouseCursor(int Wait);
extern void	ShowMouse(void);
extern void	HideMouse(void);
extern int	ButtonUp(int b);
extern void	MousePos(int *x, int *y);
extern int	InitMouse(void);

/* from config.c */

extern int	getNumber(FILE *fp, char *msg,int canBneg,int Min,int Max);
extern char	getChar(FILE *fp, char *msg);
extern void	loadCFG(char *cfgname);
extern int	loadBoard(FILE *cfg);

/* from record.c */

extern void	freeMoveRecords(void);
extern void	recordMove(int player, char action, int row, int col,
			char *mask, int score, int movenum, int first);
extern void	recordDraw(int player, char *tiles);
extern void	recordDrop(int player, char *tiles, int movenum, int first);
extern int 	doRedo(int player);
extern void	saveGame(char *name);
extern int  	loadGameFile(char *name, int gotHeader, int *first);
extern int	undo1Move(int trim, int *first);
extern int	undo2Moves(int *first);
extern void	resetData(int clearAll);
 
/* from ww.c */

extern void	takeFromRack(int p, char t, int cnt, int id);
extern void	addToRack(int p, char t, int cnt, int id);
extern void	takeFromHeap(char letter, int cnt, int id);
extern void	addToHeap(char letter, int cnt, int id);
extern int	getTiles(int player);
extern void	shutdown(void);
extern void	putLetter(int player, int r, int c, char ch);

/********************/
/* Global Variables */
/********************/

typedef uchar boardTable[MAXBOARD+2][MAXBOARD+2];

GLOBAL short	Rows, Cols;	/* board size		*/
GLOBAL boardTable
		initBoardLetters,
		boardLetters,
		screenLetters,
		boardScores;
GLOBAL short	Tiles[4][27];
GLOBAL int	hasmouse;
GLOBAL int	numtiles;	/* tiles per player	*/
GLOBAL int	Score[2];
GLOBAL int	moves;
GLOBAL int	player;
GLOBAL int	tilesleft;	/* tiles still to be played */
GLOBAL int	bonus;		/* bonus points		*/
GLOBAL int	showCompTiles;
GLOBAL short	Scores[27];	/* Letter scores	*/
GLOBAL short	PoolCnt[27];	/* Letter frequencies	*/
GLOBAL short	PoolStart[27];	/* Letter frequencies	*/
GLOBAL short	Weights[27];	/* Letter weights	*/
GLOBAL MOVE	bestmove;
GLOBAL MOVE	moveList[MAX_HINTS];
GLOBAL int	numHints;
GLOBAL int	lowestHintWeight;
GLOBAL int	lastShownHint;
GLOBAL int	stalemate;
GLOBAL int	Zippy;
GLOBAL int	Arow, Acol;	/* anchor pos		*/
GLOBAL int	isAcross;	/* doing across moves?	*/
GLOBAL ulong	XChkAcross[MAXBOARD+2][MAXBOARD+2];
GLOBAL ulong	XChkDown[MAXBOARD+2][MAXBOARD+2];
GLOBAL ulong	*CrossCheck; /* Current cross check vector in use */
GLOBAL char	Anchor[MAXBOARD+1];
GLOBAL char	myPlay[MAXBOARD+1];
GLOBAL char	myMask[MAXBOARD+1];
GLOBAL char	lastCompWord[MAXBOARD+1];
GLOBAL char	WordMark[10],
		LetterMark[10],
		inGrafxMode,
		computedMoves;
GLOBAL short	WordMarkColor[10],
		LetterMarkColor[10],
		LetterMarkBColor[10],
		WordMarkBColor[10],
		LetterMarkTexture[10],
		WordMarkTexture[10];
GLOBAL void	showPercent(double v);
GLOBAL short	lengthWeight,
		minScoreLimit,
		maxScoreLimit,
		MaxNewXWord,
		searchMin,
		searchCnt,
		strategyTest,
		noGraphics,
		colourTiles,
		minLength,
		maxLength,
		firstMoveRow,
		firstMoveCol,
		firstMoveDir,
		firstPlayer,
		controlStrategy,
		adaptiveStrategy,
		freeForm,
		noGiveaway,
		blankThreshold,
		videoCard,
		videoMode,
		exchangeAllowed,
		hotSquares,
		weightWhat,
		useLetterWeights,
		linenum;
GLOBAL MOVEREC	moveRecHead[2],	/* Linked list heads	*/
		moveRecTail[2],	/* and tails		*/
		moveRecNow[2];	/* Current pos in list	*/
GLOBAL short	ColorMap[NUM_COLORS],
		TextureMap[NUM_FILLS];

#ifdef DEBUG
GLOBAL FILE	*debug;
#endif
