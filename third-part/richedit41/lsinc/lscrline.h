#ifndef LSCRLINE_DEFINED
#define LSCRLINE_DEFINED

#include "lsdefs.h"
#include "plsline.h"
#include "breakrec.h"
#include "lslinfo.h"

LSERR WINAPI LsCreateLine(PLSC,				/* IN: ptr to line services context		*/
						  LSCP,				/* IN: cpFirst							*/
						  LONG,				/* IN: duaColumn						*/
						  const BREAKREC*,	/* IN: input array of break records		*/
						  DWORD,			/* IN: number of records in input array	*/
						  DWORD,			/* IN: size of the output array			*/
						  BREAKREC*,		/* OUT: output array of break records	*/
						  DWORD*,			/* OUT:actual number of records in array*/
						  LSLINFO*,			/* OUT: visible line info				*/
						  PLSLINE*);		/* OUT: ptr to line opaque to client	*/

LSERR WINAPI LsModifyLineHeight(PLSC,		/* IN: ptr to line services context 	*/
								PLSLINE,	/* IN: ptr to line -- opaque to client	*/
								LONG,		/* IN: dvpAbove							*/
								LONG,		/* IN: dvpAscent						*/
								LONG,		/* IN: dvpDescent						*/	
								LONG);		/* IN: dvpBelow							*/	

LSERR WINAPI LsDestroyLine(PLSC,			/* IN: ptr to line services context		*/
						   PLSLINE);		/* IN: ptr to line -- opaque to client	*/

LSERR WINAPI LsGetLineDur(PLSC,				/* IN: ptr to line services context 	*/
						  PLSLINE,			/* IN: ptr to line -- opaque to client	*/
						  LONG*,			/* OUT: dur of line incl. trailing area	*/
						  LONG*);			/* OUT: dur of line excl. trailing area	*/

LSERR WINAPI LsGetMinDurBreaks(PLSC,		/* IN: ptr to line services context 	*/
						  	   PLSLINE,		/* IN: ptr to line -- opaque to client	*/
						  	   LONG*,		/* OUT: min dur between breaks including 
																	trailing area	*/
						  	   LONG*);		/* OUT: min dur between breaks excluding 
																	trailing area	*/

#endif /* !LSCRLINE_DEFINED */
