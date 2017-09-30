#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char buff[1500];

/* S-coder encryption from Dr Dobbs Jan 1990 */

class Crypto
{
    int crypt_length;		/* Key length			*/
    int	crypt_ptr;		/* Circular pointer into key	*/
    char *cryptext;	/* Key				*/
    void Crypt(char *buf);
  public:
    Crypto(char *key)
	: crypt_length((int)strlen(key)), crypt_ptr(0), cryptext(key) 
    {
    }
    void Encrypt(char *buf, int len);
};

void Crypto::Crypt(char *buf)
{
    *buf ^= (char)(cryptext[crypt_ptr]^(cryptext[0]*crypt_ptr));
    cryptext[crypt_ptr] += ((crypt_ptr < (crypt_length - 1)) ?
    	  cryptext[crypt_ptr+1] : cryptext[0]);
    if (!cryptext[crypt_ptr]) cryptext[crypt_ptr]++;
    if (++crypt_ptr >= crypt_length) crypt_ptr = 0;
}

void Crypto::Encrypt(char *buf, int len)
{
    while (len--) Crypt(buf++);
}

Crypto *scribble = 0;

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
	Crypto *scribble = new Crypto(key);
	while (!feof(thes)) {
		n=fread(buff,sizeof(char),1024,thes);
		for (int k = 0; k < n; k++)
		    scribble->Encrypt(buff+k,1);
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