#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char buff[256];

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

char *readKey(char *inname) {
	int i, n, j, l;
	FILE *dawg = fopen(inname,"rb");
	static char key[64];
	if (dawg==NULL) {
		fprintf(stderr,"Cannot open dictionary %s!\n",inname);
		exit(0);
	}
	if (fseek(dawg,-256l,SEEK_END)!=0) {
		fprintf(stderr,"fseek failed on %s!\n",inname);
		exit(0);
	}
	if ((n=fread(buff,sizeof(char),256,dawg))==-1) {
		fprintf(stderr,"read failed on %s: %s!\n",inname,
				strerror(errno));
		exit(0);
	} else if (n<256) {
		fprintf(stderr,"Only read %d bytes of 256 in %s!\n",n,inname);
		exit(0);
	}
	fclose(dawg);
	encrypt(buff,256,"WordsWorth");
	if (buff[0]>='a') l = buff[0]-'a'+26;
	else l = buff[0] - 'A';
	for (j=0, i=1;j<l;) {
		key[j++] = buff[i];
		i += j;
	}
	key[j] = '\0';
	return key;
}

main(argc,argv)
int argc;
char *argv[];
{
    unsigned i, n;
    char key[80];
    if (argc!=2) {
      puts("Usage: readkey <dawg file>");
      exit(0);
    }
    printf("Key is %s\n",readKey(argv[1]));
}
