#ifndef DNUTILS_DEFINED
#define DNUTILS_DEFINED

#include "lsdefs.h"
#include "plsdnode.h"
#include "objdim.h"
#include "lscbk.h"
#include "plsiocon.h"
#include "pqheap.h"



/* ROUTINES ---------------------------------------------------------------------------*/


LSERR FindListDims(PLSDNODE, PLSDNODE, OBJDIM*);

void FindListDup(PLSDNODE, LSCP, LONG*);

void FindListFinalPenMovement(PLSDNODE plsdnFirst, PLSDNODE plsdnLast, LONG *pdur, LONG *pdvr, LONG *pdvp);

LSERR DestroyDnodeList(LSCBK*, POLS, PLSIOBJCONTEXT, PLSDNODE plsdn, BOOL fDontReleaseRuns);

LONG DurBorderFromDnodeInside(PLSDNODE plsdn); /* IN: dnode inside borders */

BOOL FSpacesOnly(PLSDNODE plsdn, DWORD iObjText);

#define MovePointBack(ptpen, dur, dvr) \
		(ptpen)->u -= (dur); \
		(ptpen)->v -= (dvr); 


#define  GetPointBeforeDnodeFromPointAfter(pnode, ptpen) \
	MovePointBack(ptpen, DurFromDnode(pnode), DvrFromDnode(pnode));


#endif /* DNUTILS_DEFINED */

