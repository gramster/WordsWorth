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
#if defined(W16)
#include <malloc.h>
#else
#define huge
#endif

void fatal(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(-1);
}

void useage()
{
    fatal("useage: crc <file>");
}

// We use the IP 16-bit checksum

#ifdef W16

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
    printf("s = %lu\n", s);
    while (s & 0xFFFF0000l)
        s = (s & 0xFFFFl) + (s>>16);
    return (unsigned short)((s & 0xFFFF) ^0xFFFF);
}

#else

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
    printf("s = %lu\n", s);
    while (s & 0xFFFF0000) s = (s & 0xFFFF) + (s>>16);
    return s^0xFFFF;
}

unsigned short CalcCRC(char *fname)
{
    unsigned short v;
    long sz;
    unsigned char *f = ReadFile(fname, sz, 0, 0, 0);
    v = CRC16(f, sz);
    delete [] f;
    return v;
}

#endif

main(int argc, char **argv)
{
    if (argc != 2) useage();
    else printf("%u\n", CalcCRC(argv[1]));
}

