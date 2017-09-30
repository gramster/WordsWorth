#define VERSION	"0.3.1"

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

extern NODE	*Edges;
#define Nodes(n)	Edges[n]

#define huge
#define far
#define farcalloc	calloc
#define farfree		free
#define farcoreleft()	0l

#else /* not UNIX */

#define PutCH		PutVideoChar
#define CursorAt	cursor
#define Randomize	randomize

extern NODE	*Edges[8];
#define Nodes(n)	Edges[((unsigned)(n))>>13][((unsigned)(n))&0x1FFF]

#endif /* UNIX */

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#define MAXBOARD	21	/* Max # of rows, columns	   */
#define MAXTILES	15	/* Max tiles per player; usually 7 */

#define SQ_NORMAL	0
#define SQ_LETTER2	1
#define SQ_LETTER3	2
#define SQ_WORD2	3
#define SQ_WORD3	4

typedef struct {
	char word[MAXBOARD+1];
	int r,c, score, isAcross;
} MOVE;

#define BLANKCH		'_'
#define BLANKVAL	('Z'+1)
#define EMPTYVAL	('Z'+2)	
#define BLANK		26
#define EMPTY		27		// boardLetter code

#define isEmpty(r,c)	(boardLetters[r][c]==EMPTY)
#define letterAt(r,c)	((boardLetters[r][c]&0x1F)+'A')
#define useLetter(p,c)		Tiles[p][(c)-'A']--


#define isLetter(c)	(((c)>='A' && (c)<='Z') || ((c)==BLANKCH))

/**************/
/* PROTOTYPES */
/**************/

/* From move.c */

extern void	findMove(int first);

/* From io.c */

extern int	scrInit(void);
extern void	scrEnd(void);
extern void	printBoard(void);

/* from config.c */

extern void	loadCFG(void);


/********************/
/* Global Variables */
/********************/

GLOBAL int	Rows, Cols;	/* board size		*/
GLOBAL uchar	boardLetters[MAXBOARD+2][MAXBOARD+2];
GLOBAL uchar	boardScores[MAXBOARD+1][MAXBOARD+1];
GLOBAL char	Tiles[2][27];
GLOBAL int	numtiles;	/* tiles per player	*/
GLOBAL int	myScore;
GLOBAL int	yourScore;
GLOBAL int	moves;
GLOBAL int	tilesleft;	/* tiles still to be played */
GLOBAL int	bonus;		/* bonus points		*/
GLOBAL uchar	Scores[27];	/* Letter scores	*/
GLOBAL uchar	Freq[27];	/* Letter frequencies	*/
GLOBAL MOVE	bestmove;
GLOBAL ulong	tilemask;	/* set of available letters */
GLOBAL int	Arow, Acol;	/* anchor pos		*/
GLOBAL int	isAcross;	/* doing across moves?	*/
GLOBAL ulong	XChkAcross[MAXBOARD+2][MAXBOARD+2];
GLOBAL ulong	XChkDown[MAXBOARD+2][MAXBOARD+2];
GLOBAL ulong	*CrossCheck; // Current cross check vector in use
GLOBAL int	XSum[MAXBOARD+2][MAXBOARD+2];
GLOBAL ulong	Anchor[MAXBOARD+1];
GLOBAL char	myPlay[MAXBOARD+1];

