#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <graphics.h>
#include <assert.h>
#include "svga256.h"

#define GLOBAL extern

#include "version.h"
#include "dict.h"
#include "reg.h"

#include "ww.h"

#define SHADOWSIZE	3
#define CLEARLEFT	(2*X/3)
#define TEXTLEFT	(CLEARLEFT+8)
#define ROWPOS(r)	(Y*(r)/25)
#define CLEARTOP	(ROWPOS(10)-textHeight-2)

#define RACKAREA	1
#define BOARDAREA	2
#define TEXTAREA	3

static int X, Y, W, H, Xinc, Yinc, textHeight,
	mousex, mousey, mouserow, mousecol, mousepos, mousechar;
static int far poly[8];
static void showHelp(int hlpindex);
extern char Register[];
static int defaultFont, defaultMag, tileFont;

short defaultTextureMap[NUM_FILLS] = { // For Hercs
	(short)SOLID_FILL,	// clear area
	(short)CLOSE_DOT_FILL,	// board_area	
	(short)SOLID_FILL,	// left_bshadow	
	(short)SOLID_FILL,	// top_bshadow	
	(short)SOLID_FILL,	// left_tshadow	
	(short)SOLID_FILL,	// top_tshadow	
	(short)SOLID_FILL,	// computer_tiles
	(short)SOLID_FILL,	// human_tiles	
	(short)SOLID_FILL,	// black_squares	
	(short)CLOSE_DOT_FILL,	// rack_vert	
	(short)WIDE_DOT_FILL	// rack_horiz	
};

short defaultColorMap[NUM_COLORS] = { // For EGA/VGA
	(short)BLACK,		// clear area
	(short)CYAN,		// board area
	(short)DARKGRAY,	// left board shadow
	(short)LIGHTGRAY,	// top board shadow
	(short)DARKGRAY,	// left tile shadow
	(short)LIGHTGRAY,	// top tile shadow
	(short)WHITE,		// computer tiles
	(short)LIGHTBLUE,	// human tiles
	(short)BLACK,		// black squares
	(short)BLUE,		// rack vert
	(short)LIGHTBLUE,	// rack horiz
	(short)RED,		// blank tile
	(short)BLACK,		// normal tile
	(short)WHITE,		// messages
	(short)LIGHTGRAY,	// labels
	(short)CYAN,		// title
	(short)LIGHTGRAY	// author
};

/*
 * next is adpated from move.c to help reduce screen fills
 */

static int wasTile(int r, int c)	{ register char x=(char)screenLetters[r][c];
				  return (x!=EMPTY && x!=BLACKSQ); }

void SetFillStyle(int area)
{
	if (!noGraphics)
		setfillstyle(TextureMap[area],ColorMap[area]);
}

void clearMessageArea(void)
{
	if (noGraphics) return;
	SetFillStyle(CLEAR_AREA);
	bar(CLEARLEFT,ROWPOS(16)-textHeight-2,X,Y);
}

void clearInfoArea(void)
{
	if (noGraphics) return;
	SetFillStyle(CLEAR_AREA);
	bar(CLEARLEFT,ROWPOS(21)-textHeight-2,X,Y);
}

static void _drawSquare(int i, int j, int col, int fil)
{
	int x, y;
	x = Xinc*(j+1);
	y = Yinc*(i+1);
	setcolor((int)BLACK);
	line(x,y,x+Xinc-1,y);
	line(x,y,x,y+Yinc-1);
	line(x+Xinc-1,y,x+Xinc-1,y+Yinc-1);
	line(x,y+Yinc-1,x+Xinc-1,y+Yinc-1);
	setfillstyle(fil,col);
	bar(x+1,y+1,x+Xinc-2,y+Yinc-2);
}

static void drawSquare(int i, int j, int a)
{
	_drawSquare(i,j,ColorMap[a],TextureMap[a]);
}

void clearUndoJunk(void)
{
	if (noGraphics)
		return;
	SetFillStyle(CLEAR_AREA);
	bar(Xinc,(H+1)*Yinc,(W+1)*Xinc,(H+1)*Yinc+3); // clear below board
	bar((W+1)*Xinc,Yinc,(W+1)*Xinc+3,(H+1)*Yinc); // clear to right
}

void drawGrid(void)
{
	int i, j;
	if (noGraphics)
		return;
	for (i=0;i<H;i++)
		for (j=0;j<W;j++)
			drawSquare(i,j,BOARD_AREA);
	// Draw left board shadow
	SetFillStyle(LEFT_BSHADOW);
	poly[0] = Xinc-SHADOWSIZE;	poly[1] = Yinc-SHADOWSIZE;
	poly[2] = Xinc-SHADOWSIZE;	poly[3] = (H+1)*Yinc-SHADOWSIZE-1;
	poly[4] = Xinc;			poly[5] = (H+1)*Yinc-1;
	poly[6] = Xinc;			poly[7] = Yinc;
	fillpoly(4,poly);
	SetFillStyle(TOP_BSHADOW);
	poly[0] = Xinc-SHADOWSIZE;		poly[1] = Yinc-SHADOWSIZE;
	poly[2] = (W+1)*Xinc-SHADOWSIZE-1;	poly[3] = Yinc-SHADOWSIZE;
	poly[4] = (W+1)*Xinc-1;			poly[5] = Yinc;
	poly[6] = Xinc;				poly[7] = Yinc;
	fillpoly(4,poly);
}

void printOutline(void)
{
	int i, j;
	char msg[2];
	if (noGraphics) return;
	msg[1]=0;
	drawGrid();
	/* Print row/column letter labels */
	setcolor(ColorMap[LABELS]);
	settextjustify((int)LEFT_TEXT, (int)BOTTOM_TEXT);
	if (videoCard==1) settextstyle((int)DEFAULT_FONT,HORIZ_DIR,1);
	else settextstyle((int)SANS_SERIF_FONT,HORIZ_DIR,1);
	for (i=1;i<=Rows;i++) {
		msg[0] = (char)('A'+i-1);
		outtextxy(Xinc-textwidth(msg)-5,i*Yinc+(Yinc+textheight(msg))/2,msg);
	}
	for (j=1;j<=Cols;j++) {
		msg[0] = (char)('A'+j-1);
		outtextxy(j*Xinc+(Xinc-textwidth(msg))/2,Yinc-8/*(Yinc-textheight(msg))/2*/,msg);
	}
}

static int Pos2RackTile(int x)
{
	int i, j;
	for (i=j=0;j<27;j++)
	{
		int cnt = Tiles[HUMAN][j];
		while (cnt--)
		{
			if (++i == x)
				return j;
		}
	}
	return -1;
}

// where => -2 (title) -1 (board) HUMAN (human rack) COMPUTER (comp rack)

static void drawTile(int i, int j, char c, int col, int val, int where) {
	int x, y, mag, offsetx, offsety;
	char msg[3];
	x = Xinc*j;
	y = Yinc*i;
	if (where>=0) // not on board?
	{
		y = Yinc*(H+1)+10; // human rack position
		if (where==COMPUTER) x+= Xinc*(numtiles+1); // add distance to computer rack
	}
	if ((x+Xinc+SHADOWSIZE)>CLEARLEFT)
		return;
	// draw top shadow
	if (where>=0 || (where == -1 && (i==1 || isEmpty(i-1,j)))) {
		setcolor(ColorMap[TOP_TSHADOW]); // this seems to be necessary
		SetFillStyle(TOP_TSHADOW);
		poly[0] = x+SHADOWSIZE;	poly[1] = y;
		poly[2] = x+Xinc;	poly[3] = y;
		poly[4] = x+Xinc+SHADOWSIZE;poly[5] = y+SHADOWSIZE;
		poly[6] = x+SHADOWSIZE;	poly[7] = y+SHADOWSIZE;
		fillpoly(4,poly);
		if (where>=0 || i==1 || j==1 || isEmpty(i-1,j-1))
		{
			poly[0] = x+1;/**/	poly[1] = y;
			poly[2] = x+SHADOWSIZE;	poly[3] = y;
			poly[4] = x+SHADOWSIZE;	poly[5] = y+SHADOWSIZE-1;
			fillpoly(3,poly);
		}
		setcolor((int)BLACK);
		line(x+Xinc+SHADOWSIZE-1,y+SHADOWSIZE,x+Xinc-1,y);
	}
	// draw left shadow
	if (j==1 || (where == -1 && isEmpty(i,j-1)))
	{
		setcolor(ColorMap[LEFT_TSHADOW]); // this seems to be necessary
		SetFillStyle(LEFT_TSHADOW);
		poly[0] = x;		poly[1] = y+SHADOWSIZE;
		poly[2] = x+SHADOWSIZE;	poly[3] = y+SHADOWSIZE;
		poly[4] = x+SHADOWSIZE;	poly[5] = y+Yinc+SHADOWSIZE;
		poly[6] = x;		poly[7] = y+Yinc;
		fillpoly(4,poly);
		if (j==1 || (where<0 && isEmpty(i-1,j-1))) {
			poly[0] = x;		poly[1] = y;
			poly[2] = x;		poly[3] = y+SHADOWSIZE;
			poly[4] = x+SHADOWSIZE;	poly[5] = y+SHADOWSIZE;
			fillpoly(3,poly);
		}
	}
	// draw background

	x+= SHADOWSIZE;
	y+= SHADOWSIZE;

	SetFillStyle(COMPUTER_TILES);
	if (colourTiles)
		if (where==HUMAN || (where<0 && boardLetters[i][j] & 0x40))
			SetFillStyle(HUMAN_TILES);
	bar(x,y,x+Xinc-1,y+Yinc-1);

	// draw letters
	mag = (Xinc-10)/8;
	if (mag>((Yinc-10)/8)) mag = (Yinc-2)/8;
	if (mag<1) mag=1;
	msg[0] = c;
	msg[1] = 0;
	setcolor(col);
	line(x,y,x+Xinc-1,y);
	line(x+Xinc-1,y,x+Xinc-1,y+Yinc-1);
	settextjustify((int)LEFT_TEXT,(int)BOTTOM_TEXT);
	settextstyle(tileFont,HORIZ_DIR,mag);
	offsetx = (Xinc-8-textwidth(msg))/2 + 1;
	offsety = (Yinc-8) - (Yinc-8-textheight(msg))/2;
	outtextxy(x+offsetx,y+offsety,msg); 
	settextjustify((int)RIGHT_TEXT,(int)BOTTOM_TEXT);
	settextstyle((int)DEFAULT_FONT,HORIZ_DIR,1);
	if (val>9)
	{
		msg[0] = (char)((val/10)+'0');
		msg[1] = (char)((val%10)+'0');
		msg[2] = 0;
	}
	else
	{
		msg[0] = (char)(val+'0');
		msg[1] = 0;
	}
	outtextxy(x+Xinc-1,y+Yinc-1,msg);
}

/* col in the next routine is in piece increments */

static void putStringAtRC(int row, int col, char *msg)
{
	settextjustify((int)LEFT_TEXT,(int)BOTTOM_TEXT);
	settextstyle(defaultFont,HORIZ_DIR,defaultMag);
	setcolor(ColorMap[MESSAGES]);
	outtextxy(col,ROWPOS(row),msg);
}

void putStringAt(int row, char *msg)
{
	if (!noGraphics)
		putStringAtRC(row, TEXTLEFT, msg);
}

void showInfo(int paws, char *msg)
{
	if (noGraphics) return;
	clearInfoArea();
	putStringAt(22,msg);
	if (paws>0)
		sleep((unsigned)paws);
}

static void doDelay(void) {
	if (strategyTest!=FASTTEST)
	{
		sound(100);
		delay(kbhit()?5:20);
		nosound();
		if (!kbhit()) delay(250);
	}
}

static void printSquare(int r, int c, char typ)
{
	int colour;
	char msg[2];
	msg[1]=0;
	if (!HasTile(r,c))
	{
		if (typ==BLACKCH)
			drawSquare(r-1,c-1,BLACK_SQUARES);
		else
		{
			if (typ=='a'|| typ=='A')
			{
				drawSquare(r-1,c-1,BOARD_AREA);
				msg[0]=0;
			}
			else if (typ>'a') /* double/triple ltr */
			{
				int i = typ-'b';
				_drawSquare(r-1,c-1,LetterMarkBColor[i],LetterMarkTexture[i]);
				colour = LetterMarkColor[i];
				msg[0] = LetterMark[i];
			}
			else if (typ>'A' && typ<'a') /* Double, triple word */
			{
				int i=typ-'B';
				_drawSquare(r-1,c-1,WordMarkBColor[i],WordMarkTexture[i]);
				colour = WordMarkColor[i];
				msg[0] = WordMark[i];
			}
			else /* error */;
			settextjustify((int)CENTER_TEXT,(int)CENTER_TEXT);
			settextstyle((int)SANS_SERIF_FONT,HORIZ_DIR,1);
			setcolor(colour);
			if (msg[0]<10)
			{
				if (videoCard==4 && videoMode==4)
					settextstyle((int)SANS_SERIF_FONT,HORIZ_DIR,2);
				outtextxy(c*Xinc+3*Xinc/10,r*Yinc+2*Yinc/5,msg);
			}
			else
				outtextxy(c*Xinc+2*Xinc/5,r*Yinc+2*Yinc/5,msg);
		}
	}
	else
	{
		char ch = letterAt(r,c);
  		if (boardLetters[r][c]&(uchar)0x80) // blank?
  			drawTile(r,c,ch,ColorMap[BLANK_TILE],Scores[BLANK],-1);
  		else 
  			drawTile(r,c,ch,ColorMap[NORMAL_TILE],Scores[ch-'A'],-1);
  	} // end if not tile
}

static void redrawNeighbours(int r, int c)
{
	if (GotAbove(r,c))
		printSquare(r-1,c,(char)boardScores[r-1][c]);
	if (GotBelow(r,c))
		printSquare(r+1,c,(char)boardScores[r+1][c]);
	if (GotLeft(r,c))
		printSquare(r,c-1,(char)boardScores[r][c-1]);
	if (GotRight(r,c))
		printSquare(r,c+1,(char)boardScores[r-1][c+1]);
}

static void fixSquare(int r, int c)
{
	printSquare(r,c,(char)boardScores[r][c]);
	redrawNeighbours(r,c);
}

void undoClear(int r, int c)  // `remove' a tile from the board - sheeesh!
{
	if (noGraphics) return;
	if (screenLetters[r][c]==boardLetters[r][c])
	{
		fixSquare(r,c);
		if (r<Rows && !HasTile(r+1,c) && !wasTile(r+1,c))
		{
			fixSquare(r+1,c);
			if (c<Cols && !HasTile(r+1,c+1) && !wasTile(r+1,c+1))
				fixSquare(r+1,c+1);
		}
		if (c<Cols && !HasTile(r,c+1) && !wasTile(r,c+1))
			fixSquare(r,c+1);
	}
}

void DisplayUndo(void)
{
	int i, j;
	for (i = 1; i <=Rows; i++)
	{
		for (j=1; j<=Cols; j++)
		{
			if (boardLetters[i][j] != screenLetters[i][j])
			{
				screenLetters[i][j] = boardLetters[i][j];
				undoClear(i,j);
			}
		}
	}
}

void DisplayBoard(int slowly)
{
	int i, j, p;
	char msg[40];
	if (noGraphics) return;
#ifdef DEMO_VERSION
	if (maxLength>6) maxLength = 6;
#endif
	for (i = 1; i <=Rows; i++)
	{
		for (j=1; j<=Cols; j++)
		{
			if (boardLetters[i][j] != screenLetters[i][j])
			{
				screenLetters[i][j] = boardLetters[i][j];
				printSquare(i,j,(char)boardScores[i][j]);
				if (slowly && strategyTest!=FASTTEST)
					doDelay();
			}
		}
	}
	if (numtiles>1)
	{
		/* Print the tiles */
		SetFillStyle(CLEAR_AREA);	// clear old tiles
		bar(0,(H+1)*Yinc+4,CLEARLEFT,Y-4);
		SetFillStyle(RACK_VERT);	// draw rack itself
		bar(Xinc/2,(H+1)*Yinc+7,(numtiles+1)*Xinc+Xinc/2,(H+2)*Yinc+10);
		SetFillStyle(RACK_HORIZ);
		poly[0] = Xinc/2;			poly[1] = (H+2)*Yinc+10;
		poly[2] = (numtiles+1)*Xinc+Xinc/2;	poly[3] = poly[1];
		poly[4] = poly[2]+Xinc/2;		poly[5] = poly[1]+Yinc/3;
		poly[6] = Xinc;				poly[7] = poly[5];
		fillpoly(4,poly);
		/* draw tiles */		
		for (p=0;p<2;p++)
		{
			if (p==0 && !showCompTiles) continue;
			for (i=j=0;j<27;j++)
			{
				int cnt = Tiles[p][j];
				while (cnt--)
				{
					if (j==BLANK) 
						drawTile(1,++i,'_',ColorMap[BLANK_TILE],Scores[BLANK],p);
					else 
						drawTile(1,++i,(char)(j+'A'),ColorMap[NORMAL_TILE],Scores[j],p);
				}
			}
		}
	}
	/* Print score, move and tiles remaining */
	SetFillStyle(CLEAR_AREA);	// clear old tiles
	bar(CLEARLEFT,CLEARTOP,X,Y);
	sprintf(msg,"My Score:   %d",Score[COMPUTER]);
	putStringAt(10,msg);
	sprintf(msg,"%s Score: %d",strategyTest?"Ctrl":"Your",Score[HUMAN]);
	putStringAt(11,msg);
	if (numtiles!=0)
	{
		sprintf(msg,"Tiles Left: %d",tilesleft);
		putStringAt(12,msg);
	}
	sprintf(msg,"Round %d  Move: %d",(moves+1)/2,moves);
	putStringAt(13,msg);
}

static int isVESA(void)
{
	char result[256];

	// The order of the next three is critical if they are
	// not to be written in proper assembler.

	_DI = (short)((char near *)result); // answer
	_ES = _SS;    
	_AX = 0x4F00; // check for VESA adapter

	geninterrupt(0x10);

	if (_AX == 0x4F) {
		if (strncmp("VESA",result,4)==0) return 1;
	}
	return 0;
}

void autodetectVCard(int *card)
{
	int gdriver,gmode;
	extern char *VCardNames[];
	detectgraph(&gdriver,&gmode);
	switch(gdriver)
	{
		case HERCMONO:	*card = 1;		break;
		case EGA:	*card = 2;		break;
		case VGA:	*card = 3 + isVESA();	break;
		default:	*card = 0;		break;
	}
	printf("%s detected\n",VCardNames[*card]);
	sleep(1);
}

static int gmode;

int huge TrueEnuf()
{
	return gmode;
}

void ioAbort(void)
{
	if (inGrafxMode) closegraph();
	textmode((int)C80);
	nosound();
}

int scrInit(void)
{
	int gdriver, errorcode, tmpFont;
	static int first=1;
	gmode = videoMode;
	switch(videoCard)
	{
		case 0 : // autodetect
			gdriver = (int)DETECT;
			if (gmode<0) gmode = 0;
			break;
		case 1:	gmode=0;
			gdriver = (int)HERCMONO;
			errorcode = registerfarbgidriver(Herc_driver_far);
			break;
		case 2: if (gmode<0) gmode = (int)EGAHI; // 640x350 16 color
			gdriver = (int)EGA;
			errorcode = registerfarbgidriver(EGAVGA_driver_far);
			break;
		case 3: if (gmode<0) gmode = (int)VGAHI; // 640x480 16 colors
			gdriver = (int)VGA;
			errorcode = registerfarbgidriver(EGAVGA_driver_far);
			break;
		case 4: // SVGA
			gdriver = (int)DETECT;
			if (gmode<0) gmode=4;
			(void)installuserdriver("SVGA256",TrueEnuf);
			errorcode = registerfarbgidriver(SVGA256_driver_far);
			break;
		default: fprintf(stderr,"Sorry, this game only supports EGA, VGA and Hercules\n");
			return 2;
	}
	printf("Wanted mode %d, driver %d\n",videoMode,videoCard);
	printf("Attempting mode %d, driver %d\n",gmode,gdriver);
	if (errorcode < 0) {
		printf("Graphics error: %s\n",grapherrormsg(errorcode));
		return 4;
	}
	if (noGraphics) return 0;
	initgraph(&gdriver,&gmode,"");
	inGrafxMode = 1;
	errorcode = graphresult();
	if (errorcode != (int)grOk)
	{
		textmode((int)C80);
		printf("Graphics error: %s\n",grapherrormsg(errorcode));
		return 3;
	}
	videoMode = gmode;
	/* get screen dimensions */
	X = getmaxx();
	Y = getmaxy();
	/* Register fonts */

	(void)registerfarbgifont(triplex_font_far);
	(void)registerfarbgifont(sansserif_font_far);
	(void)registerfarbgifont(gothic_font_far);
	(void)registerfarbgifont(small_font_far);

	/* Compute message font height */

	tmpFont = (int)SANS_SERIF_FONT;
	if (videoCard==4 && videoMode>2)
	{
		if (videoMode==4)
		{
			defaultFont = (int)TRIPLEX_FONT;
			defaultMag = 2;
			tmpFont = (int)TRIPLEX_FONT;
		} else
		{
			defaultFont = (int)SMALL_FONT;
			defaultMag = 6;
		}
	} else
	{
		defaultFont = (int)DEFAULT_FONT;
		defaultMag = 1;
	}
	settextstyle(defaultFont,HORIZ_DIR, defaultMag);
	textHeight = textheight("A");
	if (first) first = 0;
#ifdef DEMO_VERSION
	if (Zippy) goto skipIntro;
#else
	else goto skipIntro;
#endif

#ifdef OLD_INTRO
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	settextstyle(GOTHIC_FONT,HORIZ_DIR,7);
	setcolor(ColorMap[TITLE]);
	sprintf(title,"WordsWorth v%s",VERSION);
	outtextxy(X/2,Y/3,title);
	settextstyle(TRIPLEX_FONT,HORIZ_DIR,4);
	setcolor(ColorMap[AUTHOR]);
/*	sprintf(title,"by GramSoft (c) 1994");*/
	outtextxy(X/2,2*Y/3,"by Graham Wheeler");
#else
	/* Work out tmp tile dimensions */
	W = 7;
	H = 7;
	Xinc = 2*X / (3 * (W+2));
	Yinc = Y / (H+3);
	tileFont = (int)GOTHIC_FONT;
	{
		char *word = "Words";
		int r=4, c = 3;
		while (*word) {
			drawTile(r,c++,*word,ColorMap[NORMAL_TILE],Scores[*word-'A'],-2);
			word++;
			doDelay();
		}
	}
	{
		char *word = "W rth";
		int r=3, c = 4;
		while (*word)
		{
			if (*word!=' ')
			{
				drawTile(r,c,*word,ColorMap[NORMAL_TILE],
					Scores[*word-'A'],-2);
				doDelay();
			}
			word++; r++;
		}
	}
	settextjustify((int)CENTER_TEXT, (int)CENTER_TEXT);
	settextstyle((int)TRIPLEX_FONT,HORIZ_DIR,4);
	setcolor(ColorMap[AUTHOR]);
	outtextxy(3*X/4,2*Y/3,"by Graham Wheeler");
	outtextxy(3*X/4,2*Y/3+50,"(c) 1994");
#endif
	if (strategyTest!=FASTTEST) sleep(2);
skipIntro:;

	/* Work out real tile dimensions */
	W = Cols;
	H = Rows;
	Xinc = 2*X / (3 * (W+2));
	Yinc = Y / (H+3);
	tileFont = tmpFont;
	if (hasmouse)
		hasmouse = InitMouse();
	return 0;
}

void printTitle(void)
{
	char title[80];
	if (noGraphics) return;
	settextjustify((int)CENTER_TEXT,(int)CENTER_TEXT);
	settextstyle((int)GOTHIC_FONT,HORIZ_DIR,(videoMode==4)?3:1);
	setcolor(ColorMap[TITLE]);
	sprintf(title,"WordsWorth v%s",VERSION);
	outtextxy(5*X/6,ROWPOS(3),title);
	setcolor(ColorMap[AUTHOR]);
	outtextxy(5*X/6,ROWPOS(4)+ROWPOS(1)/2,"by Graham Wheeler");
	settextstyle(defaultFont,HORIZ_DIR,defaultMag);
	setcolor(ColorMap[MESSAGES]);
	outtextxy(5*X/6,ROWPOS(6),"Registered to:");
	sprintf(title,"   %s",Register);
	outtextxy(5*X/6,ROWPOS(7),title);
	outtextxy(5*X/6,ROWPOS(8),"Press F1 for Help");
}

void scrDraw(void)
{
	if (noGraphics) return;
	cleardevice();
	printTitle();
	printOutline();
}

void showPercent(double v)
{
	char msg[20];
	if (noGraphics) return;
	SetFillStyle(CLEAR_AREA);
	bar(CLEARLEFT,ROWPOS(14)-textHeight-2,X,ROWPOS(14)+2);
	sprintf(msg,"Thinking... (%02d)",(int)v);
	putStringAt(14,msg);
}

void showPossibleMove(int row, int col, char *word)
{
	char msg[30];
	if (noGraphics) return;
	SetFillStyle(CLEAR_AREA);
	bar(CLEARLEFT,ROWPOS(16)-textHeight-2,X,ROWPOS(16)+2);
	sprintf(msg,"%c %c %s", (row+'A'-1), (col+'A'-1), word);
	putStringAt(16,msg);
}

#if 0
void showPenalty(int penalty)
{
	char msg[30];
	if (noGraphics) return;
	SetFillStyle(CLEAR_AREA);
	bar(CLEARLEFT,ROWPOS(17)-textHeight-2,X,ROWPOS(17)+2);
	sprintf(msg,"Penalty %d", penalty);
	putStringAt(17,msg);
	getch();
}
#endif

static int getInput(void)
{
	int c;
	if (hasmouse)
		ShowMouse();
	for (; !kbhit(); )
	{
		if (hasmouse)
		{
			if (ButtonUp(0))
				c = 1;
			else if (ButtonUp(1))
				c = 13;
			else continue;
			MousePos(&mousex, &mousey);
			if (mousex > TEXTLEFT)
			{
				mouserow = (mousey-12) / (Y / 25) + 1;
				mousecol = (mousex - TEXTLEFT) / textwidth("x"); // hackish
				mousepos = TEXTAREA;
			}
			else if (mousey>Yinc && mousex>Xinc && mousey<(Yinc *(H+1)))
				// board square
			{
				mouserow = mousey / Yinc - 1;
				mousecol = mousex / Xinc - 1;
				mousechar = screenLetters[mouserow+1][mousecol+1]&0x3F;
				mousepos = BOARDAREA;
			}
			else if (mousey > (Yinc*(H+1)+10) && mousey < (Yinc*(H+2)+10) &&
				mousex > Xinc && mousex < (Xinc * (numtiles+1)))
				// rack letter?
			{
				mousechar = Pos2RackTile(mousex / Xinc);
				if (mousechar>=0) mousepos = RACKAREA;
			}
			else c = 27;
			break;
		}
	}
	if (kbhit())
		c = getch();
	if (hasmouse) HideMouse();
	return c;
}

int getUserUpdate(int row, char *prompt, char *buff, int len, int hlp)
{
	int plen, pos, c, prtPos;
	if (noGraphics) return 0;
	putStringAt(row,prompt);
	pos = (int)strlen(buff);
	plen = TEXTLEFT+textwidth(prompt);
	for (;;pos++)
	{
	    retry:
		SetFillStyle(CLEAR_AREA);
		bar(plen,ROWPOS(row)-textHeight-2,X,Y);
		prtPos = 0;
		while ((textwidth(&buff[prtPos])+plen) > X) prtPos++;
		putStringAtRC(row,plen,&buff[prtPos]);
		c = getInput();
		if (c==1)
			if (mousepos == RACKAREA || mousepos == BOARDAREA)
				c = mousechar + 'A';
		if (c==27)
		{
			buff[0]=0;
			if (pos==0) return -1; // escape
			else pos=-1;
		}
		else if (c==8 && pos>0)
		{ // backspace
			pos--;
			buff[pos]=0;
		 	pos--;
		}
		else if (c==0)
		{
			if (getch()==59)
				showHelp(hlp);
			goto retry;
		}
		else if (c<32)
		{
			buff[pos] = 0;
			break;
		}
		else
		{
			buff[pos] = (char)c; buff[pos+1] = 0;
			if (pos>=len) break;
		}
	}
	return pos;
}

int getUserInput(int row, char *prompt, char *buff, int len, int hlp)
{
	buff[0]=0;
	return getUserUpdate(row,prompt,buff,len,hlp);
}

int matchLine;

int getUserChar(int row, char *msg, int hlp)
{
	int c;
	char val[2];
	putStringAt(row,msg);
retry:
	c = getInput();
	if (c==0)
	{
		if (getch()==59) showHelp(hlp);
		goto retry;
	}
	val[0]=(char)c;
	val[1]=0;
	if (c>=32) putStringAtRC(row,TEXTLEFT+textwidth(msg),val);
	return c;
}

int showMatch(char *word)
{
	int c = 0;
	if (noGraphics) return 0;
	putStringAt(matchLine++,word);
	if (matchLine==24)
	{
		if (getUserChar(24,"  Press any key...",HLP_ANYKEY)==27) c=1;
		clearMessageArea();
		matchLine = 17;
	}
	return c;
}

int getAxis(int isCol, int row, char *prompt, int *res, int Max, int hlp)
{
	int c = getUserChar(row,prompt,hlp);
	if (c==1 && mousepos == BOARDAREA)
	{
		char val[2];
		val[1]=0;
		c = isCol ? mousecol : mouserow;
		val[0]=(char)c+'A';
		putStringAtRC(row,TEXTLEFT+textwidth(prompt), val);
	}
	else
		c -= (c<'a') ? 'A': 'a';
	*res = c+1;
	return (c>=0 && c<Max);
}

void getKeyPress(void)
{
	clearMessageArea();
	(void)getUserChar(18,"  Press any key...",HLP_ANYKEY);
	clearMessageArea();
}

static char Pattern[80]={0};

int getUserMove(int *first)
{
	char c, msg[32], word[MAXBOARD+1];
	int rtn, row, col, status;
	int doneXchange=0, gotHints=0;
	char *prompt = "D(own),A(cross),P(ass)? ";
retry:;
	clearMessageArea();
	msg[1] = 0;
	putStringAt(16,exchangeAllowed?"N(ext),X(chg),E(xclude)":"N(ext),E(xclude)");
	putStringAt(17,"C(onsult),W(atch),V(iew),");
	putStringAt(18,"H(int),U(ndo),R(edo),");
	putStringAt(19,"L(oad),S(ave),Q(uit)");
	c = (char)getUserChar(20, prompt, HLP_MOVE);
	if (c==27 || c==2) goto retry;
	if (c==1 && mousepos == TEXTAREA)
	{
		switch(mouserow)
		{
		case 16:
			if (mousecol < 6) c = 'N';
			else if (exchangeAllowed != NOXCHANGE)
			{
				if (mousecol < 12) c = 'X';
				else if (mousecol < 20) c = 'E';
			}
			else if (mousecol < 14) c = 'E';
			break;
		case 17:
			if (mousecol < 10) c = 'C';
			else if (mousecol < 18) c = 'W';
			else if (mousecol < 24) c = 'V';
			break;
		case 18:
			if (mousecol < 7) c = 'H';
			else if (mousecol < 14) c = 'U';
			else if (mousecol < 21) c = 'R';
			break;
		case 19:
			if (mousecol < 7) c = 'L';
			else if (mousecol < 14) c = 'S';
			else if (mousecol < 21) c = 'Q';
			break;
		case 20:
			if (mousecol < 7) c = 'D';
			else if (mousecol < 16) c = 'A';
			else if (mousecol < 23) c = 'P';
			break;
		default:
			goto retry;
		}
		msg[0] = c;
		putStringAtRC(20,TEXTLEFT+textwidth(prompt), msg);
	}
	if (c>'Z') c -= ('z' - 'Z');
	switch (c)
	{
	case 'V':
		showCompTiles = 1 - showCompTiles;
		DisplayBoard(SLOW);
		break;
	case 'W':
		return WATCH;
	case 'X': // swap blank tile
		if (exchangeAllowed != NOXCHANGE)
		{
			int R, C;
			c = (char)getUserChar(21,prompt = "Letter? ",HLP_XLETTER);
			if (c==1 && mousepos == RACKAREA)
			{
				c = mousechar + 'A';
				msg[0] = c;
				putStringAtRC(21,TEXTLEFT+textwidth(prompt), msg);
			}
		  	if (c>='a') c-='a'-'A';
		  	if (c<'A' || c>'Z')
				showInfo(1,"Huh?");
		  	else if (!GetTileCount(player,c))
				showInfo(1,"You don't have it!");
		  	else if (FindWildTile(c, &R, &C))
			{
				doSwap(R,C,c-'A',*first);
				doneXchange = 1;
				if (exchangeAllowed==XCHANGEONLY)
					return PLAYEDMOVE; // that's all
				gotHints = 0;
		  	}
			else showInfo(1,"No such blank!");
		}
		break;
	case 'E': /* exclude */
#ifdef DEMO_VERSION
		showInfo(1,"Register first!");
		break;
#else
		if (getUserUpdate(23,"Word? ",lastCompWord, MAXBOARD+1,HLP_EXWORD)<=0)
			goto retry;
		exclude(lastCompWord);
		// fall thru...
#endif
	case 'N': // play computers next move
		if (doneXchange)
			showInfo(1,"Too late to undo!");
		else if (!computedMoves)
			showInfo(1,"No other moves!");
		else if (undo1Move(0,first))
			showInfo(1,"Come again?");
		else
		{
			if (gotHints || c=='E')
			{
				findMove(*first); /* regenerate moves */
				gotHints = 0;
			}
			return PLAYNEXT;
		}
		break;
	case 'C':
		{
			int anagramMatch, allLengths, repeats;
			abortMatch = 0;
			matchLine = 17;
			c = (char)getUserChar(21,prompt = "O(rder) or A(nagram)? ",HLP_ANAGRAM);
			if (c==1 && mousepos == TEXTAREA && mouserow == 21)
			{
				if (mousecol < 7) c = 'O';
				else if (mousecol > 9 && mousecol < 18) c = 'A';
				msg[0] = c;
				if (c!=1) putStringAtRC(21,TEXTLEFT+textwidth(prompt), msg);
			}
			if (c=='a') anagramMatch = repeats = 1;
			else if (c=='A') { anagramMatch=1; repeats = 0; }
			else if (c!='O' && c!='o') goto retry;
			else { anagramMatch = 0; repeats = 1; }
			c = (char)getUserChar(22,"Use A(ll) or S(ome)? ",HLP_SOME);
			if (c==1 && mousepos == TEXTAREA && mouserow == 22)
			{
				if (mousecol > 4 && mousecol < 9) c = 'A';
				else if (mousecol > 13 && mousecol < 19) c = 'S';
				msg[0] = c;
				if (c!=1) putStringAtRC(22,TEXTLEFT+textwidth(prompt), msg);
			}
			if (c=='a' || c=='A') allLengths=0;
			else if (c!='S' && c!='s') goto retry;
			else allLengths = 1;
			if (allLengths && !anagramMatch)
			{
				c = (char)getUserChar(23,"From F(ront) or B(ack)? ",HLP_DIRECT);
				if (c==1 && mousepos == TEXTAREA && mouserow == 23)
				{
					if (mousecol > 5 && mousecol < 12) c = 'F';
					else if (mousecol > 16 && mousecol < 20) c = 'B';
					msg[0] = c;
					if (c!=1) putStringAtRC(23,TEXTLEFT+textwidth(prompt), msg);
				}
				if (c=='b' || c=='B') allLengths = 2;
				else if (c!='f' && c!='F') goto retry;
			}
			if (getUserUpdate(24,"Pattern? ",Pattern,79,HLP_PATTERN)>0)
			{
				int l=(int)strlen(Pattern);
				(void)strupr(Pattern);
				clearMessageArea();
				Pattern[l]=0;
				matchPattern(Pattern,anagramMatch,allLengths,repeats,0);
				if (matchLine != 17)
					(void)getUserChar(24,"  Press any key..",HLP_ANYKEY);
				clearMessageArea();
			}
			break;
		}
	case 'P':
		{
			int l, i, f;
			if (tilesleft)
			{
				l = getUserInput(22,"Drop? ",word,MAXBOARD, HLP_DROP);
				if (l<0) goto retry;
				if (l>0) (void)strupr(word);
			}
			else l=0;
			f = 0;
			for (i=0; i<l && !f; i++)
			{
				if (word[i]=='_') word[i]='Z'+1;
				if (GetTileCount(player,word[i])) useLetter(player,word[i]);
				else
				{
					showInfo(1,"Sorry! Try again...");
					putchar(7);
					sleep(2);
					while (i--) replaceLetter(player,word[i]);
					goto retry;
				}
			}
			// Used all letters - return them to heap
			for (i=0;i<l;i++)
				addToHeap(word[i],1,900+i);
			recordDrop(player,word,moves,*first);
			return PASS;
		}
	case 'A':
	case 'D': 
		if (exchangeAllowed!=XCHANGEANY && doneXchange)
		{
			showInfo(1,"You must pass!");
			break;
		}
		isAcross = (c=='a' || c=='A');
		if (*first)
		{
			int ok=1;
			if (isAcross)
			{
				if (firstMoveDir==1)
					ok=0;
			}
			else if (firstMoveDir==2)
				ok=0;
			if (!ok)
			{
				char buff[24];
				sprintf(buff,"Can't play %s!",isAcross?"across":"down");
				putStringAt(23,buff);
				putchar(7); sleep(2);
				break;
			}
		}
		if (getAxis(0, 21,"Row? ",&row,Rows,HLP_RLABEL)==0)
			break;
#if DEBUG
		fprintf(debug,"User plays row %d\n",row);
#endif
		if (getAxis(1, 22,"Col? ",&col,Cols,HLP_CLABEL)==0) 
			break;
#if DEBUG
		fprintf(debug,"User plays row %d, col %d\n",row,col);
#endif
		if (getUserInput(23,"Word? ",word,MAXBOARD,HLP_WORD)<=0)
			break;
playmove:
#if DEBUG
		fprintf(debug,"Checking move, row %d, col %d, word %s\n",row,col,word);
#endif
		switch(checkMove(word,row,col,*first,&status))
		{
		case MOVEOK:
			putStringAt(24,"Word OK!");
			sleep(1);
			clearMessageArea();
			return PLAYEDMOVE;
		case NOTINROW:
			sprintf(msg,"1st word must cross row %d!",firstMoveRow);
			putStringAt(24,msg);
			break;
		case NOTINCOL:
			sprintf(msg,"1st word must cross col %d!",firstMoveCol);
			putStringAt(24,msg);
			break;
		case NOTINDICT: // already handled
			break;
		case NOANCHOR:
			putStringAt(24,"Word has no anchor square!");
			break;
		case CONFLICT:
			putStringAt(24,"Word conflicts with board!");
			break;
		case NOBLANK:
			putStringAt(24,"You don't have a blank!");
			break;
		case NOLETTER:
			sprintf(msg,"You don't have a [%c]!",status);
			putStringAt(24,msg);
			break;
		case BADCROSS:
			// bad cross word; already done
			break;
		case BADEND:
			putStringAt(24,"Can't end there!");
			break;
		case BADSTART:
			putStringAt(24,"Can't start there!");
			break;
		}
		putchar(7); sleep(2);
		break;
	case 'Q': /* quit */
		c = (char)getUserChar(24,"QUIT: are you sure?",HLP_QUIT);
		if (c=='y'||c=='Y'|| (c==1 && mousepos == TEXTAREA && mouserow==24))
		{
			shutdown();
			return RESTART; // restart
		}
		break;
	case 'U':
		if (doneXchange)
		{
			player = 1 - player;
			c = (char)undo1Move(1,first);
			doneXchange = 0;
		} else
		{
			if (undo2Moves(first))
				showInfo(1,"Couldn't undo!");
			else return UNDONE;
		}
		break;
	case 'R':
		if (doRedo(player)==0) goto noRedo;
		DisplayBoard(SLOW);
		if (*first && moveRecNow[player]->action!='P')
			*first = 0;
		player = 1-player;
		moves++;
		if (doRedo(player))
		{
			if (*first && moveRecNow[player]->action!='P')
				*first = 0;
		}
		else
		{
			player=1-player;
			moves--;
		}
		return REDO;
	noRedo: showInfo(1,"Nothing to redo");
		break;
	case 'H': /* hint */
	nexthint:
		if (!gotHints)
		{
			findMove(*first);
			if (numHints==0) goto retry; // shouldn't happen
			gotHints = 1;
			lastShownHint = numHints;
		}
		if (lastShownHint==0) lastShownHint = numHints;
		lastShownHint--;
		if (moveList[lastShownHint].score == 0)
		{
			showInfo(1,"Pass");
			lastShownHint++; // patch around a bug
		} else
		{
			char c;
			sprintf(msg,"%c %c %c %s",moveList[lastShownHint].isAcross?'A':'D',
				moveList[lastShownHint].r+'A'-1,
				moveList[lastShownHint].c+'A'-1,
				moveList[lastShownHint].word);
			clearInfoArea();
			putStringAt(21,msg);
			sprintf(msg,"Score: %d",moveList[lastShownHint].score);
			putStringAt(22,msg);
			c = (char)getUserChar(24,"Play it (ynh)?",HLP_HINT);
			if (c==1 && mousepos == TEXTAREA && mouserow == 24)
			{
				if (mousecol == 9) c = 'y';
				else if (mousecol == 10) c = 'n';
				else if (mousecol == 11) c = 'h';
			}
			clearInfoArea();
		/*sprintf(msg,"Mousecol: %d",mousecol);
		putStringAt(22,msg);*/
			if (c=='h' || c=='H') goto nexthint;
			if (c!='y'&&c!='Y') goto retry;
			row = moveList[lastShownHint].r;
			col = moveList[lastShownHint].c;
			strcpy(word,moveList[lastShownHint].word);
			isAcross = moveList[lastShownHint].isAcross;
			goto playmove;
		}
		break;
	case 13:
	case 10:
		break;
	default:
		putStringAt(23,"Option not available");
		putchar(7);
		sleep(2);
		break;
	case 'L': /* load */
#ifdef DEMO_VERSION
		showInfo(1,"Register first!");
		if (Zippy)
		{
			if (loadGameFile("zippy",1,first)!=0)
			{
				player = 1-player;
				goto retry;
		  	}
		  	player = 1-player;
		 	return LOADEDNEW;
		}
		break;
#else
		{
			int rtn;
			if (getUserInput(23,"File name? ",msg,14,HLP_LFNAME)<=0)
				goto retry;
			if (loadGameFile(msg,1,first)!=0)
			{
				player = 1-player;
				goto retry;
		  	}
		  	player = 1-player;
		 	return LOADEDNEW;
		}
#endif
	case 'S': /* save */
#ifdef DEMO_VERSION
		showInfo(1,"Register first!");
		if (Zippy) saveGame("zippy");
#else
		if (getUserInput(23,"File name? ",msg,14,HLP_SFNAME)>0)
		{
			saveGame(msg);
			showInfo(1,"Game saved");
		}
#endif
		break;
	}
	goto retry;
	return 0;
}

static void AdjustRack(char *letters, int who)
{
	int i;
	/* toss what is there */
	for (i = 'A'; i <= BLANKVAL; i++)
	{
		int n = GetTileCount(who, i);
		if (n>0)
		{
			takeFromRack(who, i, n, 924);
			addToHeap(i, n, 923);
		}
	}
	while (*letters)
	{
		char newch = *letters++;
		if (newch>='A' && newch <= 'Z')
		{
	 		if (numtiles > 0 && PoolCnt[newch-'A']>0)
	 		{
	 			takeFromHeap(newch,1,925);
				addToRack(who,newch,1,926);
	 		}
		}
		else
		{
			if (numtiles > 0 && PoolCnt[BLANK]>0)
			{
				takeFromHeap(BLANKVAL,1,927);
				addToRack(who,BLANKVAL,1,928);
			}
		}
	}
}

static void AdjustBoard(int row, int col, char *letters, int isacross)
{
	while (*letters)
	{
		char old = boardLetters[row][col];
		char oldch = (old & 0x1F);
		char newch = *letters++;
		if (row<1 || col<1 || row>Rows || col>Cols)
			break;
		/* replace any existing tiles back to pool */
		if (HasTile(row,col))
			if (old & 0x80)
				addToHeap(BLANKVAL, 1, 927);
			else
				addToHeap(oldch+'A', 1, 927);
		if (oldch != BLACKSQ) /* skip black squares */
		{
			if (newch>='A' && newch <= 'Z')
			{
				if (numtiles > 0 && PoolCnt[newch-'A']>0)
				{
					takeFromHeap(newch,1,925);
					boardLetters[row][col] = newch - 'A';
				}
			}
			else
			{
				if (numtiles > 0 && PoolCnt[BLANK]>0)
				{
					takeFromHeap(BLANKVAL,1,925);
					boardLetters[row][col] = 0x80 | (newch-'a');
				}
			}
		}
		if (isacross) col++;
		else row++;
	}
}

static void RecordNewStartup(void)
{
	int i, j;
	for (i=1;i<=MAXBOARD;i++) 
		for (j=1;j<=MAXBOARD;j++)
			initBoardLetters[i][j] = boardLetters[i][j];
	for (i=0;i<27;i++)
	{
		Tiles[COMPUTER+2][i] = Tiles[COMPUTER][i];
		Tiles[HUMAN+2][i] = Tiles[HUMAN][i];
	}
}

void EditBoard(void)
{
	char c, msg[32], word[MAXBOARD+1];
	int row, col, direction, rack;
	int oldshow = showCompTiles;
	char *prompt = "D(own),A(cross),Q(uit)? ";
	showCompTiles = 1;
retry:;
	DisplayBoard(FAST);
	clearMessageArea();
	msg[1] = 0;
	putStringAt(19,"H(uman),C(omputer),");
	c = (char)getUserChar(20, prompt, HLP_MOVE);
	if (c==27 || c==2) goto retry;
	if (c==1 && mousepos == TEXTAREA)
	{
		switch(mouserow)
		{
		case 19:
			if (mousecol < 8) c = 'H';
			else if (mousecol < 19) c = 'C';
			break;
		case 20:
			if (mousecol < 7) c = 'D';
			else if (mousecol < 16) c = 'A';
			else if (mousecol < 23) c = 'Q';
			break;
		default:
			goto retry;
		}
		msg[0] = c;
		putStringAtRC(20,TEXTLEFT+textwidth(prompt), msg);
	}
	if (c>'Z') c -= ('z' - 'Z');
	switch (c)
	{
	case 'Q':
		showCompTiles = oldshow;
		RecordNewStartup();
		return;
	case 'H':
	case 'C':
		if (getUserInput(23,"Letters? ",word,MAXBOARD,HLP_WORD)<=0)
			break;
		AdjustRack(word,(c=='h' || c=='H')?HUMAN:COMPUTER);
		break;
	case 'A':
	case 'D': 
		if (getAxis(0, 21,"Row? ",&row,Rows,HLP_RLABEL)==0)
			break;
		if (getAxis(1, 22,"Col? ",&col,Cols,HLP_CLABEL)==0) 
			break;
		if (getUserInput(23,"Letters? ",word,MAXBOARD,HLP_WORD)<=0)
			break;
		AdjustBoard(row,col,word,(c=='a' || c=='A'));
		DisplayBoard(FAST);
		break;
	case 13:
	case 10:
		break;
	default:
		putStringAt(23,"Option not available");
		putchar(7);
		sleep(2);
		break;
	}
	goto retry;
}

int override2(char *msg)
{
	int rtn ;
	clearMessageArea();
	putStringAt(22,msg);
	putStringAt(23,"Do it anyway? (y/n)");
	rtn = getch();
	if (rtn=='y' || rtn=='Y') return 1;
	else return 0;
}

int override(char *msg)
{
#ifdef DEMO_VERSION
	clearMessageArea();
	putStringAt(22,msg);
	sleep(2);
	return 0;
#else
	return override2(msg);
#endif
}

int sayBye(void)
{
	int c = 0;
	if (noGraphics) return 0;
	if (strategyTest==FASTTEST) goto done;
retry:
	clearMessageArea();
	putStringAt(20,"Press S to save,");
	putStringAt(21,"      Q to quit,");
	putStringAt(23,"or any other key");
	putStringAt(24,"to play again...");
	c = getch();
	if (c=='s' || c=='S')
#ifdef DEMO_VERSION
	if (!Zippy) showInfo(1,"Register first!");
	else
#endif
	{
		char name[16];
		clearMessageArea();
		if (getUserInput(23,"File name? ",name,14,HLP_SFNAME)<=0)
			goto retry;
		saveGame(name);
		showInfo(1,"Game saved");
		goto retry;
	}
done:
	if (inGrafxMode)
	{
		closegraph();
		textmode((int)C80);
		inGrafxMode = 0;
	}
	return (c=='q'||c=='Q'||c==3);
}

void dawgLoadFail(void)
{
	extern char dawgError[];
	if (inGrafxMode)
	{
		closegraph();
		textmode((int)C80);
	}
	fprintf(stderr,"Cannot load dictionary!\n%s\nAborting\n", dawgError);
	exit(-1);
}

typedef char *HelpList[];

static HelpList PatternHelp =
{
	"Please enter a pattern to",
	"look up in the dictionary.",
	"",
	"See the WW.DOC file for an",
	"explanation of patterns.",
	NULL
};

static HelpList DropHelp =
{
	"Enter a list of letters",
	"that you want to put",
	"back in the pool. Use",
	"`_' for a blank.",
	NULL
};

static HelpList WordHelp =
{
	"Type the word you are",
	"playing in upper case,",
	"and press ENTER. If you",
	"use a blank tile, enter",
	"the letter it represents,",
	"in LOWER CASE.",
	NULL
};

static HelpList LFNameHelp =
{
	"Enter the name of",
	"the file you used",
	"when you saved the",
	"game you wish to",
	"restore, and press",
	"ENTER.",
	NULL
};

static HelpList SFNameHelp =
{
	"Enter the name of",
	"the file you want",
	"to save the game",
	"in, and press",
	"ENTER.",
	NULL
};

static HelpList RLabelHelp =
{
	"Type the letter that",
	"corresponds to the row",
	"at which your word",
	"starts. The row labels",
	"are shown on the left",
	"side of the board.",
	NULL
};

static HelpList CLabelHelp =
{
	"Type the letter that",
	"corresponds to the",
	"column at which your",
	"word starts. The column",
	"labels are shown along",
	"the top of the board.",
	NULL
};

static HelpList MoveHelp =
{
	"Press the letter of",
	"the move option you",
	"want.",
	NULL
};

static HelpList AnagramHelp =
{
	"If you specify Anagram,",
	"all orderings of the",
	"pattern will be matched",
	"otherwise only the ",
	"order you specify",
	"will be used. `a' finds",
	"all anagrams; `A' finds",
	"less but avoids duplicates",
	NULL
};

static HelpList SomeHelp =
{
	"If you specify All,",
	"only words that use",
	"all the letters in the",
	"pattern will be listed,",
	"otherwise shorter words",
	"will be listed too.",
	NULL
};

static HelpList QuitHelp =
{
	"Press `y' if you",
	"want to go back to",
	"DOS, or `n' if you",
	"want to keep playing.",
	NULL
};

static HelpList HintHelp =
{
	"Press `y' if you want",
	"this move played for",
	"you automatically,",
	"`h' for another hint,",
	"or `n' to continue.",
	NULL
};

static HelpList XChangeHelp =
{
	"You can swap one of",
	"your tiles for a blank",
	"on the board, if there",
	"is one with that letter.",
	NULL
};

static HelpList XLtrHelp =
{
	"Enter the letter",
	"that you are swapping",
	"with a blank.",
	NULL
};

static HelpList ExWrdHelp =
{
	"Enter the word that",
	"you want excluded",
	"from play.",
	NULL
};

static HelpList DirectHelp =
{
	"Press F to match",
	"prefixes and B to",
	"match suffixes.",
	NULL
};

static HelpList *HlpEntries[] =
{
	&PatternHelp,
	&DropHelp,
	&WordHelp,
	&LFNameHelp,
	&SFNameHelp,
	&RLabelHelp,
	&CLabelHelp,
	&MoveHelp,
	&AnagramHelp,
	&SomeHelp,
	NULL,
	&QuitHelp,
	&HintHelp,
	&XChangeHelp,
	&XLtrHelp,
	&ExWrdHelp,
	&DirectHelp
};

static void showHelp(int hlpindex)
{
	int r = 1;
	HelpList *h = HlpEntries[hlpindex];
	if (h==NULL) return;
	SetFillStyle(CLEAR_AREA);
	bar(CLEARLEFT,1,X,CLEARTOP);
	while ((*h)[r-1]!= NULL)
	{
		putStringAt(r,(*h)[r-1]);
		r++;
	}
	putStringAt(r,"Press any key...");
	(void)getch();
	bar(CLEARLEFT,1,X,CLEARTOP);
	printTitle();
}

void printIntro(void)
{
	char buff[80];
	clrscr();
#ifdef DEMO_VERSION
	sprintf(buff,"WordsWorth v%s Serial #%s UNREGISTERED DEMO COPY\n",
		VERSION,SERIAL);
	puts(buff);
	puts("This software is fully copyrighted and may not be hired out or sold");
	puts("without the express permission of the author. This restricted version may be");
	puts("freely copied and given away provided this is in its complete form with");
	puts("all original files intact and unmodified.\n");
	puts("For details on how to order the full version of WordsWorth by becoming");
	puts("a registered user, run the program with a -R argument. This will create");
	puts("a file called REGISTER.DOC with further details. The full version has");
	puts("most of the delays removed, uses a large dictionary, allows you to");
	puts("save and load your games, force the program to play a different");
	puts("move, and consult and override the dictionary.\n");
	if (!Zippy) sleep(10);
	puts("If you have read and understood the conditions above and");
	puts("agree to them, press ENTER or RETURN to begin playing...\n");
	puts("Read the file WW.DOC for instructions on how to play.");
	while (!kbhit());
	(void)getchar();
#else
	sprintf(buff,"WordsWorth v%s Serial #%s Registered to: %s\n",
		VERSION,SERIAL,Register);
	puts(buff);
	puts("\n(c) Graham Wheeler 1994. All Rights Reserved.\n");
	sleep(1);
#endif
}

void showCopyright(void)
{
	int i;
	static int first = 1;
	if (noGraphics) return;
	i = -1;
	if (first)
	{
		do {
			i++;
			Register[i] -= (char)0x80;
		}
		while (Register[i]);
		first=0;
	}
	printIntro();
}

int endGameMessage(void)
{
	int p, i, rtn, pen[2];
	extern int stalemate;
	rtn = sayBye();
	if (moves>1)
	{
		printf("Score before penalties: me %d   %s %d\n",
			Score[COMPUTER],strategyTest?"control":"you",Score[HUMAN]);
		printf("Average score per move: me %d   %s %d\n",
			Score[COMPUTER]/(moves/2),
			strategyTest?"control":"you", Score[HUMAN] / (moves/2));
		pen[0] = pen[1] = 0;
		if (numtiles>1)
		{
			for (p=0;p<2;p++)
			{
				for (i=0;i<27;i++)
					if (Tiles[p][i])
						pen[p] += Tiles[p][i] * Scores[i];
				Score[p] -= pen[p];
			}
		}
		printf("Penalties: me %d   %s %d\n",
			pen[COMPUTER],strategyTest?"control":"you",pen[HUMAN]);
		for (p=0;p<2;p++) 
			if (Score[p]<0) Score[p]=0;
		printf("Final result: me %d  %s %d  %s\n%s\nThanks for playing WordsWorth %s!\n",
			Score[COMPUTER],strategyTest?"control":"you",Score[HUMAN],
			(stalemate==3?"(both stuck)":""),
			(Score[COMPUTER]>Score[HUMAN] ? "I won":
			(Score[COMPUTER]<Score[HUMAN]?(strategyTest?"Control won":"You won"):
			"We drew")),VERSION);
	}
#ifdef DEMO_VERSION
	puts("Support shareware - please register your copy of WordsWorth!\n");
	if (moves>1 && !Zippy) sleep(5);
#endif
	if (!rtn && strategyTest!=FASTTEST)
	{
		puts("Press a key to continue...\n");
		(void)getch();
	}
	return rtn;
}

void makeRegInfo(void)
{
#ifdef DEMO_VERSION
	FILE *fp = fopen("REGISTER.DOC", "wt");
	if (fp==NULL) return;
	fputs("WordsWorth Registration\n", fp);
	fputs("=======================\n", fp);
	fputs("\n", fp);
	fputs("This version of WordsWorth is a restricted unregistered version.\n", fp);
	fputs("It uses a US English dictionary of about 17000 words, which contains\n", fp);
	fputs("only words of six letters or less, and you cannot override the\n", fp);
	fputs("dictionary.\n\n", fp);
	fputs("If you wish to obtain the unrestricted version, you must\n", fp);
	fputs("register your copy. When you register, you will receive a\n", fp);
	fputs("new copy of WordsWorth, together with an 80,000+ word \n", fp);
	fputs("dictionary. The registered version plays at almost the \n", fp);
	fputs("same speed as this version (the only really noticeable \n", fp);
	fputs("difference on my 486 is the time taken to load the dictionary),\n", fp);
	fputs("allows you to override the dictionary, and eliminates the \n", fp);
	fputs("annoying delays, as well as enabling Save/Load/Consult and Next.\n\n", fp);
	fputs("To register, please send an international bank draft or money\n", fp);
	fputs("order for R50 (South African Rands) (foreign users: US $25 or\n", fp);
	fputs("20 Pounds Sterling).\n\n", fp);
	fputs("Send your registration fee to:\n\n", fp);
	fputs("	Graham Wheeler\n	P.O.Box 6680\n	Roggebaai\n", fp);
	fputs("	Cape Town 8012 \n	South Africa\n\n", fp);
	fputs("together with your name and address, and the media you require \n", fp);
	fputs("(1 x 5.25\" DSHD or 1 x 3.5\" DSHD).\n", fp);
	fclose(fp);
	exit(0);
#endif
}

/******************/
/* Mouse handling */
/******************/

// Mouse driver service request IDs

#define RESET_MOUSE          	0
#define SHOW_MOUSE           	1
#define HIDE_MOUSE           	2
#define GET_MOUSE_STATUS 	3
#define PRESSED         	5     /* Button presses */
#define RELEASED        	6     /* Button releases */
#define SETCURSOR		9

unsigned arrowcursor[34] = // mask and hots spots x,y at end
{
	0x3fff,	0x1fff,	0xfff,	0x7ff,	0x3ff,	0x1ff,	0xff,	0x7f,
	0x3f,	0x1f,	0x1ff,	0x10ff,	0x30ff,	0xf87f,	0xf87f,	0xfc3f,
        0x0,	0x4000,	0x6000,	0x7000,	0x7800,	0x7c00,	0x7e00,	0x7f00,
	0x7f80,	0x7fc0,	0x7c00,	0x4600,	0x600,	0x300,	0x300,	0x180,
	1,	1
};

unsigned hourglasscursor[34] =
{
	0xc003,	0xc003,	0xc003,	0xc003,	0xc003,	0xe007,	0xf00f,	0xf81f,
	0xf81f,	0xf00f,	0xe007,	0xc003,	0xc003,	0xc003,	0xc003,	0xc003,
	0x0,	0x1ff8,	0x1008,	0x1008,	0x810,	0x5a0,	0x7e0,	0x3c0,
	0x240,	0x420,	0x990,	0xbd0,	0x13c8,	0x17e8,	0x1ff8,	0x0,
	8,	8

};

static int hidden = 0;

static void MouseService(int *m1, int *m2, int *m3, int *m4, int es)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	if (m1) inregs.x.ax = *m1;
	if (m2) inregs.x.bx = *m2;
	if (m3) inregs.x.cx = *m3;
	if (m4) inregs.x.dx = *m4;
	if (es)
	{
		sregs.ds = _DS;
		sregs.es = es;
		int86x(0x33,&inregs,&outregs,&sregs);
	}
	else int86(0x33,&inregs,&outregs);
	if (m1) *m1 = outregs.x.ax;
	if (m2) *m2 = outregs.x.bx;
	if (m3) *m3 = outregs.x.cx;
	if (m4) *m4 = outregs.x.dx;
}

static int ResetMouse(void)
{
	int m1 = RESET_MOUSE;
	MouseService(&m1, NULL, NULL, NULL, 0);
	return m1;
}

static void SetMouseCursor(int hotx, int hoty, unsigned *masks)
{
	int seg = FP_SEG(masks), off = FP_OFF(masks), svc = SETCURSOR;
	MouseService(&svc, &hotx, &hoty, &off, seg);
}

void MouseCursor(int Wait)
{
	if (Wait) SetMouseCursor(hourglasscursor[32], hourglasscursor[33],
				hourglasscursor);
	else SetMouseCursor(arrowcursor[32], arrowcursor[33], arrowcursor);
}

void ShowMouse(void)
{
	if (--hidden <= 0)
	{
		int m1 = SHOW_MOUSE;
		MouseService(&m1, NULL, NULL, NULL, 0);
		hidden = 0;
	}
}

void HideMouse(void)
{
	if (!hidden)
	{
		int m1 = HIDE_MOUSE;
		MouseService(&m1, NULL, NULL, NULL, 0);
	}
	hidden++;
}

int ButtonUp(int b)
{
	int m1 = RELEASED;
	MouseService(&m1, &b, NULL, NULL, 0);
	if (b) return 1;
	else return 0;
}

void MousePos(int *x, int *y)
{
	int m1 = GET_MOUSE_STATUS;
	MouseService(&m1, NULL, x, y, 0);
	// Adjust for virtual coordinates of the mouse
	if (getmaxx() == 319) *x /= 2;
}

int InitMouse(void)
{
	if (ResetMouse())
	{
		/* Hercules card requires an extra step */
		if (getgraphmode() == HERCMONOHI)
		{
			char far *memory = (char far *)0x004000049L;
			*memory = 0x06;
			(void)ResetMouse();
		}
		return 1;
	}
	else return 0; /* Mouse not found! */
}

