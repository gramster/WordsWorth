#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

#define MAXOPTS	9

static char buff[82];
static char question[82];
static char options[MAXOPTS][82];
char *addCode[MAXOPTS];
char *incFiles[MAXOPTS];
char *nestFiles[MAXOPTS];

#define MAXNEST		8

FILE	*ocfg;			/* Output file		*/
FILE	*fpStack[MAXNEST];	/* Nested input files	*/
int	line[MAXNEST],
	nestLevel = 0;
char	names[MAXNEST][64];

static void startnest(char *name) {
	int i = strlen(name);
	if (name[i-1]<32) {
		while (i && name[--i]<32);
		name[i+1]=0;
	}
	if (++nestLevel==MAXNEST) {
		fprintf(stderr,"Cannot nest more than %d levels deep, line %d\n",line);
		exit(-2);
	}
	strcpy(names[nestLevel],name);
	if ((fpStack[nestLevel] = fopen(name,"r"))==NULL) {
		fprintf(stderr,"Cannot open setup file %s!\n",name);
		exit(-1);
	}
	line[nestLevel] = 0;
}

static int doOption(int i) {
	if (addCode[i]) {
		int j = 0;
		while (addCode[i][j]) {
			if (addCode[i][j]=='\\') {
				char c;
				switch (c = addCode[i][++j]) {
				case 'n':
					fprintf(ocfg,"\n");
					break;
				case 't':
					fprintf(ocfg,"\t");
					break;
				default:
					fprintf(ocfg,"%c",c);
					break;
				}
			} else fprintf(ocfg,"%c",addCode[i][j]);
			j++;
		}
	} else if (nestFiles[i]) {
		strcpy(buff,nestFiles[i]+1);
		return 1;
	} else if (incFiles[i]) {
		int j = 0, k = 0, l = strlen(incFiles[i]);
		while (j<l) {
			if (incFiles[i][j]==' ' || incFiles[i][j]=='\t') {
				incFiles[i][j]=0;
				if (j>k) fprintf(ocfg,"Include\t\t= %s\n",&incFiles[i][k]);
				j++;
				while (j<l && (incFiles[i][j]==' ' || incFiles[i][j]=='\t'))
					j++;
				k = j;
			} else j++;
		}
		if (j>k) fprintf(ocfg,"Include\t\t= %s\n",&incFiles[i][k]);
	}
	return 0;
}

static int ask(char *buff, int defalt) {
	int o = 0; /* number of options */
	int i, j, k, l;
	strcpy(question,buff);
	while (!feof(fpStack[nestLevel])) {
		fgets(buff,80,fpStack[nestLevel]);
		if (buff[0]=='<') startnest(buff+1);
		line[nestLevel]++;
		if (buff[0]=='%') goto gotOptions;
		if (buff[0]=='>') {
			strcpy(options[o],buff+1);
			l = strlen(buff)-1;
			j = 0;
			while (options[o][j]!='=' && j<l) j++;
			if (j==l) {
				fprintf(stderr,"Error in setup configuration file %s line %d, missing `='\n",
					names[nestLevel], line[nestLevel]);
				exit(-2);
			}
			/* skip forward to the action */
			addCode[o] = incFiles[o] = nestFiles[o] = NULL;
			i = j;
			options[o][j] = 0;
			while (++i < l) {
				if (options[o][i]==' ') continue;
				if (options[o][i]=='\t') continue;
				if (options[o][i]=='{') {
					i++;
					if (i<(l-1)) {
						addCode[o] = &options[o][i];
						k = l-1;
						while (k>i) {
							if (options[o][k]=='}') {
								options[o][k]=0;
								goto gotAction;
							}
							k--;
						}
					}
					fprintf(stderr,"Missing } in configuration file %s line %d\n",
						names[nestLevel], line[nestLevel]);
					exit(-2);
				} else {
					if (options[o][i]=='<') 
						nestFiles[o] = &options[o][i];
					else
						incFiles[o] = &options[o][i];
					k = l-1;
					while (k>i) {
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
			while (--i>=0) {
				if (options[o][i]==' ' || options[o][i]=='\t')
					options[o][i] = 0;
				else break;
			}
			if (++o==MAXOPTS) break;
		}
	}
gotOptions:;
	if (o) { /* ask */
		if (question[0]=='?') {
			/* do all the options */
			for (i=0;i<o;i++)
				if (doOption(i)) return 1;
		} else {
			for (;;) {
				clrscr();
				printf("\n\n%s",question);
				if (defalt>0) printf("(default %d)\n\n",defalt);
				else printf("\n");
				for (i=0;i<o;i++)
					printf("%2d> %s\n",i+1,options[i]);
				printf("\nOption (1-%d)? ",o);
				fflush(stdout);
				k = getch();
				if (k<32) k = defalt;
				else k -= '0';
				if (k>0 && k<=o) {
					if (doOption(k-1)) return 1;
					break;
				}
				fprintf(stderr,"Illegal option! Enter a number between 1 and %d\n",o);
				putchar(7);
				sleep(2);
			}
		}
	}
	return 0;
}

static void useage(void)
{
	fprintf(stderr,"Useage: setup [ <config file> ] [ -o <output file> ]\n");
	fprintf(stderr,"The default config file is SETUP.CFG\n");
	fprintf(stderr,"The default output file is WW.CFG\n");
	exit(-1);
}

void main(int argc, char *argv[])
{
	int i;
	char *inm = NULL, *onm = NULL;
	for (i=1; i<argc; i++)
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
	if (inm == NULL) inm = "setup.cfg";
	if (onm == NULL) onm = "ww.cfg";

	if ((fpStack[nestLevel=0] = fopen(inm,"r"))==NULL) {
		fprintf(stderr,"Cannot open %s!\n", inm);
		exit(-1);
	}
	ocfg = fopen(onm,"w");
	if (ocfg == NULL)
	{
		fprintf(stderr,"Cannot open %s!\n", onm);
		exit(-1);
	}
	for (;;) {
		if (feof(fpStack[nestLevel])) {
			fclose(fpStack[nestLevel]);
			if (--nestLevel < 0) break;
		}
		fgets(buff,80,fpStack[nestLevel]);
		line[nestLevel]++;
		if (buff[0]=='#') continue;
		if (buff[0]=='%' && buff[1]=='%') break;
		if (buff[0]=='@') {
			int defalt = -1, i=1;
			if (isdigit(buff[i])) {
				defalt = 0;
				while (isdigit(buff[i]))
					defalt = defalt * 10 + buff[i++] - '0';
			}
			if (ask(buff+i, defalt)) startnest(buff);
		}
	}
	fclose(ocfg);
	printf("\n\nSetup has saved the configuration in the file %s\n", onm);
	puts("Thank you for using WordsWorth Setup!");
}
