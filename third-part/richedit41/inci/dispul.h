#ifndef DISPUL_DEFINED
#define DISPUL_DEFINED

#include "lsdefs.h"
#include "dispi.h"
#include "plsdnode.h"

/* all we need to remember about a merge is in this structure - enough to draw UL */
/* in case of bad metrics, only first three are filled. */
typedef struct {
	UINT kul;						/* kind of UL-line, simple, wavy, etc, opaque to LS */
    DWORD cNumberOfLines;			/* number of lines in UL: possible values 1,2 */
	LONG vpUnderlineOrigin;			/* UnderlineOrigin position */
	LONG dvpFirstUnderlineOffset;	/* from UnderlineOrigin position */
	LONG dvpFirstUnderlineSize;		
	LONG dvpGapBetweenLines;		
	LONG dvpSecondUnderlineSize;	
} LSULMETRIC;

/* all we need to remember - enough to draw SS */
typedef struct {
	UINT kul;				/* kind of SS-line, same as UL-types */
    DWORD cNumberOfLines;	/* number of lines in SS: possible values 1,2 */
	LONG dvp1stSSSize;		/* SS line width */
	LONG dvp1stSSOffset;	/* position relative to the baseline, filled page direction (normally > 0) */
	LONG dvp2ndSSSize;		/* 1stSS is common for single and double SS, save space */
	LONG dvp2ndSSOffset;	/* normally dvp1stSSOffset < dvp2ndSSOffset */
} LSSTRIKEMETRIC;

LSERR GetUnderlineMergeMetric(
				PLSC, 
				PLSDNODE, 			/* dnode to start UL merging */
				POINTUV,			/* starting pen point (u,v) */
				LONG,				/* upLimUnderline */
				LSTFLOW, 
				LSCP, 				/* cpLimBreak */
				LSULMETRIC*,	 	/* merge metrics */
				int*, 				/* nodes to display in the merge */
				BOOL*);				/* good metrics? */

LSERR DrawUnderlineMerge(
				PLSC, 
				PLSDNODE, 			/* dnode to start underlining */
				const POINT*, 		/* pptOrg (x,y) */
				int, 				/* nodes to display in the merge */
				LONG,				/* upStartUnderline */
				BOOL,				/* good metrics? */
				const LSULMETRIC*, 	/* merge metrics */
				UINT, 				/* kDisp : transparent or opaque */
				const RECT*, 		/* &rcClip: clipping rect (x,y) */
				LONG,				/* upLimUnderline */
				LSTFLOW); 


LSERR GetStrikeMetric(
				PLSC,
				PLSDNODE, 			/* dnode to strike */
				LSTFLOW, 
				LSSTRIKEMETRIC*,	/* strike metrics */
				BOOL*);				/* good metrics? */

LSERR StrikeDnode(PLSC,
				PLSDNODE, 				/* dnode to start underlining */
				const POINT*, 			/* pptOrg (x,y) */
				POINTUV,				/* starting pen point (u,v) */
				const LSSTRIKEMETRIC*,	/* merge metrics */
				UINT, 					/* kDisp : transparent or opaque */
				const RECT*, 			/* &rcClip: clipping rect (x,y) */
				LONG,					/* upLimUnderline */
				LSTFLOW); 

#endif /* ndef DISPUL_DEFINED */
