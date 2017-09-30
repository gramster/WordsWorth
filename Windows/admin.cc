#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blowfish.h"
#include "encode.h"

const unsigned char *DemoKey = "+\\ke;*^23!x$"; // only 64-bits are used

const unsigned char *BigKey = "6g$#2[]';"; // only 64-bits are used

static void EncipherFile(const char *fromname, const char *toname, 
			 const unsigned char *key, int keybytes)
{
    FILE *fromfp = fopen(fromname, "rb");
    FILE *tofp = fopen(toname, "wb");
    if (fromfp && tofp)
    {
    	Blowfish *b = new Blowfish;
    	if (b && b->SetKey(key, keybytes))
    	{
	    unsigned long buf[2];
	    while (!feof(fromfp))
	    {
	    	int n = fread(buf, sizeof(long), 2, fromfp);
	    	if (n > 0)
	    	{
	            b->EncipherBlock(&buf[0], &buf[1]);
		    fwrite(buf, sizeof(long), 2, tofp);
	    	}
	    	if (n != 2) break;
	    }
    	}
        delete b;
    }
    if (fromfp) fclose(fromfp);
    if (tofp) fclose(tofp);
}

void MakeDemoDictionary()
{
    // Blowfish encipher master.dem dict with key DemoKey()
    // to create file wwdemo.dic
    EncipherFile("master.dem", "wwdemo.dic", DemoKey, 8);
}

void MakeBigDictionary()
{
    // Blowfish encipher master.dem dict with key BigKey
    // to create file wwbig.dic
    EncipherFile("master.big", "wwbig.dic", BigKey, 8);
}

void Print(char *title, const unsigned char *s, int len)
{
    printf("%s = ", title);
    while (--len >= 0)
        printf(" 0x%02X", (unsigned)(*s++));
    printf("\n");
}

char *MakeWRegKey(char *user, char *email)
{
    unsigned char rv[10], bkey[8];
    unsigned short h = ComputeHash(user, email, BigKey);
//Print("BigKey", BigKey, 8);
//printf("Hash is %u (0x%04x)\n", h, h);
    memcpy(bkey, BigKey, 8);
    Blowfish *b = new Blowfish;
    char k[128];
    strcpy(k, user);
    strcat(k, email);
    if (b && b->SetKey(k, strlen(k)))
        b->Encipher(bkey, 8);
    else
        printf("Couldn't make blowfish!!\n");
    delete b;
//Print(" Encrypted BKey", bkey, 8);
    rv[0] = h>>8;
    rv[6] = h;
    rv[8] = bkey[0];
    rv[2] = bkey[1];
    rv[9] = bkey[2];
    rv[1] = bkey[3];
    rv[4] = bkey[4];
    rv[3] = bkey[5];
    rv[7] = bkey[6];
    rv[5] = bkey[7];
//Print("RV", rv, 10);
    static char key[22];
    Encode(key, (unsigned long *)rv, 80);
//printf("Encoded = %s\n", key);
    return key;
}

char *MakeXRegKey(char *user, char *email)
{
    unsigned char rv[10], bkey[8];
    unsigned short h = ComputeHash(user, email, BigKey);
//Print("BigKey", BigKey, 8);
//printf("Hash is %u (0x%04x)\n", h, h);
    memcpy(bkey, BigKey, 8);
    Blowfish *b = new Blowfish;
    char k[128];
    strcpy(k, email);
    strcat(k, user);
    if (b && b->SetKey(k, strlen(k)))
        b->Encipher(bkey, 8);
    else
        printf("Couldn't make blowfish!!\n");
    delete b;
//Print(" Encrypted BKey", bkey, 8);
    rv[0] = h>>8;
    rv[6] = h;
    rv[8] = bkey[0];
    rv[2] = bkey[1];
    rv[9] = bkey[2];
    rv[1] = bkey[3];
    rv[4] = bkey[4];
    rv[3] = bkey[5];
    rv[7] = bkey[6];
    rv[5] = bkey[7];
//Print("RV", rv, 10);
    static char key[22];
    Encode(key, (unsigned long *)rv, 80);
//printf("Encoded = %s\n", key);
    return key;
}

unsigned char *Key2Vec(const char *key)
{
    static unsigned char regvec[16];
    Decode(key, (unsigned long *)regvec, 80);
Print("Decoded key ", regvec, 10);
    return regvec;
}

unsigned char *RegKey(const unsigned char *rv, const char *user, 
				const char *email)
{
    static unsigned char key[8];
    key[0] = rv[8];
    key[1] = rv[2];
    key[2] = rv[9];
    key[3] = rv[1];
    key[4] = rv[4];
    key[5] = rv[3];
    key[6] = rv[7];
    key[7] = rv[5];
//Print("Key before decipher ", key, 8);
    Blowfish *b = new Blowfish;
    char k[128];
    strcpy(k, user);
    strcat(k, email);
    if (b && b->SetKey(k, strlen(k)))
	b->Decipher(key, 8);
    delete b;
//Print("Key after decipher ", key, 8);
    return key;
}

unsigned char *RegisteredKey(char *rkey, char *user, char *email)
{
    unsigned char *rv = Key2Vec(rkey);
//Print("Key2Vector returns ", rv, 10);
    return RegKey(rv, user, email);
}

int VerifyWKey(char *user, char *email, char *key)
{
    // verify the key by extracting the hash and
    // big dict key and comparing them
Print("Actual big dict key", BigKey, 8);
    const unsigned char *rk = RegisteredKey(key, user, email);
Print(" Extracted big dict key", rk, 8);
    unsigned short h = ComputeHash(user, email, BigKey);
printf("Actual hash is %u (0x%04x)\n", h, h);
    unsigned short h2 = GetHash(Key2Vector(key));
printf("Extracted hash is %u (0x%04x)\n", h2, h2);
    return (h == h2 && memcmp(rk, BigKey, 8)==0);
}

int VerifyXKey(char *user, char *email, char *key)
{
    // verify the key by extracting the hash and
    // big dict key and comparing them
Print("Actual big dict key", BigKey, 8);
    const unsigned char *rk = RegisteredKey(key, email, user);
Print(" Extracted big dict key", rk, 8);
    unsigned short h = ComputeHash(user, email, BigKey);
printf("Actual hash is %u (0x%04x)\n", h, h);
    unsigned short h2 = GetHash(Key2Vector(key));
printf("Extracted hash is %u (0x%04x)\n", h2, h2);
    return (h == h2 && memcmp(rk, BigKey, 8)==0);
}

void useage()
{
    fprintf(stderr, "Useage: admin [-R <user> <e-mail>] [-B] [-D]\n");
    exit(-1);
}

main(int argc, char **argv)
{ 
#if 0
    char key[16], regvec[12];
    strcpy(regvec, "abcdefghij");
    Encode(key, regvec, 80);
    regvec[0]=0;
    Decode(key, regvec, 80);
    regvec[10]=0;
    puts(regvec);
    exit(0);
#endif

    if (argc < 2) useage();
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0]=='-')
	{
	    switch (argv[i][1])
	    {
	    case 'B':
		MakeBigDictionary();
		break;
	    case 'D':
		MakeDemoDictionary();
		break;
	    case 'R':
	        if (i < (argc-2))
		{
		    char *key = MakeWRegKey(argv[i+1], argv[i+2]);
		    printf("User: \"%s\"\nE-Mail: \"%s\"\nWKey: \"%s\"\n",
				argv[i+1], argv[i+2], key);
		    VerifyWKey(argv[i+1], argv[i+2], key);
		    key = MakeXRegKey(argv[i+1], argv[i+2]);
		    printf("XKey: \"%s\"\n", key);
		    VerifyXKey(argv[i+1], argv[i+2], key);
		    i += 2;
		}
		else useage();
		break;
	    default:
	        useage();
	    }
	}
	else useage();
    }
}


