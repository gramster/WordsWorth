#include <stdio.h>
#include <stdlib.h>

#define GLOBAL extern
#include "dict.h"

NODE buff[1500];

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

static void encrypt(char *buf, int len, char *key) {
	cryptext = key;
	crypt_length = strlen(key);
	while (len--) crypt(buf++);
}

void encryptDawg(char *inname, char *outname, char *key) {
	int i, n, j;
	FILE *dawg, *outf; char *B = (char *)buff;
	static char K[64];
	strcpy(K,key);
	dawg = fopen(inname,"rb");
	if (dawg==NULL) {
		fprintf(stderr,"Cannot open dictionary %s!\n",inname);
		exit(0);
	}
	outf = fopen(outname,"wb");
	if (outf==NULL) {
		fprintf(stderr,"Cannot create encrypted dictionary %s!\n",outname);
		exit(0);
	}
	printf("Encrypting with key <%s>\n",key);
	while (!feof(dawg)) {
		n=fread(buff,sizeof(NODE),1024,dawg);
		if (n>=0) encrypt((char*)buff,n*sizeof(NODE),key);
		fwrite(buff,sizeof(NODE),n,outf);
		i++;
	}
	fclose(dawg);
	printf("Adding key info\n");
	for (i=0;i<256;i++) B[i] = random(26)+'A' + random(2)*('a'-'A');
	i = 1;
	j = 0;
	while (K[j]) {
		B[i] = K[j++];
		i += j;
	}
	if (j>=26) B[0] = 'a'+j-26;
	else B[0] = 'A'+j;
	crypt_ptr=0;
	encrypt(B,256,"WordsWorth");
	fwrite(B,sizeof(char),256,outf);
	fclose(outf);
}

main(argc,argv)
int argc;
char *argv[];
{
    unsigned i, n;
    char key[80];
    if (argc!=4) {
      puts("Usage: makedict <dawg file> <output file> <regname>");
      exit(0);
    }
    encryptDawg(argv[1],argv[2],argv[3]);
}
