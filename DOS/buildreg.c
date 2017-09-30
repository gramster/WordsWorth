#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "version.h"
#include "regname.h"

FILE *fp;
long serial;
char regname[80];
int i, l;
struct date date;

void main(int argc, char *argv[])
{
	if (argc!=3)
	{
		fprintf(stderr,"Useage: buildreg <registration data base name> <header file>\n");
		exit(-1);
	}
	fp = fopen("serial.dat","r");
	if (fp)
	{
		fscanf(fp,"%ld",&serial);
		fclose(fp);
	}
	else serial = 0;
	serial++;
	strcpy(regname,REGNAME);
	printf("Building %s, Version: %s  Serial: %s-%ld  Name: %s\n", argv[2],
		VERSION,VERSION,serial,REGNAME);
	fp = fopen(argv[2],"w");
	fprintf(fp,"%cdefine SERIAL\t\"%s-%d\"\n\n",'#',VERSION,serial);
	fprintf(fp,"char Register[] = {\t(char)('['|0x80),\n%cifdef REGISTERED\n",'#');
	l = strlen(regname);
	for (i=0;i<l;i++) fprintf(fp,"\t(char)('%c'|0x80),\n",regname[i]);
	fprintf(fp,"%celse\n",'#');
	l = strlen("UNREGISTERED");
	for (i=0;i<l;i++) fprintf(fp,"\t(char)('%c'|0x80),\n","UNREGISTERED"[i]);
	fprintf(fp,"%cendif\n",'#');
	fprintf(fp,"\t(char)(']'|0x80),\n\t(char)0x80,\n\t0\n};\n");
	fclose(fp);
	puts("Is this copy being shipped?");
	{ int c; if (c=getch(), c=='y'||c=='Y') {
		fp = fopen("serial.dat","w");
		if (fp)
		{
			fprintf(fp,"%ld\n",serial);
			fclose(fp);
		}
		printf("Updating %s...\n", argv[1]);
		getdate(&date);
		fp =fopen(argv[1],"a");
		fprintf(fp,"%s-%ld\t%d-%d-%d\t%s\t%s\t%s\n",VERSION,serial,
			date.da_day,date.da_mon,date.da_year-1900,REGNAME,
			ADDRESS,COMMENT);
		fclose(fp);
		puts("Making dictionary...");
		system("makedict master.big wwbig.dic \""REGNAME"\"");
		system("makedict master.med wwmed.dic \""REGNAME"\"");
	  }
	}
}

