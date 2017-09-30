#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if !defined(W32) && !defined(W16)
#include <unistd.h>
#include <sys/errno.h>
#define WMD	"w"
#define RMD	"r"
#define DSEPCH	'/'
#define DSEPSTR	"/"
#else
#include <errno.h>
#include <io.h>
#include <dir.h>
#define WMD	"wb"
#define RMD	"rb"
#define DSEPCH	'\\'
#define DSEPSTR	"\\"
#endif
#if defined(W16) || defined(DOS)
#include <malloc.h>
#else
#define huge
#endif

#include "patch.h"

FILE *logfp = 0;
int errcnt = 0;
char *logfname = 0;

void finish(int rtn)
{
    if (logfp) fclose(logfp);
    if (errcnt == 0) unlink(logfname);
    else printf("Errors occurred: see transcript in %s\n", logfname);
    exit(rtn);
}

void fatal(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    if (logfp) fprintf(logfp, "%s\n", msg);
    errcnt++;
    finish(-1);
}

void useage()
{
    fatal("useage: dopatch <patchfile>");
}

int MakePath(char *fname)
{
    char *s = fname, ch;
    while ((s = strchr(s, DSEPCH)) != 0)
    {
	ch = *s;
	*s = 0;
	if (access(fname, 0) != 0)
#if !defined(W32) && !defined(W16)
	    if (mkdir(fname, 0755)<0)
#else
	    if (mkdir(fname)<0)
#endif
		return -1;
	*s++ = ch;
    }
    return 0;
}

// We use the IP 16-bit checksum

unsigned short CalcCRC(char *fname)
{
    FILE *fp = fopen(fname, RMD);
    unsigned long s = 0l;
    unsigned short crc;
    if (fp == 0)
    { 
	printf("Can't read file %s\n", fname);
	return -1;
    }
    while (!feof(fp))
    {
        unsigned char buf[512], *p;
	int n = fread((char*)p = buf, 1, 512, fp);
	while (n>1)
	{
    	    unsigned short ss = ((p[0]<<8) + p[1]);
	    s += (unsigned long)ss;
	    p += 2;
	    n -= 2;
    	}
	/* on some machines this last byte must be shifted */
        if (n==1) s += (unsigned long)(p[0]<<8);
    }
    fclose(fp);
    while (s & 0xFFFF0000l)
        s = (s & 0xFFFFl) + (s>>16);
    return (unsigned short)((s & 0xFFFF) ^0xFFFF);
}

int CheckCRC(char *fname, unsigned short cksum)
{
    unsigned short crc = CalcCRC(fname);
    if (cksum == crc)
	return 0;
    else
    {
        printf("Bad patch: expected CRC %u, got CRC %u\n", cksum, crc);
        return -1;
    }
}

int CopyFile(char *inname, char *outname)
{
    FILE *ofp = fopen(outname, WMD);
    if (ofp == 0) return -1;
    FILE *ifp = fopen(inname, RMD);
    if (ifp == 0) return -1;
    while (!feof(ifp))
    {
	char buf[1024];
	int n = fread(buf, sizeof(char), sizeof(buf), ifp);
	if (n > 0) fwrite(buf, sizeof(char), n, ofp);
    }
    fclose(ifp);
    fclose(ofp);
    return 0;
}

void Close(char *ptm,
	   char *&fname, FILE *&oldfp, FILE *&newfp, unsigned short cksum)
{
    if (oldfp) { fclose(oldfp); oldfp = 0; }
    if (newfp) { fclose(newfp); newfp = 0; }
    if (fname)
    {
//        printf("Closing [%s]\n", fname);
	// do checksum confirm and then rename...
//        printf("Checking [%s] checksum\n", ptm);
	if (CheckCRC(ptm, cksum)==0)
	{
	    if (rename(ptm, fname) == 0 || CopyFile(ptm, fname)==0)
	    {
		printf("%s patched successfully\n", fname);
		if (logfp)
		    fprintf(logfp, "%s patched successfully\n", fname);
	    }
	    else
	    {
		errcnt++;
		printf("Patch of %s failed: bad CRC in patched file\n", fname);
		if (logfp) 
		    fprintf(logfp, "Patch of %s failed: bad CRC in patched file\n", fname);
	    }
	}
        else printf("Final checksum doesn't match! Unpatching [%s]\n", fname);
	delete [] fname;
	fname = 0;
    }
    //unlink(ptm);
}

int DoPatch(FILE *patchfp, FILE *&newfp, FILE*&oldfp, char *&fname, char *pf,
		unsigned short &cksum, char *path, int recurse = 0)
{
    static int ignore = 0;
    unsigned char action;
    if (fread(&action, sizeof(action), 1, patchfp)<=0) return -1;
    int pathlen = path ? (strlen(path)+1) : 0, n;
    long len, pos;
    unsigned short oldck, slen, where;
    FILE *nfp;
  nextaction:
//    printf("%ld Action %d\n", ftell(patchfp)-1, action);
    switch(action)
    {
    case FILE_PATCH: /* start patching a file */
	Close(pf, fname, oldfp, newfp, cksum);
	ignore = 0;
	if (fread(&oldck, sizeof(oldck), 1, patchfp)<=0) return -2;
	if (fread(&cksum, sizeof(cksum), 1, patchfp)<=0) return -2;
	if (fread(&slen, sizeof(slen), 1, patchfp)<=0) return -2;
	fname = new char[pathlen+slen];
	if (fname == 0) fatal("Out of memory");
	if (path) { strcpy(fname, path); strcat(fname, "/"); }
	if (fread(fname+pathlen, sizeof(char), slen, patchfp)<=0) return -2;
	printf("Patching [%s]\n", fname);
	if (logfp) fprintf(logfp, "Patching [%s]\n", fname);
	if (CheckCRC(fname, oldck) != 0)
	{
	    if (CheckCRC(fname, cksum) != 0)
	    {
		errcnt++;
		printf("Can't patch %s; bad CRC\n", fname);
	        if (logfp)
		    fprintf(logfp, "Can't patch %s; bad CRC\n", fname);
	    }
	    else
	    {
		printf("It looks like it's been patched; skipping...\n");
	        if (logfp)
		    fprintf(logfp, "It looks like it's been patched; skipping...\n");
	    }
	    ignore = 1;
	    delete [] fname;
	    fname = 0;
	    break;
	}
	else
	{
//	    printf("CRC is as expected\n");
	    if (logfp) fprintf(logfp, "CRC is as expected\n");
	}
	oldfp = fopen(fname, RMD);
    	if (oldfp == 0)
	    fatal("Can't open file to be patched");
    	newfp = fopen(pf, WMD);
    	if (newfp == 0)
	    fatal("Can't open patched file for output");
	break;
    case COPY_PATCH: /* copy bytes from old file */
	if (fread(&len, sizeof(len), 1, patchfp)<=0) return -2;
	if (fread(&pos, sizeof(pos), 1, patchfp)<=0) return -2;
//      printf("C %ld %ld\n", pos, len);
	if (ignore) break;
	fseek(oldfp, pos, SEEK_SET);
    	while (!feof(oldfp) && --len >= 0)
	    (void)fputc(fgetc(oldfp), newfp);
	break;
    case INSERT_PATCH: /* insert bytes from patch file */
	if (fread(&len, sizeof(len), 1, patchfp)<=0) return -2;
//      printf("I %ld\n", len);
    	while (!feof(patchfp) && --len >= 0)
	{
	    char c = fgetc(patchfp);
	    if (!ignore) fputc(c, newfp);
	}
	break;
    case EXEC_PATCH: /* execute command */
	Close(pf, fname, oldfp, newfp, cksum);
	ignore = 0;
	if (fread(&slen, sizeof(slen), 1, patchfp)<=0) return -2;
	else
	{
	    char *cmd = new char[slen];
	    if (cmd == 0 ||
	        fread(cmd, sizeof(char), slen, patchfp)<=0) return -2;
	    printf("Executing %s\n", cmd);
	    if (logfp) fprintf(logfp, "Executing %s\n", cmd);
	    system(cmd);
	    delete [] cmd;
	}
	break;
    case DEL_PATCH: /* delete file */
	Close(pf, fname, oldfp, newfp, cksum);
	ignore = 0;
	if (fread(&slen, sizeof(slen), 1, patchfp)<=0) return -2;
	fname = new char[pathlen+slen];
	if (fname == 0) fatal("Out of memory");
	if (path) { strcpy(fname, path); strcat(fname, "/"); }
	if (fread(fname, sizeof(char), slen, patchfp)<=0) return -2;
	printf("Deleting %s\n", fname);
	if (logfp) fprintf(logfp, "Deleting %s\n", fname);
	unlink(fname);
	delete [] fname;
	fname = 0;
	break;
    case NEW_PATCH: /* create new file */
	Close(pf, fname, oldfp, newfp, cksum);
	ignore = 0;
	if (fread(&slen, sizeof(slen), 1, patchfp)<=0) return -2;
	fname = new char[pathlen+slen];
	if (fname == 0) fatal("Out of memory");
	if (path) { strcpy(fname, path); strcat(fname, "/"); }
	if (fread(fname, sizeof(char), slen, patchfp)<=0) return -2;
	if (fread(&len, sizeof(len), 1, patchfp)<=0) return -2;
	printf("Creating %s (%ld bytes)\n", fname, len);
	if (logfp) fprintf(logfp, "Creating %s (%ld bytes)\n", fname, len);
	MakePath(fname);
	nfp = fopen(fname, WMD);
	if (nfp)
	{
	    while (len-- > 0) fputc(fgetc(patchfp), nfp);
	    fclose(nfp);
	}
	else fatal("Failed to create file");
	break;
    case ATTR_PATCH: /* set attributes */
	Close(pf, fname, oldfp, newfp, cksum);
	ignore = 0;
	if (fread(&slen, sizeof(slen), 1, patchfp)<=0) return -2;
	fname = new char[pathlen+slen];
	if (fname == 0) fatal("Out of memory");
	if (path) { strcpy(fname, path); strcat(fname, "/"); }
	if (fread(fname, sizeof(char), slen, patchfp)<=0) return -2;
	else
	{
#if !defined(W16) && !defined(DOS)
	    mode_t mode;
	    uid_t uid;
	    gid_t gid;
#else
	    short mode;
	    long uid;
	    long gid;
#endif
	    if (fread(&mode, sizeof(mode), 1, patchfp)<=0) return -2;
	    if (fread(&uid, sizeof(uid), 1, patchfp)<=0) return -2;
	    if (fread(&gid, sizeof(gid), 1, patchfp)<=0) return -2;
#if !defined(W16) && !defined(DOS)
	    chmod(fname, mode);
	    chown(fname, uid, gid);
#endif
	    delete [] fname;
	    fname = 0;
	}
	break;
    case REF_PATCH: /* reference to prev patch */
	if (fread(&len, sizeof(len), 1, patchfp)<=0) return -2;
	where = ftell(patchfp);
	fseek(patchfp, len, SEEK_SET);
	n = DoPatch(patchfp, newfp, oldfp, fname, pf, cksum, path, 1);
	if (n < 0) return n;
	fseek(patchfp, where, SEEK_SET);
	break;
    default:
	if (IS_ASCII_PATCH(action))
	{
	    while (IS_ASCII_PATCH(action))
	    {
		if (!ignore) fputc(action, newfp);
    		if (fread(&action, sizeof(action), 1, patchfp)<=0) return -1;
	    }
	    if (!recurse) goto nextaction;
	}
	else
	{
	    int l = LEN_PATCH_LEN(action);
    	    while (!feof(patchfp) && --l >= 0)
	    {
		char c = fgetc(patchfp);
		if (!ignore) fputc(c, newfp);
	    }
	}
	break;
    }
    return 0;
}

main(int argc, char **argv)
{
#if !defined(W16) && !defined(W32)
    char ptm[20], ltm[20];
    strcpy(ptm, "/tmp/patXXXX");
    strcpy(ltm, "/tmp/plgXXXX");
    char *pf = mktemp(ptm);
    logfname = mktemp(ltm);
#else
    char *pf = "patch.tmp";
    logfname = "patch.log";
#endif
    logfp = fopen(logfname, "w");
    char *fname = 0;

    unsigned short cksum;
    if (argc != 2 && argc != 3) useage();
    FILE *newfp = 0, *oldfp = 0, 
        *patchfp = fopen(argv[1], RMD);
    if (patchfp == 0) fatal("Can't open patch file");
    char *path = (argc==3) ? argv[2] : 0;
    for (;;)
    {
	int n = DoPatch(patchfp, newfp, oldfp, fname, pf, cksum, path);
	if (n == -2) goto bad;
	else if (n < 0) break;
    }
    Close(pf, fname, oldfp, newfp, cksum);
    fclose(patchfp);
#if defined(W16) || defined(W32)
    unlink("patch.tmp");
#endif
    finish(0);
 bad:
#if defined(W16) || defined(W32)
    unlink("patch.tmp");
#endif
    fatal("Bad patchfile!");
}

