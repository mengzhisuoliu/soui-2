#ifndef LSFGI_DEFINED
#define LSFGI_DEFINED

#include "lsdefs.h"
#include "lstflow.h"
#include "plsfgi.h"

/* ------------------------------------------------------------------------ */

struct lsfgi							/* Formatter geometry input */
{
	BOOL fFirstOnLine;	/* REVIEW sergeyge(elik): Query instead of this member? */
	LSCP cpFirst;
	LONG urPen,vrPen;
	LONG urColumnMax;
	LSTFLOW lstflow;
};
typedef struct lsfgi LSFGI;

#endif /* !LSFGI_DEFINED */
