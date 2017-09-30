/*
 * The DAWG definitions
 *
 * (c) Graham Wheeler, 1993-1995
 */

#ifndef _DICT_H
#define _DICT_H

typedef unsigned long NODE;	/* dictionary nodes			*/

#ifdef DEMO_VERSION
#  define MAXNODES	23000	/* demo version has small dictionary	*/
   typedef unsigned short nodenum;
#else
# ifdef BIGDICT
#  define MAXNODES	91000l	/* big dictionary			*/
   typedef unsigned long nodenum;
# else
#  define MAXNODES	65000	/* 16-bit index dictionary		*/
   typedef unsigned short nodenum;
# endif
#endif
#define MAXADDNODES	500

#ifdef UNIX
#  include <malloc.h>
#  define huge
#  define farcalloc	calloc
#  define farfree	free
#  define farcoreleft()	0l
#else
#  include <alloc.h>
#endif

extern char dictName[64];
extern short abortMatch;

extern NODE huge *Edges;		/* dictionary nodes	*/
extern NODE huge *XEdges;		/* user dictionary	*/

#define Nodes(n)  (*(Edges+n))
#define XNodes(n)  (*(XEdges+n))

/* Node bit layout is now:
	MLTIIIIINNNNNNNNNNNNNNNNNNNNNNNN
   where I is the index, M the mark, L the last child flag,
   T the terminal flag, and N the next node index
*/

/*
 * The following macros used to work on Nodes; they
 * now just work on values. This means we can copy a
 * node to a local temp var and access that, which will
 * be useful if the dictionary ends up in high memory.
 */

#ifdef BIGDICT
#define NextNode(v)	(v & 0xFFFFFFL)		/* 24-bit indices */
#else
#define NextNode(v)	(v & 0xFFFF)		/* 16-bit indices */
#endif
#define NodeIndex(v)	((v >> 24) & 0x1F)
#define NodeChar(v)	(NodeIndex(v)+'A'-1)
#define IsWordEnd(v)	((v >> 29) & 1)
#define IsLastChild(v)	((v >> 30) & 1)
#define IsMarked(v)	((v >> 31) & 1)

/*
 * These still work on nodes, as they must set the actual node
 *	and not a local copy.
 */

#define MarkWordEnd(n)	Nodes(n) |= 0x20000000UL
#define Mark(n)		Nodes(n) |= 0x80000000UL
#define Unmark(n)	Nodes(n) &= 0x7FFFFFFFUL

extern nodenum NumDictNodes;

/* prototypes from dict.c */

extern int	loadDawg(char *key);
extern void	freeDict(void);
extern int	lookup(char *word);
extern void	exclude(char *word);
extern int	excluded(char *word);
extern void	matchPattern(char *pat, int anagrams, int allLengths,
                  int repeats, int showThink);

#endif _DICT_H

