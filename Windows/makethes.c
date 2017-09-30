#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char buff[1500];

/***********************/
/* DICTIONARY HANDLING */
/***********************/

/* S-coder encryption from Dr Dobbs Jan 1990 */

static char 	*cryptext;	/* Key				*/
static int	crypt_ptr=0;	/* Circular pointer into key	*/
static int	crypt_length;	/* Key length			*/

static void crypt(char *buf)
{
  *buf ^= cryptext[crypt_ptr]^(cryptext[0]*crypt_ptr);
  cryptext[crypt_ptr] += ((crypt_ptr < (crypt_length - 1)) ?
			  cryptext[crypt_ptr+1] : cryptext[0]);
  if (!cryptext[crypt_ptr]) cryptext[crypt_ptr]++;
  if (++crypt_ptr >= crypt_length) crypt_ptr = 0;
}

static void encrypt(char *buf, int len) {
	while (len--) crypt(buf++);
}

void encryptThes(char *inname, char *outname, char *key) {
	int n, j;
	FILE *thes, *outf; char *B = (char *)buff;
	static char K[64];
	strcpy(K,key);
	thes = fopen(inname,"rb");
	if (thes==NULL) {
		fprintf(stderr,"Cannot open thesaurus %s!\n",inname);
		exit(0);
	}
	outf = fopen(outname,"wb");
	if (outf==NULL) {
		fprintf(stderr,"Cannot create encrypted thesaurus %s!\n",outname);
		exit(0);
	}
	printf("Encrypting with key <%s>\n",key);
	cryptext = key;
	crypt_length = strlen(key);
	while (!feof(thes)) {
		n=fread(buff,sizeof(char),1024,thes);
		if (n>=0) encrypt((char*)buff,n);
		fwrite(buff,sizeof(char),n,outf);
	}
	fclose(thes);
	fclose(outf);
}

main(argc,argv)
int argc;
char *argv[];
{
    unsigned i, n;
    char key[80];
    if (argc!=4) {
      puts("Usage: makethes <thes. ref file> <output file> <regname>");
      exit(0);
    }
    encryptThes(argv[1],argv[2],argv[3]);
}
