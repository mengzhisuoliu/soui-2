#ifndef ROBJ_DEFINED
#define ROBJ_DEFINED

#include "lsimeth.h"

#define REVERSE_VERSION 0x300

/* Prototype for Reverse Object enumeration callback */
typedef LSERR (WINAPI * PFNREVERSEENUM)(
	POLS pols,				/*(IN): client context */
	PLSRUN plsrun,			/*(IN): from DNODE */
	PCLSCHP plschp,			/*(IN): from DNODE */
	LSCP cp,				/*(IN): from DNODE */
	LSDCP dcp,				/*(IN): from DNODE */
	LSTFLOW lstflow,		/*(IN): text flow */
	BOOL fReverse,			/*(IN): enumerate in reverse order */
	BOOL fGeometryNeeded,	/*(IN): */
	const POINT* pt,		/*(IN): starting position (top left), iff fGeometryNeeded */
	PCHEIGHTS pcheights,	/*(IN): from DNODE, relevant iff fGeometryNeeded */
	LONG dupRun,			/*(IN): from DNODE, relevant iff fGeometryNeeded */
	LSTFLOW lstflowSubline,	/*(IN): lstflow of subline in reverse object */
	PLSSUBL plssubl);		/*(IN): subline in reverse object. */

/*
 *
 *	Reverse Object initialization data that the client application must return
 *	when the Reverse Object handler calls the GetObjectHandlerInfo callback.
 *
 */


/* Prototype for Reverse Object get info */

typedef LSERR (WINAPI * PFNREVERSEGETINFO)
(
	POLS	pols,
	LSCP	cp,
	PLSRUN	plsrun,

	BOOL	* pfDoNotBreakAround,
	BOOL	* pfSuppressTrailingSpaces
);

typedef struct REVERSEINIT
{
        DWORD					dwVersion;		/* Version. Must be REVERSE_VERSION */
        WCHAR					wchEndReverse;	/* Escape char for end of Reverse Object */

		WCHAR					wchUnused1;		/* Unused for alignment */
		PFNREVERSEGETINFO		pfnGetRobjInfo;	/* Callback GetInfo */
		PFNREVERSEENUM			pfnEnum;		/* Enumeration callback */

} REVERSEINIT;

LSERR WINAPI LsGetReverseLsimethods(
        LSIMETHODS *plsim);

/* GetReverseLsimethods
 *
 *	plsim (OUT): Reverse Object Handler methods for Line Services.
 *
 */

#endif /* ROBJ_DEFINED */

