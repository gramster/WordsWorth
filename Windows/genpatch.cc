#include <sys\types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

// NB - it is probably best to do the optimisation merges in multiple
// passes for each type, doing those that save the most first. 

#if !defined(W16) && !defined(W32)
#include <unistd.h>
#define WMD	"w"
#define RMD	"r"
#else
#define WMD	"wb"
#define RMD	"rb"
#endif

// parts can be disabled for debugging...

#define GENPATCH
#define OPTIMISE

#include "patch.h"

// In this one we assume a file patch consists of only two types of actions -
// copies and inserts. A copy specifies the offset and length of the 
// data to be copied, while an insert gives the length and data to insert.
// By restricting the patch thus, we should be able to create a simple
// general (but not necessarily efficient) algorithm. The cost of a 
// copy should be about 10 bytes, while for an insert it is about 6+len
// bytes.
//
// We walk through the new file. For each position, we find the longest
// common sequence in the old file, add a copy record, and advance (if there
// is no common sequence we add an insert record). At the end we do 
// multiple passes over the records, collapsing pairs of records into insert
// records wherever this lowers the total patch size.
// This is still O(n^2) in the worst case...
//
// This one seems to work OK (although it's slow) so it has been extended to
// do file lists in two trees, and generate EXEC, DELETE, CREATE, OWNER,
// GROUP and PERMS type patch actions as well.

typedef struct _Match
{
    unsigned char type;
    long pos;
    long len;
    struct _Match *next;
} Match;

#define CCOST	9
#define ICOST	5

Match *head = 0, *tail = 0;
unsigned char *oldf = 0, *newf = 0;
long osz, nsz;
mode_t oldmode, newmode;
uid_t olduid, newuid;
gid_t oldgid, newgid;

// Read a file into a memory buffer, returning the buffer

unsigned char *ReadFile(char *fname,
			long &size, mode_t *mode, uid_t *uid, gid_t *gid)
{
    struct stat st;
#if !defined(W16) && !defined(W32)
    if (lstat(fname, &st)< 0) {size = 0; return 0; }
#else
    if (stat(fname, &st)< 0) {size = 0; return 0; }
#endif
    if (mode) *mode = st.st_mode;
    if (uid) *uid = st.st_uid;
    if (gid) *gid = st.st_gid;
    size = st.st_size;
    unsigned char *buf = new unsigned char[size];
    if (buf == 0) { size = -1; return 0; }
    FILE *fp = fopen(fname, RMD);
    long s = size;
    if (fp == 0) goto fail;
#if 1 /*!defined(W16) && !defined(W32) */
    if (fread((char*)buf, 1, size, fp) != size)
	goto fail;
#else
    while (s > 0 && !feof(fp))
    {
        long cc = 16536l;
	if (cc > s) cc = s;
	s -= fread((char*)buf, 1, cc, fp);
    }
    if (s > 0l) goto fail;
#endif
    fclose(fp);
    return buf;
  fail:
    if (fp) fclose(fp);
    delete [] buf;
    return 0;
}

// We use the IP 16-bit checksum */

unsigned short CRC16(unsigned char *buf, int len)
{
    register unsigned long s = 0;
    while (len>1)
    {
	s += (buf[0]<<8) + buf[1];
	buf += 2;
	len -= 2;
    }
    /* on some machines this last byte must be shifted */
    if (len==1) s += (buf[0]<<8);
    while (s & 0xFFFF0000) s = (s & 0xFFFF) + (s>>16);
    return s^0xFFFF;
}

long MatchLen(unsigned char *s1, unsigned char *s2, long max)
{
    long rtn = 0l;
    while (--max >= 0)
	if (s1[rtn] != s2[rtn]) break;
	else rtn++;
    return rtn;
}

void AppendChunk(unsigned char type, long pos, long len)
{
    Match *m = new Match;
    m->type = type;
    m->pos = pos;
    m->len = len;
    m->next = 0;
    if (head == 0)
    {
	tail = head = m;
    }
    else if (type == INSERT_PATCH && tail->type == INSERT_PATCH &&
		pos == (tail->pos + tail->len))
    {
	tail->len += len;
	delete m;
        //printf("Expanded chunk (%c, %ld, %ld)\n", type, pos, len);
	return;
    }
    else
    {
	tail->next = m;
	tail = m;
    }
    //printf("Added chunk (%c, %ld, %ld)\n", type, pos, len);
}

void Walk()
{
    int pct = -1;
    for (long np = 0l; np < nsz; np++)
    {
	int newpct = (np*100) / nsz;
	if (newpct != pct)
	{
	    if (pct >= 0)
		printf("\b\b\b\b");
	    printf("%3d%%", pct = newpct);
	    fflush(stdout);
	}
	long best = 4, bestpos = -1, maxx = nsz-np;
        for (long op = 0; op < (osz-best); op++)
	{
	    if (newf[np] == oldf[op] && newf[np+best] == oldf[op+best])
	    {
		long maxl = osz - op;
		if (maxl > maxx) maxl = maxx;
		long n = MatchLen(newf+np, oldf+op, maxl);
		if (n > best)
		{
		    best = n;
		    bestpos = op;
		}
	    }
	}
	if (bestpos >= 0)
	{
	    AppendChunk(COPY_PATCH, bestpos, best);
	    np += best-1;
	}
	else AppendChunk(INSERT_PATCH, np, 1);
    }
    printf("\b\b\b\b100%%");
    fflush(stdout);
}

void DumpMatch(Match *m)
{
    char act;
    if (m->type == INSERT_PATCH) act = 'I';
    else act = 'C';
    printf ("%c Pos %ld Len %ld", act, m->pos, m->len);
}

void Dump()
{
    Match *m = head;
    while (m)
    {
	DumpMatch(m);
        printf("\n");
	m = m->next;
    }
}

void fatal(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(-1);
}

void useage()
{
    fatal("Useage: genpatch <oldpath> <newpath> <namelistfile>");
}

// There are a few local optimisations that can be made to patches:
//
// * a copy which is next to an insert can be converted to an insert if the
//	copy length is less than 10 (the two adjacent inserts will later
//	be coalesced).
// * two adjacent copies can be converted to an insert if their combined
//	copy lengths are less than 14.
// * adjacent inserts can be merged. The offsets should be for adjacent
//	blocks by the nature of the algorithm (enforced by assert()).

void OptimisePatch()
{
    int change = 1;
    if (head == 0) return;
    while (change) // do multiple passes till no more possibilities
    {
	change = 0;
	long offset = 0;
	Match *pm = head, *m = head->next;
	while (m)
	{
#ifdef DEBUG
	    printf("Offset %ld Cmp [", offset);
	    DumpMatch(pm);
	    printf("] and [");
	    DumpMatch(m);
	    printf("]\n");
#endif
	    if (pm->type == INSERT_PATCH)
	    {
	        if (m->type == INSERT_PATCH) // insert followed by insert
		{
		    assert(m->pos == (pm->pos + pm->len));
		    // merge adjacent inserts...
		    pm->len += m->len;
		    pm->next = m->next;
		    delete m;
		    m = pm;
		    change = 1;
#ifdef DEBUG
		    printf("Merged insert/insert: Result [");
		    DumpMatch(pm);
		    printf("] Save: 5\n");
#endif
		}
		else // insert followed by copy
		{
		    if (m->len < CCOST)
		    {
#ifdef DEBUG
			int save = CCOST-m->len;
#endif
		        pm->len += m->len;
		        pm->next = m->next;
		        delete m;
			m = pm;
		        change = 1;
#ifdef DEBUG
		        printf("Merged insert/copy: Result [");
		        DumpMatch(pm);
		        printf("] Save: %d\n\n", save);
#endif
		    }
		}
	    }
	    else
	    {
	        if (m->type == INSERT_PATCH) // copy followed by insert
		{
		    if (pm->len < CCOST)
		    {
#ifdef DEBUG
			int save = CCOST-pm->len;
#endif
			pm->type = INSERT_PATCH;
		        pm->pos = m->pos - pm->len;;
		        pm->len += m->len;
		        pm->next = m->next;
		        delete m;
			m = pm;
		        change = 1;
#ifdef DEBUG
		        printf("Merged copy/insert: Result [");
		        DumpMatch(pm);
		        printf("] Save: %d\n\n", save);
#endif
		    }
		}
		else // copy followed by copy
		{
		    if ((m->len + pm->len) < (2*CCOST-ICOST))
		    {
#ifdef DEBUG
			int save = (2*CCOST-ICOST)-pm->len-m->len;
#endif
			pm->type = INSERT_PATCH;
		        pm->pos = offset;
		        pm->len += m->len;
		        pm->next = m->next;
		        delete m;
			m = pm;
		        change = 1;
#ifdef DEBUG
		        printf("Merged copy/copy: Result [");
		        DumpMatch(pm);
		        printf("] Save: %d\n\n", save);
#endif
		    }
		}
	    }
	    if (pm != m)
	    {
		offset += pm->len;
	    	pm = m;
	    }
	    m = m->next;
	}
    }
}

long WritePatch(FILE *fp, char *fname)
{
    long patchsize = 0;
    Match *m = head;
    // write the header ID
    unsigned char action = FILE_PATCH;
    fwrite(&action, sizeof(action), 1, fp);
    unsigned short v = CRC16(oldf, osz);
#ifdef DEBUG
    printf("File %s\nOld CRC %u\n", fname, v);
#endif
    fwrite(&v, sizeof(v), 1, fp);
    v = CRC16(newf, nsz);
#ifdef DEBUG
    printf("New CRC %u\n", v);
#endif
    fwrite(&v, sizeof(v), 1, fp);
    v = strlen(fname)+1;
    fwrite(&v, sizeof(v), 1, fp);
    fwrite(fname, sizeof(char), v, fp);
    patchsize = sizeof(char)+3*sizeof(v)+v;
    while (m)
    {
	fwrite(&m->type, sizeof(m->type), 1, fp);
	fwrite(&m->len, sizeof(m->len), 1, fp);
	patchsize += sizeof(m->type) + sizeof(m->len);
	if (m->type == COPY_PATCH)
	{
  	    fwrite(&m->pos, sizeof(m->pos), 1, fp);
	    patchsize += sizeof(m->pos);
	}
	else
	{
	    fwrite(newf+m->pos, sizeof(char), m->len, fp);
	    patchsize += m->len*sizeof(char);
	}
	m = m->next;
    }
    return patchsize;
}

void WriteAttribPatch(FILE *fp, char *name, mode_t mode, uid_t uid, gid_t gid)
{
    unsigned char action = ATTR_PATCH;
    fwrite(&action, sizeof(action), 1, fp);
    unsigned short v = strlen(name)+1;
    fwrite(&v, sizeof(v), 1, fp);
    fwrite(name, sizeof(char), v, fp);
    fwrite(&mode, sizeof(mode), 1, fp);
    fwrite(&uid, sizeof(uid), 1, fp);
    fwrite(&gid, sizeof(gid), 1, fp);
}

void WriteDeletePatch(FILE *fp, char *fname)
{
    unsigned char action = DEL_PATCH;
    fwrite(&action, sizeof(action), 1, fp);
    unsigned short v = strlen(fname)+1;
    fwrite(&v, sizeof(v), 1, fp);
    fwrite(fname, sizeof(char), v, fp);
}

void WriteCreatePatch(FILE *fp, char *fname, unsigned char *data, long sz)
{
    unsigned char action = NEW_PATCH;
    fwrite(&action, sizeof(action), 1, fp);
    unsigned short v = strlen(fname)+1;
    fwrite(&v, sizeof(v), 1, fp);
    fwrite(fname, sizeof(char), v, fp);
    fwrite(&sz, sizeof(sz), 1, fp);
    fwrite(data, sizeof(char), sz, fp);
}

void GenPatch(FILE *fp, char *oldfname, char *newfname, char *targetname)
{
    printf("\nGenerating patch: %s -> %s: ", oldfname, newfname);
    oldf = ReadFile(oldfname, osz, &oldmode, &olduid, &oldgid);
    newf = ReadFile(newfname, nsz, &newmode, &newuid, &newgid);
    if (oldf && newf)
    {
	if (osz != nsz || memcmp(oldf, newf, osz))
	{
	    Walk();
#ifdef DEBUG
	    Dump();
#endif
	    OptimisePatch();
#ifdef DEBUG
	    printf("\n\nAfter optimisation: \n");
	    Dump();
#endif
	    long plen = WritePatch(fp, targetname);
	    printf(" %ld bytes", plen);
	}
	// Compare owners/permissions and generate perm patch if different...
	if (oldmode != newmode || olduid != newuid || oldgid != newgid)
	    WriteAttribPatch(fp, targetname, newmode, newuid, newgid);
    }
    else if (oldf && nsz == 0) // delete file
	WriteDeletePatch(fp, targetname);
    else if (newf && osz == 0) // create new file
    {
	WriteCreatePatch(fp, targetname, newf, nsz);
	// generate owner/perm patch...
	WriteAttribPatch(fp, targetname, newmode, newuid, newgid);
    }
    else printf("Patch generation failed! Out of memory or no such file\n");
    delete [] oldf;
    delete [] newf;
    oldf = newf = 0;
    while (head)
    {
	Match *tmp = head;
	head = head->next;
	delete tmp;
    }
    tail = 0;
}

//-------------------------------------------------------------------------
// Global patch optimiser

typedef struct _ModPatch
{
    unsigned char type;
    long len;
    long origoffset;
    long newoffset;
    long arg;
} ModPatch;

unsigned char GetAction(unsigned char *patch, long &offset, long limit)
{
    unsigned char rtn = patch[offset];
    short s; long l;

// for debug:
    long oo = offset;

#define GETS(n)		(* ((short*)(patch+offset+n)) )
#define GETL(n)		(* ((long*)(patch+offset+n)) )
    switch(rtn)
    {
    case FILE_PATCH: /* start patching a file */
	s = GETS(5);
	offset += 7 + s;
	break;
    case COPY_PATCH: /* copy bytes from old file */
	offset += 9;
	break;
    case INSERT_PATCH: /* insert bytes from patch file */
	l = GETL(1);
	offset += 5 + l;
	break;
    case EXEC_PATCH: /* execute command */
    case DEL_PATCH: /* delete file */
	s = GETS(1);
	offset += 3 + s;
	break;
    case NEW_PATCH: /* create new file */
	s = GETS(1);
	l = GETL(3+s);
	offset += 7+s+l;
	break;
    case ATTR_PATCH: /* set attributes */
	s = GETS(1);
	offset += 3 + s + sizeof(mode_t)+sizeof(uid_t)+sizeof(gid_t);
	break;
    case REF_PATCH:
	offset += 5;
	break;
    default:
	if (IS_ASCII_PATCH(rtn))
	{
	    while (offset < limit && IS_ASCII_PATCH(patch[offset]))
		offset++;
	}
	else offset += 1 + LEN_PATCH_LEN(rtn); // length patch
	break;
    }
    if (offset < 0) // gone mad!
    {
        void DumpPatch(unsigned char *patch, long psz);
	printf("Bad op: offset %ld; next offset %ld\n", oo, offset);
	DumpPatch(patch+oo, limit-oo);
    }
    return rtn;
}

void Optimise(char *infile, char *outfile)
{
    long psz;
    unsigned char *patch = ReadFile(infile, psz, 0, 0, 0);
    if (patch == 0) fatal("Can't open temporary patch file for optimisation");
    long icnt = 0, pp;
    for (pp = 0l; pp < psz; )
	if (GetAction(patch, pp, psz) == INSERT_PATCH)
	    icnt++;
    printf("%ld insert actions\n", icnt);
    ModPatch *mods = new ModPatch[icnt];
    icnt = 0;
    printf("Getting offsets...\n");
    for (pp = 0l; pp < psz; )
    {
	long where = pp;
	if (GetAction(patch, pp, psz) == INSERT_PATCH)
	{
	    mods[icnt].type = INSERT_PATCH;
	    mods[icnt++].origoffset = where;
	}
    }
    long save = 0, i;
    printf("Finding U matches...\n");
    for (i = 0; i < (icnt-1); i++)
    {
	if (mods[i].type != INSERT_PATCH) continue;
	unsigned char *pi = patch+mods[i].origoffset;
	long li = * ((long*)(pi+1));
	mods[i].len = li;
	if (li < 6) continue;
	pi += 5;
	for (long j = i+1; j < icnt; j++)
	{
	    if (mods[j].type != INSERT_PATCH) continue;
	    unsigned char *pj = patch+mods[j].origoffset;
	    long lj = * ((long*)(pj+1));
	    mods[j].len = lj;
	    if (li != lj) continue;
	    pj += 5;
	    if (memcmp(pi, pj, li) == 0)
	    {
		mods[j].type = REF_PATCH;
		mods[j].arg = i;
		save += lj;
	    }
	}
    }
    printf("Save: %ld\n", save);
    printf("Finding ASCII matches...\n");
    save = 0;
    for (i = 0; i < icnt; i++)
    {
	if (mods[i].type ==INSERT_PATCH)
	{
	    int k;
	    unsigned char *pi = patch+mods[i].origoffset+5;
	    for (k = mods[i].len; --k >= 0; )
		if (!IS_ASCII_PATCH(pi[k]))
		    break;
	    if (k < 0)
	    {
		mods[i].type = 0;
		save += 5;
	    }
	}
    }
    printf("Save: %ld\n", save);
    printf("Finding length matches...\n");
    save = 0;
    for (i = 0; i < icnt; i++)
    {
	if (mods[i].type ==INSERT_PATCH && mods[i].len <= MAX_LEN_PATCH_LEN)
	{
	    mods[i].type = LEN_PATCH(mods[i].len);
	    save += 4;
	}
    }
    printf("Save: %ld\n", save);
    // Now write out the new patch
    FILE *ofp = fopen(outfile, WMD);
    if (ofp == 0) fatal("Can't open patch file for output");
    long newoff = 0;
    i = 0;
    for (long oldoff = 0l; oldoff < psz; )
    {
	long where = oldoff;
	if (GetAction(patch, oldoff, psz) == INSERT_PATCH)
	{
	    mods[i].newoffset = newoff;
	    switch (mods[i].type)
	    {
	    case INSERT_PATCH: // just copy thru...
		fwrite(patch+where, sizeof(char), oldoff-where, ofp);
		newoff += oldoff-where;
		break;
	    case REF_PATCH:
		fwrite(&mods[i].type, sizeof(char), 1, ofp);
		fwrite(&mods[mods[i].arg].newoffset, sizeof(long), 1, ofp);
		newoff += 5;
		break;
	    default:
		if (IS_ASCII_PATCH(mods[i].type)) // ASCII patch
		{
		    fwrite(patch+where+5, sizeof(char), oldoff-where-5, ofp);
		    newoff += oldoff-where-5;
		}
		else // length patch
		{
		    fwrite(&mods[i].type, sizeof(char), 1, ofp);
		    fwrite(patch+where+5, sizeof(char), oldoff-where-5, ofp);
		    newoff += oldoff-where-4;
		}
	    }
	    i++;
	}
	// else just copy thru...
	else
	{
	    fwrite(patch+where, sizeof(char), oldoff-where, ofp);
	    newoff += oldoff-where;
	}
	
    }
    fclose(ofp);
    delete [] mods;
    delete [] patch;
}

//-------------------------------------------------------------------------

void DumpPatch(unsigned char *patch, long psz) // need work!
{
    char *fname;
    for (long pp = 0l; pp < psz; )
    {
	long where = pp;
	short oldck, cksum, slen;
	long len;
	FILE *fp;
	unsigned char act = GetAction(patch, pp, psz);
	printf("%ld ", where, act);
#define DS(n)		(* ((short*)(patch+n)) )
#define DL(n)		(* ((long*)(patch+n)) )
        switch(act)
        {
        case FILE_PATCH: /* start patching a file */
	    printf("'F' %d %d %d %s\n",
		((int)DS(where+1)), ((int)DS(where+3)), 
	    	((int)DS(where+5)), patch+where+7);
	    fname = (char*)(patch+where+7);
	    break;
        case COPY_PATCH: /* copy bytes from old file */
	    printf("'C' %ld %ld\n", DL(where+1), DL(where+5));
	    fp = fopen(fname, RMD);
	    if (fp)
	    {
		fseek(fp, DL(where+5), SEEK_SET);
		long l = DL(where+1);
		putchar('[');
		while (--l >= 0)
		    putchar(fgetc(fp));
		putchar(']');
		putchar('\n');
		fclose(fp);
	    }
	    break;
        case INSERT_PATCH: /* insert bytes from patch file */
	    printf("'I' %ld\n", DL(where+1));
	    break;
    	case EXEC_PATCH: /* execute command */
	    printf("'X' %s\n", patch+where+3);
	    break;
        case DEL_PATCH: /* delete file */
	    printf("'D' %s\n", patch+where+3);
	    break;
        case NEW_PATCH: /* create new file */
	    printf("'N' %s\n", patch+where+3);
	    break;
        case ATTR_PATCH: /* set attributes */
	    printf("'A' %s\n", patch+where+3);
	    break;
        case REF_PATCH: /* reference to prev patch */
	    printf("'R' %ld\n", DL(where+1));
	    break;
        default:
	    if (IS_ASCII_PATCH(act))
	    {
	        printf("'\"' %d ", pp-where);
		for (int k = 0; k < (pp-where); k++)
		    printf("%c", patch[where+k]);
		printf("\n");
	    }
	    else
	        printf("'L' %d\n", LEN_PATCH_LEN(act));
	    break;
	}
    }
}

//-------------------------------------------------------------------------

main(int argc, char **argv)
{
#ifdef GENPATCH
    if (argc != 4) useage();
    FILE *fp = fopen("patch.tmp", WMD);
    if (fp == 0) fatal("Can't open patch file");
    FILE *ifp = fopen(argv[3], RMD);
    if (ifp == 0) fatal("Can't open file list");
    while (!feof(ifp))
    {
	char buf[256], ofn[1024], nfn[1024];
	if (fgets(buf, 256, ifp) == 0) break;
	int l = strlen(buf);
	while (l > 0 && buf[l-1] <= 32) l--;
	buf[l] = 0;
	if (strncmp(buf, "exec", 4) == 0 && isspace(buf[4]))
	{
	    // special exec entry - generate X patch command
    	    char action = EXEC_PATCH;
    	    fwrite(&action, sizeof(action), 1, fp);
	    short l = strlen(buf+4)+1;
    	    fwrite(&l, sizeof(l), 1, fp);
    	    fwrite(buf+4, sizeof(char), l, fp);
	}
	else if (buf[0])
	{
	    strcpy(ofn, argv[1]);
#if !defined(W16) && !defined(W32)
	    if (ofn[strlen(ofn)-1] != '/')
	        strcat(ofn, "/");
#else
	    if (ofn[strlen(ofn)-1] != '\\')
	        strcat(ofn, "\\");
#endif
	    strcat(ofn, buf);
	    strcpy(nfn, argv[2]);
#if !defined(W16) && !defined(W32)
	    if (nfn[strlen(nfn)-1] != '/')
	        strcat(nfn, "/");
#else
	    if (nfn[strlen(nfn)-1] != '\\')
	        strcat(nfn, "\\");
#endif
	    strcat(nfn, buf);
	    GenPatch(fp, ofn, nfn, buf);
	}
    }
    printf("\nDone\n");
    fclose(ifp);
    fclose(fp);
#endif
#ifdef DEBUG
    printf("\n\nBEFORE GLOBAL OPTIMISATION:\n\n");
    long psz;
    unsigned char *patch = ReadFile("patch.tmp", psz, 0, 0, 0);
    DumpPatch(patch, psz);
    delete [] patch;
#endif
#ifdef OPTIMISE
    Optimise("patch.tmp", "patch");
#ifdef DEBUG
    printf("\n\nAFTER GLOBAL OPTIMISATION:\n\n");
    patch = ReadFile("patch", psz, 0, 0, 0);
    DumpPatch(patch, psz);
    delete [] patch;
#endif
#ifdef GENPATCH
    unlink("patch.tmp");
#endif
#endif
}

