#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOBAL extern

#include "dict.h"

#include "ww.h"

extern short ColorMap[], TextureMap[];
static char inc_name[14], *fnamenow;

/* Types */

#define STRING		1
#define CHAR		2
#define SHORT		4	
#define COLOR		8
#define VIDCARD		16
#define TEXTURE		32
#define BASE_MASK	(TEXTURE*2-1)
#define UNSIGNED	64
#define TABLEOF		128
#define USE_PTRS	256
#define INCLUDE_FILE	512

static char *ColourNames[] = {
	"Black",
	"Blue",
	"Green",
	"Cyan",
	"Red",
	"Magenta",
	"Brown",
	"LightGray",
	"DarkGray",
	"LightBlue",
	"LightGreen",
	"LightCyan",
	"LightRed",
	"LightMagenta",
	"Yellow",
	"White",
	NULL
};

static char *TextureNames[] = {
	"Empty",
	"Solid",
	"Line",
	"ThinSlash",
	"ThickSlash",
	"ThickBackslash",
	"ThinBackslash",
	"ThinHatch",
	"ThickHatch",
	"InterleavingLine",
	"WideSpacedDots",
	"CloseSpacedDots",
	NULL
};

char *VCardNames[] = {
	"Autodetect",
	"Hercules",
	"EGA",
	"VGA",
	"SuperVGA",
	NULL,
};

typedef struct {
	char *name;	// symbolic name
	ushort type;	// element type
	void *loc;	// pointer to value or start of value array
	short *dimr, *dimc; // pointers to dimensions for tables, or
			// allowed range of values.
} cfgEntry;

cfgEntry WWCfg[] = {
{ "Include", INCLUDE_FILE|STRING, (void *)inc_name, 0, (short *)12 },
{ "Dictionary", STRING, (void *)dictName, 0, (short *)12 },
{ "Rows", UNSIGNED|SHORT,(void *)&Rows, (short*)7, (short*)MAXBOARD },
{ "Columns", UNSIGNED|SHORT,(void *)&Cols, (short*)7, (short*)MAXBOARD },
{ "NumberOfTiles", UNSIGNED|SHORT,(void *)&numtiles, (short*)3, (short*)MAXTILES },
{ "PlayAllTileBonusPoints", UNSIGNED|SHORT,(void *)&bonus, 0, 0 },

{ "LetterMarks",TABLEOF|CHAR,(void *)LetterMark, 0, (short*)10 },
{ "LetterMarkColors",TABLEOF|COLOR,(void *)LetterMarkColor, 0, (short*)10 },
{ "LetterMarkBColors",TABLEOF|COLOR,(void *)LetterMarkBColor, 0, (short*)10 },
{ "LetterMarkTextures",TABLEOF|TEXTURE,(void *)LetterMarkTexture, 0, (short*)10 },

{ "WordMarks",TABLEOF|CHAR,(void *)WordMark, 0, (short*)10 },
{ "WordMarkColors",TABLEOF|COLOR,(void *)WordMarkColor, 0, (short*)10 },
{ "WordMarkBColors",TABLEOF|COLOR,(void *)WordMarkBColor, 0, (short*)10 },
{ "WordMarkTextures",TABLEOF|TEXTURE,(void *)WordMarkTexture, 0, (short*)10 },

{ "BoardColor", COLOR,(void *)&ColorMap[BOARD_AREA], 0, 0 },
{ "BoardTexture", TEXTURE,(void *)&TextureMap[BOARD_AREA], 0, 0 },

{ "BoardLeftShadowColor", COLOR,(void *)&ColorMap[LEFT_BSHADOW], 0, 0 },
{ "BoardLeftShadowTexture", TEXTURE,(void *)&TextureMap[LEFT_BSHADOW], 0, 0 },

{ "BoardTopShadowColor", COLOR,(void *)&ColorMap[TOP_BSHADOW], 0, 0 },
{ "BoardTopShadowTexture", TEXTURE,(void *)&TextureMap[TOP_BSHADOW], 0, 0 },

{ "TileLeftShadowColor", COLOR,(void *)&ColorMap[LEFT_TSHADOW], 0, 0 },
{ "TileLeftShadowTexture", TEXTURE,(void *)&TextureMap[LEFT_TSHADOW], 0, 0 },

{ "TileTopShadowColor", COLOR,(void *)&ColorMap[TOP_TSHADOW], 0, 0 },
{ "TileTopShadowTexture", TEXTURE,(void *)&TextureMap[TOP_TSHADOW], 0, 0 },

{ "TileColor", COLOR,(void *)&ColorMap[COMPUTER_TILES], 0, 0 },
{ "TileTexture", TEXTURE,(void *)&TextureMap[COMPUTER_TILES], 0, 0 },

{ "AlternateTileColor", COLOR,(void *)&ColorMap[HUMAN_TILES], 0, 0 },
{ "AlternateTileTexture", TEXTURE,(void *)&TextureMap[HUMAN_TILES], 0, 0 },

{ "NonPlayableSquareColor", COLOR,(void *)&ColorMap[BLACK_SQUARES], 0, 0 },
{ "NonPlayableSquareTexture", TEXTURE,(void *)&TextureMap[BLACK_SQUARES], 0, 0 },

{ "RackWallColor", COLOR,(void *)&ColorMap[RACK_VERT], 0, 0 },
{ "RackWallTexture", TEXTURE,(void *)&TextureMap[RACK_VERT], 0, 0 },

{ "RackBaseColor", COLOR,(void *)&ColorMap[RACK_HORIZ], 0, 0 },
{ "RackBaseTexture", TEXTURE,(void *)&TextureMap[RACK_HORIZ], 0, 0 },

{ "BlankTileTextColor", COLOR,(void *)&ColorMap[BLANK_TILE], 0, 0 },
{ "NormalTileTextColor", COLOR,(void *)&ColorMap[NORMAL_TILE], 0, 0 },
{ "MessageTextColor", COLOR,(void *)&ColorMap[MESSAGES], 0, 0 },
{ "LabelColor", COLOR,(void *)&ColorMap[LABELS], 0, 0 },

{ "PenaliseSquares", UNSIGNED|SHORT,(void *)&noGiveaway, 0, 0 },
{ "UseLetterWeights", UNSIGNED|SHORT,(void *)&useLetterWeights, 0, 0 },
{ "LengthWeight", UNSIGNED|SHORT,(void *)&lengthWeight, 0, 0 },
{ "MininumAllowedScore", UNSIGNED|SHORT,(void *)&minScoreLimit, 0, 0 },
{ "MaximumAllowedScore", UNSIGNED|SHORT,(void *)&maxScoreLimit, 0, 0 },
{ "RestrictedSearchMin", UNSIGNED|SHORT,(void *)&searchMin, 0, 0 },
{ "RestrictedSearchCount", UNSIGNED|SHORT,(void *)&searchCnt, 0, 0 },
{ "MinimumWordLength", UNSIGNED|SHORT,(void *)&minLength, 0, 0 },
{ "MaximumWordLength", UNSIGNED|SHORT,(void *)&maxLength, 0, 0 },
{ "MaxNewCrossWords", UNSIGNED|SHORT,(void *)&MaxNewXWord, 0, 0 },
{ "BoardPositionScore", TABLEOF|CHAR|USE_PTRS, (void *)boardScores, (short *)&Rows, (short *)&Cols },
{ "FirstMoveRow", UNSIGNED|SHORT|USE_PTRS,(void *)&firstMoveRow, NULL, (short *)&Rows },
{ "FirstMoveCol", UNSIGNED|SHORT|USE_PTRS,(void *)&firstMoveCol, NULL, (short *)&Cols },
{ "FirstMoveDir", UNSIGNED|SHORT,(void *)&firstMoveDir, 0, 0 },
{ "ControlStrategy", UNSIGNED|SHORT,(void *)&controlStrategy, 0, 0 },
{ "IncludeCrossWeights", UNSIGNED|SHORT,(void *)&weightWhat, 0, 0 },
{ "AdaptiveStrategy", UNSIGNED|SHORT,(void *)&adaptiveStrategy, 0, 0 },
{ "FreeForm", UNSIGNED|SHORT,(void *)&freeForm, 0, 0 },
{ "VideoMode", UNSIGNED|SHORT,(void *)&videoMode, 0, (short *)5 },
{ "VideoCard", VIDCARD,(void *)&videoCard, 0, (short *)5 },
{ "ExchangeAllowed", UNSIGNED|SHORT,(void *)&exchangeAllowed, 0, 0 },
{ "HotSquares", UNSIGNED|SHORT,(void *)&hotSquares, 0, 0 },
{ "BlankThreshold", UNSIGNED|SHORT,(void *)&blankThreshold, 0, 0 },
{ "LetterFrequencies", TABLEOF|UNSIGNED|SHORT,(void *)PoolStart, 0, (short*)27 },
{ "LetterScores", TABLEOF|SHORT,(void *)Scores, 0, (short*)27 },
{ "LetterWeights", TABLEOF|SHORT,(void *)Weights, 0, (short*)27 },
{ NULL, 0, NULL, 0, 0 }
};

/*
 * Default Configuration
 */

static void setDefaultConfig(void)
{
	int i, j;
	strcpy(dictName,"WW.DIC");
	videoMode = -1;
	autodetectVCard((int *)&videoCard);
	Rows = Cols = maxLength = 13;
	numtiles = 6;
	minScoreLimit = maxScoreLimit = searchMin = searchCnt = bonus = 0;
	lengthWeight = 2;
	firstMoveDir = controlStrategy = freeForm = weightWhat = minLength = 0;
	blankThreshold = 20;
	for (i=0;i<NUM_FILLS;i++) ColorMap[i] = TextureMap[i] = -1;
	for (i=NUM_FILLS;i<NUM_COLORS;i++) ColorMap[i] = -1;
	for (i=0;i<10;i++){
		WordMarkBColor[i] = LetterMarkBColor[i] = -1;
		WordMarkTexture[i] = LetterMarkTexture[i] = -1;
	}
	for (i=1;i<=MAXBOARD;i++) 
		for (j=1;j<=MAXBOARD;j++)
			boardScores[i][j] = 'a';

	firstMoveRow = firstMoveCol = 8;
	useLetterWeights = adaptiveStrategy = 1;
	noGiveaway = 3;
	exchangeAllowed = 0;
	hotSquares = 0;
	MaxNewXWord = 0;
	for (i=0;i<27;i++)
	{
		PoolStart[i]=2;
		Scores[i]=1;
		Weights[i]=0;
	}
	PoolStart['A'-'A'] = 7;
	PoolStart['E'-'A'] = 10;
	PoolStart['I'-'A'] = 6;
	PoolStart['O'-'A'] = 7;
	PoolStart['U'-'A'] = 4;
}

/*******************************/
/* CONFIGURATION FILE HANDLING */
/*******************************/

static void abort(void) {
	fprintf(stderr,"Error in configuration file %s, line %d\n",
			fnamenow, linenum);
	shutdown();
}

int getNumber(FILE *fp, char *msg, int canBneg, int Min, int Max) {
	char c;
	int rtn, sgn=1;
	while (!feof(fp)) {
		c = (char)fgetc(fp);
		if (c=='#') { /* comment - skip to end of line */
			while (fgetc(fp)!='\n')
				if (feof(fp)) break;
			linenum++;
			continue;
		}
		if (c==' ' || c=='\t' || c=='\n' || c=='\r') {
			if (c=='\n') linenum++;
			continue;
		}
		rtn = 0;
		if (canBneg && c=='-') { sgn=-1; c = (char)fgetc(fp); }
		if (c<'0' || c>'9') {
			fprintf(stderr,"Line %d - digit or - expected, not %c!\n",linenum,c);
			fprintf(stderr,"Error happened while scanning for %s\n",msg);
			abort();
		}
		while (c>='0' && c<='9' && !feof(fp)) {
			rtn = rtn*10 + c-'0';
			c = (char)fgetc(fp);
		}
		if (c=='\n') linenum++;
		rtn *= sgn;
		if (Max!=Min) {
			if (rtn<Min || rtn>Max) {
				fprintf(stderr,"Bad %s value (%d) on line %d of configuration\n! Must be between %d and %d\n",
					msg,rtn,linenum,Min,Max);
				abort();
			}
		}
		return rtn;
	}
	fprintf(stderr,"Premature end-of-file while scanning %s!\n",msg);
	abort();
	return 0; /* unreachable */
}

char getChar(FILE *fp, char *msg) {
	char c;
	while (!feof(fp)) {
		c = (char)fgetc(fp);
		if (c=='#') { /* comment - skip to end of line */
			while (fgetc(fp)!='\n')
				if (feof(fp)) break;
			linenum++;
			continue;
		}
		if (c==' ' || c=='\t' || c=='\n' || c=='\r') {
			if (c=='\n') linenum++;
			continue;
		}
		if (c=='\\') c = (char)fgetc(fp);
		else if (c=='}') c=-1;
		else if (c=='$') {
			if ((c = (char)(fgetc(fp)-'0'))==0) c=10;
		} else if (c<32) {
			if (msg) {
				fprintf(stderr,"Bad %s value (%d) on line %d of configuration\n",
					msg,c,linenum);
				abort();
			} else return 0;
		}
		return c;
	}
	return 0; /* end of file */
}

static void skipchar(FILE *cfg, char c) {
	char c2;
	if ((c2=getChar(cfg,"separator"))!=c && (c!='}' || c2!=-1)) {
		fprintf(stderr,"Expected a %c but read a %c on line %d of configuration!\n",
			c,c2,linenum);
		abort();
	}
}

static void getString(FILE *cfg, short elt, short Max) {
	char *res = WWCfg[elt].loc, buff[80];
	buff[0] = getChar(cfg,WWCfg[elt].name);
	if (buff[0]=='}') {
		buff[1]=0;
	} else {
		fscanf(cfg,"%s",buff+1);
	}
	strncpy(res,buff,(unsigned)Max);
}

static int readString(FILE *cfg, char *buff) {
	char c;
	c = getChar(cfg,NULL);
	buff[1]=0;
	if (c>0) {
		buff[0]=c;
		fscanf(cfg,"%s",buff+1);
		(void)strupr(buff);
		return 0;
	} else {
		buff[0] = '}';
		return -1;
	}
}

static int getIdentifier(FILE *cfg) {
	int i = 0;
	char c, buff[80];
	c = getChar(cfg,NULL);
	if (c>'Z') c -= 'a'-'A';
	while (c>='A' && c<='Z') {
		buff[i++] = c;
		c = (char)fgetc(cfg);
		if (c>'Z') c-='a'-'A';
	}
	buff[i]=0;
	if (i>0) {
		i = 0;
		if (c != '=') skipchar(cfg, '=');
		while (WWCfg[i].name)
			if (strcmp(WWCfg[i].name,buff)==0) return i;
			else i++;
	}
	return -1;
}

int loadBoard(FILE *cfg) { // returns 1 if no words on board, 0 otherwise
	int i, j, c, rtn=1;
	for (i=1;i<=Rows;i++)
	{
		for (j=1;j<=Cols;j++)
		{
			c = getChar(cfg,"board letter");
			if (c!='.')
			{
				if (c>='a')
					boardLetters[i][j] = (uchar)(0x80 | (c-'a'));
				else
					boardLetters[i][j] = (uchar)(c-'A');
				rtn = 0;
			}
		}
	}
	return rtn;
}

static short getSymbolic(FILE *cfg, char *msg, char *nameTbl[], int inTable) {
	char buff[80];
	if (readString(cfg, buff)==0) {
		short i = 0;
		while (nameTbl[i])
			if (strcmp(nameTbl[i],buff)==0) return i;
			else i++;
	}
       	if (inTable && strcmp(buff,"}")==0) return -1; /* end of table */
	fprintf(stderr,"Invalid %s (%s) on line %d of configuration\n",msg,buff,linenum);
	abort();
	return 0; /* unreachable */
}

short getColour(FILE *cfg, int inTable) {
	return getSymbolic(cfg,"colour name",ColourNames,inTable);
}

static short getTexture(FILE *cfg,int inTable) {
	return getSymbolic(cfg,"texture name",TextureNames,inTable);
}

static short getVidcard(FILE *cfg) {
	return getSymbolic(cfg,"video card",VCardNames,0);
}

void loadCFG(char *cfgname) {
	extern short defaultColorMap[],
		     defaultTextureMap[];
	int i, j, elt, top_lnnum;
	FILE *cfg = fopen(cfgname,"rt"), *top_fp = NULL;
	setDefaultConfig();
	if (cfg==NULL) {
		fprintf(stderr,"Cannot open configuration file %s!\n",cfgname);
		if (strcmp(cfgname,"ww.cfg")==0)
			fprintf(stderr,"Please run the SETUP program.\n");
		exit(-5);
	}
	fnamenow = cfgname;
	linenum = 1;
	i = 0; while (WWCfg[i].name) (void)strupr(WWCfg[i++].name);
	i = 0; while (ColourNames[i]) (void)strupr(ColourNames[i++]);
	i = 0; while (TextureNames[i]) (void)strupr(TextureNames[i++]);
	i = 0; while (VCardNames[i]) (void)strupr(VCardNames[i++]);
	for (;;) {
		if (feof(cfg)) {
			if (top_fp) {
				fclose(cfg);
				cfg = top_fp;
				linenum = top_lnnum;
				top_fp = NULL;
				fnamenow = cfgname;
			} else break;
		}
		if ((elt = getIdentifier(cfg))>=0) {
			if (WWCfg[elt].type&TABLEOF) {
				int LimR, LimC;
				skipchar(cfg, '{');
				if (WWCfg[elt].type&USE_PTRS) {
					if (WWCfg[elt].dimr)
						LimR = *WWCfg[elt].dimr;
					else LimR = 0;
					LimC = *WWCfg[elt].dimc;
				} else {
					LimR = (int)(WWCfg[elt].dimr);
					LimC = (int)(WWCfg[elt].dimc);
				}
				if (LimR==0) { /* 1-d table; we assume of shorts */
					for (i=0;i<LimC;i++) {
						switch(WWCfg[elt].type&BASE_MASK) {
						case SHORT:
							((short*)(WWCfg[elt].loc))[i] = getNumber(cfg,WWCfg[elt].name,(WWCfg[elt].type&UNSIGNED)?0:1,0,0);
							break;
						case CHAR:
							{ char c = getChar(cfg,WWCfg[elt].name);
							  if (c!=-1) ((char*)(WWCfg[elt].loc))[i] = c;
							  else goto endTable;
							break;
							}
						case COLOR:
							{ short c = getColour(cfg,1);
							  if (c!=-1) ((short*)(WWCfg[elt].loc))[i] = c;
							  else goto endTable;
							break;
							}
						case TEXTURE:
							{ short c = getTexture(cfg,1);
							  if (c!=-1) ((short*)(WWCfg[elt].loc))[i] = c;
							  else goto endTable;
							break;
							}
						}
					}
				} else { /* 2-d table - only boardScores */
					for (i=1;i<=LimR;i++)
						for (j=1;j<=LimC;j++)
							boardScores[i][j] = (uchar)getChar(cfg,WWCfg[elt].name);
				}
				skipchar(cfg,'}');
endTable:;
			} else {
				int Min, Max;
				if (WWCfg[elt].type&USE_PTRS) {
					if (WWCfg[elt].dimr)
						Min = *WWCfg[elt].dimr;
					else Min = 0;
					Max = *WWCfg[elt].dimc;
				} else {
					Min = (int)(WWCfg[elt].dimr);
					Max = (int)(WWCfg[elt].dimc);
				}
				switch(WWCfg[elt].type&BASE_MASK) {
				case STRING:
					getString(cfg,elt,Max);
					if (WWCfg[elt].type&INCLUDE_FILE) {
						if (top_fp) {
							fprintf(stderr,"WW: Already nested; cannot have Include in %s (%d)\n",inc_name,linenum);
							abort();
						} else {
							top_fp = cfg;
							cfg = fopen(inc_name,"rt");
							fnamenow = inc_name;
							if (cfg==NULL) {
								fprintf(stderr,"WW: Cannot open included file %s (%s line %d)\n",inc_name,cfgname,linenum);
								abort();
							}
							top_lnnum = linenum;
							linenum =1;
						}
					}
					break;
				case CHAR:
					*((char*)WWCfg[elt].loc) = getChar(cfg,WWCfg[elt].name);
					break;
				case SHORT:
					*((short*)WWCfg[elt].loc) = getNumber(cfg,
						WWCfg[elt].name,
						(WWCfg[elt].type&UNSIGNED)?0:1,Min,Max);
					break;
				case COLOR:
					*((short*)WWCfg[elt].loc) = getColour(cfg,0);
					break;
				case TEXTURE:
					*((short*)WWCfg[elt].loc) = getTexture(cfg,0);
					break;
				case VIDCARD:
					*((short*)WWCfg[elt].loc) = getVidcard(cfg);
					break;
				}
			}
		} else if (!feof(cfg)) abort();
		else continue;
	}
	/* Initialise other stuff */
	for (i=0;i<NUM_FILLS;i++) {
		if (TextureMap[i]==-1) {
			TextureMap[i] = defaultTextureMap[i];
		}
	}
	for (i=0;i<NUM_COLORS;i++) {
		if (ColorMap[i]==-1) {
			if (videoCard!=1) ColorMap[i] = defaultColorMap[i];
			else if (i==CLEAR_AREA) ColorMap[i] = 0; // BLACK
			else ColorMap[i] = 15; // WHITE
		}
	}
	for (i=0;i<10;i++) {
		if (LetterMarkBColor[i]==-1)
			LetterMarkBColor[i] = ColorMap[BOARD_AREA];
		if (LetterMarkTexture[i]==-1)
			LetterMarkTexture[i] = TextureMap[BOARD_AREA];
		if (WordMarkBColor[i]==-1)
			WordMarkBColor[i] = ColorMap[BOARD_AREA];
		if (WordMarkTexture[i]==-1)
			WordMarkTexture[i] = TextureMap[BOARD_AREA];
	}
	for (i=1;i<=MAXBOARD;i++) 
		for (j=1;j<=MAXBOARD;j++)
			initBoardLetters[i][j] = (boardScores[i][j]==BLACKCH)?
					BLACKSQ: EMPTY;
	if (freeForm==1) colourTiles=1;
	for (i=0;i<=(Rows+1);i++) boardScores[i][0] = boardScores[i][Cols+1] = 'a';
	for (j=1;j<=(Cols+1);j++) boardScores[0][j] = boardScores[Rows+1][j] = 'a';
	if (firstPlayer==-1) firstPlayer = (int)random(2);
	fclose(cfg);
}


