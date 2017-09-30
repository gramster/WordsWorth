#include <stdio.h>
#include <string.h>

#define THASH		64000	// topic hash bit vector

//---------------------------------------------------------------------
// simple incidence hash table

class Hash
{
    int sz;
    unsigned short *vector;
    long collisions;
    long words;
    int Key(char *word);
  public:
    Hash(long size);
    void Empty();
    void Add(char *word);
    int Lookup(char *word);
    ~Hash();
};

Hash::Hash(long size)
{
    sz = (size+15l)/16l;
    vector = new unsigned short[sz];
    Empty();
}

void Hash::Empty()
{
    memset(vector, 0, sz*sizeof(short));
    collisions = words = 0;
}

int Hash::Key(char *word)
{
    unsigned long h = 0x0;
    while (*word)
	h = (h*26l + (*word++) - 'A') % 0x7FFFFFFEl;
    return h % (16*sz);
}

void Hash::Add(char *word)
{
    int k = Key(word);
    if ((vector[k/16] & (1 << (k%16))) != 0) collisions++;
    else vector[k/16] |= (1 << (k%16));
    words++;
}

int Hash::Lookup(char *word)
{
    int k = Key(word);
    return ((vector[k/16] & (1 << (k%16))) != 0);
}

Hash::~Hash()
{
    printf("Words %ld Collisions %ld\n", words, collisions);
    delete [] vector;
}

main(int argc, char **argv)
{
    Hash *h = new Hash(THASH);
    FILE *fp = fopen(((argc==2) ? argv[1] : "wwmed.txt"), "r");
    if (fp)
    {
        while (!feof(fp))
	{
	    char buf[80];
	    if (fgets(buf, 80, fp)==0) break;
	    buf[strlen(buf)-1] = 0;
	    h->Add(buf);
	}
	fclose(fp);
    }
    delete h;
}




