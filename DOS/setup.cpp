// WordsWorth install - uses Al Steven's DFlat++ CUA library

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <fstream.h>
#include <sys/stat.h>
#include <io.h>
#include <errno.h>

#include "dflatpp.h"

#define APP	1	// 0 => refresh assistant, 1 => WordsWorth setup

#if APP
#define TITLE	"WordsWorth Setup by Graham Wheeler (c) 1994"
#define BYE	"Thanks for using WordsWorth Setup!\n"
#define USE	"Useage: setup [ <config file> ] [ -o <output file> ]\n" \
		"The default config file is SETUP.CFG\n" \
		"The default output file is WW.CFG\n"
#else
#define TITLE	"Refresh Assistant v1.0      (c) Open Mind Solutions 1995"
#define BYE	"Thanks for using the Refresh Assistant!\n"
#define USE	"Useage: ra\n"
#endif

#define Df void (DFWindow::*)()
#define Ap void (Application::*)()

static int doOption(int i);
static void startnest(char *name);
static int ask(char *buff, int defalt);

static void showMsg(const char *msg, char *arg)
{
	char buff[80];
	sprintf(buff,msg,arg);
	ErrorMessageBox msgBox(buff, "Error", BORDER | SAVESELF | SHADOW);
	desktop.speaker().Beep();
	msgBox.Execute();
}

//---------------------------------------------------------------------
// SETUP

#define MAXOPTS	9

static char buff[82];
static char question[82];
static char options[MAXOPTS][82];
char *addCode[MAXOPTS];
char *incFiles[MAXOPTS];
char *nestFiles[MAXOPTS];
char *commands[MAXOPTS];

#define MAXNEST		8

FILE	*cfgOut;			/* Output file		*/
FILE	*fpStack[MAXNEST];	/* Nested input files	*/
int	lineNum[MAXNEST],
	nestLevel = 0;
char	names[MAXNEST][64];

// Dialog box for setup

class Setup : public Dialog
{
    // -----File Open Dialog Box Controls:
protected:
    int cancelled;
    OKButton ok;
    CancelButton cancel;
public:
   int Cnt, mustNest;
    RadioButton *Set[9];
    Setup(char *item, char options[][82], int cnt, int defalt);
    ~Setup();
    virtual void OKFunction();
    virtual void CancelFunction();
    void handleButton();
    int IsCancelled() { return cancelled; }
};

void Setup::handleButton()
{
}

Setup::Setup(char *item, char options[][82], int cnt, int defalt)
#if APP
	: Dialog(item, 19, 70, (DFWindow *)desktop.ApplWnd(), BORDER | SAVESELF| SHADOW),
	  ok(20, 16, this), cancel(60,16, this), cancelled(0)
#else
	: Dialog(item, 25, 80, (DFWindow *)desktop.ApplWnd(), BORDER | SAVESELF| SHADOW),
	  ok(20, 21, this), cancel(60,21, this), cancelled(0)
#endif
{
	for (int i=0;i< cnt;i++)
		Set[i] = new RadioButton(options[i], 10, i+2, this);
	Cnt = cnt;
	if (defalt>0) Set[defalt-1]->PushButton();
}

Setup::~Setup()
{
	for (int i=0;i<Cnt;i++)
		delete Set[i];
}

void Setup::OKFunction()
{
	int i;
	mustNest = 0;
	for (i=0;i<Cnt;i++)
		if (Set[i]->Setting())
		{
			mustNest = doOption(i);
			Dialog::OKFunction();
			break;
		}
}

void Setup::CancelFunction()
{
    cancelled = 1;
    CloseWindow();
    Dialog::CancelFunction();
}

// ------- WWSetup application definition

class WWSetup : public Application
{
    char *inm, *onm;
public:
    WWSetup(char *inm_in, char *onm_in);
    ~WWSetup();
    void Execute();
};

static Color col = {
	LIGHTGRAY, BLUE,
	LIGHTGRAY, BLUE,
	LIGHTGRAY, BLUE,
	LIGHTGRAY, BLUE
};

void WWSetup::Execute()
{
	cfgOut = fopen(onm,"w");
	if (cfgOut == NULL)
	{
		showMsg("Cannot open %s", onm);
		return;
	}
	if ((fpStack[nestLevel=0] = fopen(inm,"r"))==NULL)
	{
		showMsg("Cannot open %s", inm);
		fclose(cfgOut);
		return;
	}
	int cancel = 0;
	for (;;)
	{
		if (cancel || feof(fpStack[nestLevel]))
		{
			cancel = 0;
			fclose(fpStack[nestLevel]);
			if (--nestLevel < 0) break;
		}
		fgets(buff,80,fpStack[nestLevel]);
		lineNum[nestLevel]++;
		if (buff[0]=='#') continue;
		if (buff[0]=='%' && buff[1]=='%') break;
		if (buff[0]=='@')
		{
			int defalt = -1, i=1;
			if (isdigit(buff[i]))
			{
				defalt = 0;
				while (isdigit(buff[i]))
					defalt = defalt * 10 + buff[i++] - '0';
			}
			int rtn = ask(buff+i, defalt);
			if (rtn == 1) startnest(buff);
			else if (rtn < 0) cancel = 1;
		}
	}
	fclose(cfgOut);
	CloseWindow();
}

WWSetup::~WWSetup()
{
	CloseWindow();
}

WWSetup::WWSetup(char *inm_in, char *onm_in)
	: Application(TITLE, BORDER | SAVESELF)
{
    SetClearChar(' ');
    inm = inm_in;
    onm = onm_in;
    Show();
}

static void startnest(char *name)
{
	int i = strlen(name);
	if (name[i-1]<=32)
	{
		while (i && name[--i]<=32);
		name[i+1]=0;
	}
	if (++nestLevel==MAXNEST)
	{
		--nestLevel;
		showMsg("Nesting overflow",NULL);
		return;
	}
	strcpy(names[nestLevel],name);
	if ((fpStack[nestLevel] = fopen(name,"r"))==NULL)
	{
		showMsg("Cannot open %s",name);
		--nestLevel;
		return;
	}
	lineNum[nestLevel] = 0;
}

static int doOption(int i)
{
	if (commands[i])
	{
	    char cmdbuf[80];
	    char *s = commands[i];
	    while (s)
	    {
		char *e = strchr(s, ';');
		if (e)
		{
		    strncpy(cmdbuf, s, e-s);
		    cmdbuf[e-s] = 0;
		    s = e+1;
		}
		else
		{
		    strcpy(cmdbuf, s);
		    s = 0;
		}
		system(cmdbuf);
	    }
	}
	else if (addCode[i])
	{
		int j = 0;
		while (addCode[i][j])
		{
			if (addCode[i][j]=='\\')
			{
				char c;
				switch (c = addCode[i][++j])
				{
				case 'n':
					fprintf(cfgOut,"\n");
					break;
				case 't':
					fprintf(cfgOut,"\t");
					break;
				default:
					fprintf(cfgOut,"%c",c);
					break;
				}
			}
			else fprintf(cfgOut,"%c",addCode[i][j]);
			j++;
		}
	}
	else if (nestFiles[i])
	{
		strcpy(buff,nestFiles[i]+1);
		return 1;
	}
	else if (incFiles[i])
	{
		int j = 0, k = 0, l = strlen(incFiles[i]);
		while (j<l)
		{
			if (incFiles[i][j]==' ' || incFiles[i][j]=='\t')
			{
				incFiles[i][j]=0;
				if (j>k) fprintf(cfgOut,"Include\t\t= %s\n",&incFiles[i][k]);
				j++;
				while (j<l && (incFiles[i][j]==' ' || incFiles[i][j]=='\t'))
					j++;
				k = j;
			}
			else j++;
		}
		if (j>k) fprintf(cfgOut,"Include\t\t= %s\n",&incFiles[i][k]);
	}
	return 0;
}

static int ask(char *buff, int defalt)
{
	int o = 0; /* number of options */
	int i, j, k, l;
	strcpy(question,buff);
	question[strlen(question)-1] = '\0'; // strip \n
	while (!feof(fpStack[nestLevel]))
	{
		fgets(buff,80,fpStack[nestLevel]);
		if (buff[0]=='<') startnest(buff+1);
		lineNum[nestLevel]++;
		if (buff[0]=='%') goto gotOptions;
		if (buff[0]=='>')
		{
			strcpy(options[o],buff+1);
			l = strlen(buff)-1;
			j = 0;
			while (options[o][j]!='=' && j<l) j++;
			if (j==l)
			{
				char buff[80];
				sprintf(buff,"Error in %s line %d, missing `='",
					names[nestLevel], lineNum[nestLevel]);
				showMsg(buff, NULL);
			}
			/* skip forward to the action */
			commands[o] = addCode[o] = incFiles[o] = nestFiles[o] = NULL;
			i = j;
			options[o][j] = 0;
			while (++i < l)
			{
				if (options[o][i]==' ') continue;
				if (options[o][i]=='\t') continue;
				if (options[o][i]=='{')
				{
					i++;
					if (i<(l-1))
					{
						addCode[o] = &options[o][i];
						k = l-1;
						while (k>i)
						{
							if (options[o][k]=='}')
							{
								options[o][k]=0;
								goto gotAction;
							}
							k--;
						}
					}
					char buff[80];
					sprintf(buff,"Missing } in %s line %d",
						names[nestLevel], lineNum[nestLevel]);
					showMsg(buff, NULL);
				}
				else if (options[o][i]=='[')
				{
					i++;
					if (i<(l-1))
					{
						commands[o] = &options[o][i];
						k = l-1;
						while (k>i)
						{
							if (options[o][k]==']')
							{
								options[o][k]=0;
								goto gotAction;
							}
							k--;
						}
					}
					char buff[80];
					sprintf(buff,"Missing ] in %s line %d",
						names[nestLevel], lineNum[nestLevel]);
					showMsg(buff, NULL);
				}
				else
				{
					if (options[o][i]=='<') 
						nestFiles[o] = &options[o][i];
					else
						incFiles[o] = &options[o][i];
					k = l-1;
					while (k>i)
					{
						if (options[o][k]==' ' || 
						    options[o][k]=='\t' || 
						    options[o][k]=='\n') 
							options[o][k]=0;
						else break;
						k--;
					}
					break;
				}
			}
gotAction:;
			/* trim backwards */
			i = j;
			while (--i>=0)
			{
				if (options[o][i]==' ' || options[o][i]=='\t')
					options[o][i] = 0;
				else break;
			}
			if (++o==MAXOPTS) break;
		}
	}
gotOptions:;
	if (o)
	{ /* ask */
		if (question[0]=='?')
		{
			/* do all the options */
			for (i=0;i<o;i++)
				if (doOption(i)) return 1;
		}
		else
		{
			Setup Stp(question, options, o, defalt);
			Stp.Show();
			Stp.Execute();
			if (Stp.IsCancelled()) return -1;
			else if (Stp.mustNest) return 1;
		}
	}
	return 0;
}


static void useage(void)
{
    fprintf(stderr, USE);
    exit(-1);
}

void main(int argc, char *argv[])
{
	char *inm = NULL, *onm = NULL;
#if APP
	for (int i=1; i<argc; i++)
	{
		if (argv[i][0]=='-')
		{
			switch (argv[i][1])
			{
				case 'o':
					if (onm) useage();
					else if (argv[i][2]=='\0')
					{
						if (++i == argc) useage();
						else onm = argv[i];
					}
					else onm = argv[i]+2;
					break;
				default:
					useage();
			}
		}
		else if (inm) useage();
		else inm = argv[i];
	}
	if (onm == NULL) onm = "ww.cfg";
	if (inm == NULL) inm = "setup.cfg";
#else
	if (onm == NULL) onm = "ra.out";
	if (inm == NULL) inm = "ra.cfg";
#endif
	WWSetup ma(inm, onm);
	ma.Execute();
	fprintf(stderr, BYE);
}
