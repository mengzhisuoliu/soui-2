/*
 *	@doc	INTERNAL
 *
 *	@module	COLEOBJ.CPP	OLE Object management class implemenation |
 *
 * 	Authors:
 *	alexgo  Much of this code is a port from RichEdit 1.0 sources
 *			(cleaned up a bit, ported to C++, etc.)  So if there's any
 *			bit of strangeness, it's probably there for a reason.
 *
 *	KeithCu	Cleaned up (removed _rcPos, ResetPosRect(), ScrollObject,
			fixed undo/resizing issues)
 *			Added textflow support
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_coleobj.h"
#include "_objmgr.h"
#include "_select.h"
#include "_rtext.h"
#include "_disp.h"
#include "_dispprt.h"
#include "_antievt.h"
#include "_dxfrobj.h"

ASSERTDATA

// 
// data private to this file
//
static const OLECHAR szSiteFlagsStm[] = OLESTR("RichEditFlags");	

// 
// EXCEL clsid's.  We have to make some special purpose hacks
// for XL.
const CLSID rgclsidExcel[] =
{
    { 0x00020810L, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46} },  // Excel Worksheet
    { 0x00020811L, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46} },  // Excel Chart
    { 0x00020812L, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46} },  // Excel App1
    { 0x00020841L, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46} },  // Excel App2
};
const INT cclsidExcel = sizeof(rgclsidExcel) / sizeof(rgclsidExcel[0]);


//
//	WordArt CLSID for more special purpose hacks.
//
const GUID CLSID_WordArt = 
    { 0x000212F0L, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46} };
const GUID CLSID_PaintbrushPicture = 
    { 0x0003000AL, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const GUID CLSID_BitmapImage = 
    { 0xD3E34B21L, 0x9D75, 0x101A, { 0x8C, 0x3D, 0x00, 0xAA, 0x00, 0x1A, 0x16, 0x52 } };

//
//	Ink object
//
const GUID CLSID_Ink = 
	{ 0x13DE4A42, 0x8D21, 0x4C8E, {0xBF, 0x9C, 0x8F, 0x69, 0xCB, 0x06, 0x8F, 0xCA} };
const IID IID_ILineInfo = 
	{ 0x9C1C5AD5, 0xF22F, 0x4DE4, {0xB4, 0x53, 0xA2, 0xCC, 0x48, 0x2E, 0x7C, 0x33} };

#define dxyHandle (6) // Object frame handle size
#define dxyFrameDefault  (1) // Object frame width

// 
// utility functions
//

/*
 *	IsExcelCLSID (clsid)
 *
 *	@func	checks to see if the given clsid is one of XL's
 *
 *	@rdesc	TRUE/FALSE
 */
BOOL IsExcelCLSID(
	REFGUID clsid)
{
    for(LONG i = 0; i < cclsidExcel; i++)
    {
        if(IsEqualCLSID(clsid, rgclsidExcel[i]))
			return TRUE;
    }
    return FALSE;
}

//
//	PUBLIC methods
//

/*
 *	COleObject::QueryInterface(ridd, ppv)
 *
 *	@mfunc	the standard OLE QueryInterface
 *
 *	@rdesc	NOERROR		<nl>
 *			E_NOINTERFACE
 */
STDMETHODIMP COleObject::QueryInterface(
	REFIID	riid,		//@parm Requested interface ID
	void **	ppv)		//@parm Out parm for result
{
	HRESULT hr = NOERROR;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::QueryInterface");

    if(IsZombie())
        return CO_E_RELEASED;
        
	if(!ppv)
		return E_INVALIDARG;
	else
		*ppv = NULL;

	if(IsEqualIID(riid, IID_IUnknown))
		*ppv = (IUnknown *)(IOleClientSite *)this;

	else if(IsEqualIID(riid, IID_IOleClientSite))
		*ppv = (IOleClientSite *)this;

	else if(IsEqualIID(riid, IID_IOleInPlaceSite))
		*ppv = (IOleInPlaceSite *)this;

	else if(IsEqualIID(riid, IID_IAdviseSink))
		*ppv = (IAdviseSink *)this;

	else if(IsEqualIID(riid, IID_IOleWindow))
		*ppv = (IOleWindow *)this;

	else if(IsEqualIID(riid, IID_IRichEditOleCallback))
	{
		// NB!! Returning this pointer in our QI is 
		// phenomenally bogus; it breaks fundamental COM
		// identity rules (granted, not many understand them!).
		// Anyway, RichEdit 1.0 did this, so we better.

		TRACEWARNSZ("Returning IRichEditOleCallback interface, COM "
			"identity rules broken!");

		*ppv = _ped->GetRECallback();
	}
	else
		hr = E_NOINTERFACE;

	if(*ppv)
		(*(IUnknown **)ppv)->AddRef();

	return hr;
}

/*
 *	COleObject::AddRef()
 *
 *	@mfunc	Increments reference count
 *
 *	@rdesc	New reference count
 */
STDMETHODIMP_(ULONG) COleObject::AddRef()
{
    ULONG cRef;
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::AddRef");

    cRef = SafeAddRef();
	
	return cRef;
}

/*
 *	COleObject::Release	()
 *
 *	@mfunc	Decrements reference count
 *
 *	@rdesc	New reference count
 */
STDMETHODIMP_(ULONG) COleObject::Release()
{
    ULONG cRef;
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::Release");

	cRef = SafeRelease();

	return cRef;
}

/*
 *	COleObject::SaveObject ()
 *
 *	@mfunc	implemtenation of IOleClientSite::SaveObject
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::SaveObject()
{
	CCallMgr	callmgr(_ped);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::SaveObject");

	return SafeSaveObject();
}

/*
 *	COleObject::SafeSaveObject ()
 *
 *	@mfunc	implemtenation of IOleClientSite::SaveObject for internal consumption
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::SafeSaveObject()
{
	IPersistStorage *pps;
	HRESULT hr;
	CStabilize stabilize(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::SafeSaveObject");

	if(!_punkobj || !_pstg)
	{
		TRACEWARNSZ("SaveObject called on invalid object");
		return E_UNEXPECTED;
	}

    if(IsZombie())
        return CO_E_RELEASED;

	hr = _punkobj->QueryInterface(IID_IPersistStorage, (void **)&pps);

	TESTANDTRACEHR(hr);

	if(hr == NOERROR)
	{
        if(IsZombie())
            return CO_E_RELEASED;
        
		SavePrivateState();
		
        if(IsZombie())
            return CO_E_RELEASED;
        
		hr = OleSave(pps, _pstg, TRUE);
	
	    if(IsZombie())
	        return CO_E_RELEASED;
        
		TESTANDTRACEHR(hr);

		// note that SaveCompleted is called even if OleSave fails.
		// If both OleSave and SaveCompleted succeed, then go ahead
		// and commit the changes

		if(pps->SaveCompleted(NULL) == NOERROR && hr == NOERROR)
		{
		    if(IsZombie())
		        return CO_E_RELEASED;
			
			hr = _pstg->Commit(STGC_DEFAULT);

			TESTANDTRACEHR(hr);
		}
        pps->Release();
	}
	return hr;
}

/*
 *	COleObject::GetMoniker (dwAssign, dwWhichMoniker, ppmk)
 *
 *	@mfunc	implementation of IOleClientSite::GetMoniker
 *
 *	@rdesc	E_NOTIMPL
 */
STDMETHODIMP COleObject::GetMoniker(
	DWORD		dwAssign,		//@parm	Force an assignment?
	DWORD		dwWhichMoniker,	//@parm	Kind of moniker to get
	IMoniker **	ppmk)			//@parm Out parm for result
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::GetMoniker");

	TRACEWARNSZ("method not implemented!");

	if(ppmk)
		*ppmk = NULL;

	return E_NOTIMPL;
}
	
/*
 *	COleObject::GetContainer(ppcont)
 *
 *	@mfunc	implementation of IOleClientSite::GetContainer
 *
 *	@rdesc	E_NOINTERFACE
 */
STDMETHODIMP COleObject::GetContainer(
	IOleContainer **ppcont)	//@parm	Out parm for result
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::GetContainer");

	TRACEWARNSZ("method not implemented!");

	if(ppcont)
		*ppcont = NULL;

	// Richedit 1.0 returns E_NOINTERFACE instead of E_NOTIMPL.  Do
	// the same.
	return E_NOINTERFACE;
}

/*
 *	COleObject::ShowObject()
 *
 *	@mfunc	Implementation of IOleClientSite::ShowObject.  
 *
 *	@rdesc	E_NOTIMPL
 */
STDMETHODIMP COleObject::ShowObject()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::ShowObject");

	TRACEWARNSZ("method not implemented!");

	return E_NOTIMPL;
}

/*
 *	COleObject::OnShowWindow (fShow)
 *
 *	@mfunc
 *		implementation of IOleClientSite::OnShowWindow -- notifies
 *		the client site that the object is or is not being shown in its
 *		own application window.  This governs whether or not hatching 
 *		should appear around the object in richedit.
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::OnShowWindow(
	BOOL fShow)		//@parm If TRUE, object is being drawn in its own window
{
	DWORD dwFlags = _pi.dwFlags;
	CCallMgr	callmgr(_ped);
	CStabilize stabilize(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnShowWindow");

    if(IsZombie())
        return CO_E_RELEASED;

	_pi.dwFlags &= ~REO_OPEN;
	if(fShow)
		_pi.dwFlags |= REO_OPEN;

	// If something changed, redraw object
	if(dwFlags != _pi.dwFlags)
	{
		RECTUV rc;
		GetRectuv(rc);

		// Invalidate rect that we're in.
		_ped->TxInvalidateRect(&rc);

		// We're not allowed to call invalidate rect by itself without
		// terminating it with a call to update window.However, we don't
		// care at this point if things are redrawn right away.
		_ped->TxUpdateWindow();
	}
	return NOERROR;
}

/*
 *	COleObject::RequestNewObjectLayout ()
 *
 *	@mfunc	Implementation of IOleClientSite::RequestNewObjectLayout
 *
 *	@rdesc	E_NOTIMPL
 */
STDMETHODIMP COleObject::RequestNewObjectLayout()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, 
			"COleObject::RequestNewObjectLayout");

	TRACEWARNSZ("method not implemented!");

	return E_NOTIMPL;
}

/*
 *	COleObject::GetWindow(phwnd)
 *
 *	@mfunc	Implementation of IOleInPlaceSite::GetWindow
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::GetWindow(
	HWND *phwnd)	//@parm Where to put window
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::GetWindow");

	// NB! this method is not stabilized.

    if(IsZombie())
        return CO_E_RELEASED;
        
	if(phwnd)
		return _ped->TxGetWindow(phwnd);

	return E_INVALIDARG;
}

/*
 *	COleObject::ContextSensitiveHelp(fEnterMode)
 *
 *	@mfunc	Implemenation of IOleInPlaceSite::ContextSensitiveHelp
 *
 *	@rdesc	HRESULT
 */
 STDMETHODIMP COleObject::ContextSensitiveHelp(
 	BOOL fEnterMode)	//@parm, If TRUE, then we're in help mode
 {
 	IRichEditOleCallback *precall;
	CCallMgr	callmgr(_ped);
	CStabilize	stabilize(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, 
			"COleObject::ContextSensitiveHelp");

    if(IsZombie())
        return CO_E_RELEASED;

	CObjectMgr *pobjmgr = _ped->GetObjectMgr();
	if(!pobjmgr)
		return E_OUTOFMEMORY;
	
	// If the mode changes
	if(pobjmgr->GetHelpMode() != fEnterMode)
	{
		pobjmgr->SetHelpMode(fEnterMode);

		precall = _ped->GetRECallback();
		if(precall)
			return precall->ContextSensitiveHelp(fEnterMode);
	}
	return NOERROR;
}

/*
 *	COleObject::CanInPlaceActivate()
 *
 *	@mfunc	implementation of IOleInPlaceSite::CanInPlaceActivate
 *
 *	@rdesc	NOERROR or S_FALSE
 */
STDMETHODIMP COleObject::CanInPlaceActivate()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, 
			"COleObject::CanInPlaceActivate");

    if(IsZombie())
        return CO_E_RELEASED;
        
	// If we have a callback && the object is willing to show
	// content, then we can in-place activate

	if(_ped->GetRECallback() && _pi.dvaspect == DVASPECT_CONTENT)
 		return NOERROR;
 
	return S_FALSE;
}

/*
 *	COleObject::OnInPlaceActivate()
 *
 *	@mfunc	implementation of IOleInPlaceSite::OnInPlaceActivate
 *
 *	@rdesc	noerror
 */
STDMETHODIMP COleObject::OnInPlaceActivate()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnInPlaceActivate");
	// assume that in-place objects can never be blank.
	_pi.dwFlags &= ~REO_BLANK;
	_fInPlaceActive = TRUE;

	return NOERROR;
}

/*
 *	COleObject::OnUIActivate ()
 *
 *	@mfunc	implementation of IOleInPlaceSite::OnUIActivate.  Notifies
 *			the container that the object is about to be activated in
 *			place with UI elements suchs as merged menus
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::OnUIActivate()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnUIActivate");

	CCallMgr	callmgr(_ped);
	CStabilize	stabilize(this);

    if(IsZombie())
        return CO_E_RELEASED;
        
	CObjectMgr *pobjmgr = _ped->GetObjectMgr();
	if(!pobjmgr)
		return E_OUTOFMEMORY;

	IRichEditOleCallback *precall = pobjmgr->GetRECallback();

	if(precall)
	{
		// Force this object to be selected, if it isn't already
		// Update the selection before making the outgoing call
		if(!(_pi.dwFlags & REO_SELECTED))
		{
			CTxtSelection *psel = _ped->GetSel();
			if(psel)
				psel->SetSelection(_cp, _cp + 1);
		}
		precall->ShowContainerUI(FALSE);
	    if(IsZombie())
	        return CO_E_RELEASED;
        
		// This is an optimization for activating multiple
		pobjmgr->SetShowUIPending(FALSE);

		// RAID 7212
		// We don't want to set the in place active object if we are already in the process of activating the pbject.
		// Otherwise, there will be bad interactions with the code in TxDraw for out of process servers.
		// Note : it may be possible to always set this in ActivateObj but I left this here for those cases wher
		// OnUIActivate may be called directly.
		if (!_fActivateCalled)
		{
			Assert(!pobjmgr->GetInPlaceActiveObject());	
			pobjmgr->SetInPlaceActiveObject(this);
			_pi.dwFlags |= REO_INPLACEACTIVE;
		}

		return NOERROR;
	}
	return E_UNEXPECTED;
}

/*
 *	COleObject::GetWindowContext(ppipframe, ppipuidoc, prcPos, prcClip, pipfinfo) 
 *
 *	@mfunc	Implementation of IOleInPlaceSite::GetWindowContext.
 *			Enables the in-place object to retrieve the window
 *			interfaces that form the window object hierarchy.
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::GetWindowContext(
	IOleInPlaceFrame **ppipframe,	//@parm	Where to put in-place frame
	IOleInPlaceUIWindow **ppipuidoc,//@parm Where to put ui window
	LPRECT prcPos,					//@parm Position rect
	LPRECT prcClip,					//@parm Clipping rect
	LPOLEINPLACEFRAMEINFO pipfinfo)	//@parm Accelerator information
{
	CCallMgr	callmgr(_ped);
	CStabilize stabilize(this);
	
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::GetWindowContext");
	
    if(IsZombie())
        return CO_E_RELEASED;
        
	// Let container verify other parameters; we don't use them
	if(!prcPos || !prcClip)
		return E_INVALIDARG;
		
	IRichEditOleCallback *precall = _ped->GetRECallback();
	if(precall)
	{
		RECTUV rcuv;
		GetRectuv(rcuv);
		_ped->_pdp->RectFromRectuv(*prcPos, rcuv);

		// FUTURE (alexgo); we may need to get this from the
		// display instead to handle the inactive state if we ever
		// want to support embedded objects with the inactive state.
		_ped->TxGetClientRect(prcClip);
		return precall->GetInPlaceContext(ppipframe, ppipuidoc, pipfinfo);
	}
	return E_UNEXPECTED;
}

/*
 *	COleObject::Scroll(sizeScroll)
 *
 *	@mfunc	implementation of IOleInPlaceSite::Scroll
 *
 *	@rdesc 	E_NOTIMPL;
 */
STDMETHODIMP COleObject::Scroll(
	SIZE sizeScroll)	//@parm Amount to scroll
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::Scroll");

	TRACEWARNSZ("method not implemented!");

	return E_NOTIMPL;
}

/*
 *	COleObject::OnUIDeactivate (fUndoable)
 *
 *	@mfunc	implementation of IOleInPlaceSite::OnUIDeactivate.  Notifies
 *			the container that it should re-install its user interface
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::OnUIDeactivate(
	BOOL fUndoable)		//@parm Whether you can undo anything here
{
	CCallMgr	callmgr(_ped);
	CStabilize stabilize(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnUIDeactivate");

    if(IsZombie())
        return CO_E_RELEASED;

	CObjectMgr *pobjmgr = _ped->GetObjectMgr();
	IRichEditOleCallback *precall = _ped->GetRECallback();

	if(_fIsPaintBrush)
	{
		// Hack for RAID 3293.  Bitmap object disappears after editing.
		// Apparently paint only triggers OnUIDeactivate and not OnInPlaceDeactivate
		// assume that in-place objects can never be blank.
		_fInPlaceActive = FALSE;
		// Reset REO_INPLACEACTIVE
		_pi.dwFlags &= ~REO_INPLACEACTIVE;
	}

	if(!precall)
		return E_UNEXPECTED;

	if(_ped->TxIsDoubleClickPending())
		_ped->GetObjectMgr()->SetShowUIPending(TRUE);
	else
	{
		// Ignore any errors; the old code did.
		precall->ShowContainerUI(TRUE);

	    if(IsZombie())
	        return CO_E_RELEASED;
	}
	
	pobjmgr->SetInPlaceActiveObject(NULL);

	if (!_fDeactivateCalled)
	{
		// We got here without DeActiveObj. Since to shutdown correctly
		// we need to call this, we do so here.
		DeActivateObj();
	}

	// Get focus back
	_ped->TxSetFocus();

#ifdef DEBUG
	// the OLE undo model is not very compatible with multi-level undo.
	// For simplicity, just ignore stuff.
	if(fUndoable)
	{
		TRACEWARNSZ("Ignoring a request to keep undo from an OLE object");
	}
#endif

	// Some objects are lame and draw outside the
	// areas they are supposed to.  So we need to 
	// just invalidate everything and redraw.

	_ped->TxInvalidate();
	return NOERROR;
}

/*
 *	COleObject::OnInPlaceDeactivate	()
 *
 *	@mfunc	implementation of IOleInPlaceSite::OnInPlaceDeactivate
 *
 *	@rdesc	NOERROR
 */
STDMETHODIMP COleObject::OnInPlaceDeactivate()
{
	CCallMgr	callmgr(_ped);
	CStabilize stabilize(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, 
			"COleObject::OnInPlaceDeactivate");

	_fInPlaceActive = FALSE;

	//Reset REO_INPLACEACTIVE
	_pi.dwFlags &= ~REO_INPLACEACTIVE;

	if(!_punkobj)
		return E_UNEXPECTED;

    if(IsZombie())
        return CO_E_RELEASED;
        
	// Apparently, WordArt 2.0 had some sizing problems.  The original
	// code has this call to GetExtent-->SetExtent, so I've kept it here.
	if(_fIsWordArt2)
	{
		// Ignore errors.  If anything fails, too bad.
		FetchObjectExtents();	// this will reset _size
		SetExtent(SE_NOTACTIVATING);
	}

	// Some objects are lame and draw outside the
	// areas they are supposed to.  So we need to 
	// just invalidate everything and redraw.

	// Note that we do this in UIDeactivate as well; however, the
	// double invalidation is necessary to cover some re-entrancy 
	// cases where we might be painted before everything is ready.

	_ped->TxInvalidate();
	return NOERROR;
}

/*
 *	COleObject::DiscardUndoState ()
 *
 *	@mfunc	implementation of IOleInPlaceSite::DiscardUndoState.
 *
 *	@rdesc	NOERROR
 */
STDMETHODIMP COleObject::DiscardUndoState()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, 
			"COleObject::DiscardUndoState");

	// Nothing to do here; we don't keep any OLE-undo state as it's
	// not very compatible with multi-level undo.
	
	return NOERROR;
}

/*
 *	COleObject::DeactivateAndUndo ()
 *
 *	@mfunc	implementation of IOleInPlaceSite::DeactivateAndUndo--
 *			called by an active object when the user invokes undo
 *			in the active object
 *
 *	@rdesc	NOERROR	(yep, richedit1.0 ignored all the errors here)
 */
STDMETHODIMP COleObject::DeactivateAndUndo()
{
	CStabilize	stabilize(this);

  	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::DeactivateAndUndo");

    if(IsZombie())
        return CO_E_RELEASED;
        
	// Ignore error
	_ped->InPlaceDeactivate();

	// COMPATIBILITY ISSUE: we don't bother doing any undo here, as 
	// a multi-level undo model is incompatible with OLE undo.

	return NOERROR;
}

/*
 *	COleObject::OnPosRectChange	(prcPos)
 *
 *	@mfunc	implementation of IOleInPlaceSite::OnPosRectChange.  This
 *			method is called by an in-place object when its extents have
 *			changed
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP COleObject::OnPosRectChange(
	LPCRECT prcPos)
{
	IOleInPlaceObject *pipo;
 	RECT rcClip;
	RECT rcNewPos;
	CCallMgr	callmgr(_ped);
	CStabilize stabilize(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnPosRectChange");
	
	if(!prcPos)
		return E_INVALIDARG;

	if(!_punkobj)
		return E_UNEXPECTED;
		
    if(IsZombie())
        return CO_E_RELEASED;

	if(!_ped->fInplaceActive())
		return E_UNEXPECTED;
        
	// Check to see if the rect moved; we don't allow this, but
	// do allow the object to keep the new size

	rcNewPos = *prcPos;

	rcNewPos.right = rcNewPos.left + (prcPos->right - prcPos->left);
	rcNewPos.bottom = rcNewPos.top + (prcPos->bottom - prcPos->top);		

	_ped->TxGetClientRect(&rcClip);

	HRESULT hr = _punkobj->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo);
	if(hr == NOERROR)
	{
		hr = pipo->SetObjectRects(&rcNewPos, &rcClip);
        pipo->Release();

        // bug fix 6073
        // Need to set viewchange flag so we resize the ole object properly on ITextServices::TxDraw
		CObjectMgr * pobjmgr = _ped->GetObjectMgr();
		if (pobjmgr && pobjmgr->GetInPlaceActiveObject() == this)
			_fViewChange = TRUE;
	}
	return hr;
}

/*
 *	COleObject::OnDataChange (pformatetc, pmedium)
 *
 *	@mfunc	implementation of IAdviseSink::OnDataChange
 *
 *	@rdesc	NOERROR
 */
STDMETHODIMP_(void) COleObject::OnDataChange(
	FORMATETC *pformatetc,		//@parm Format of data that changed
	STGMEDIUM *pmedium)			//@parm Data that changed
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnDataChange");
	CCallMgr	callmgr(_ped);

    if(IsZombie())
        return;
	_pi.dwFlags &= ~REO_BLANK;
	// this will also set the modified flag
	_ped->GetCallMgr()->SetChangeEvent(CN_GENERIC);

	return;
}

/*
 *	COleObject::OnViewChange(dwAspect, lindex) 
 *
 *	@mfunc	implementation of IAdviseSink::OnViewChange.  Notifies
 *			us that the object's view has changed.
 *
 *	@rdesc	HRESULT
 */
STDMETHODIMP_(void) COleObject::OnViewChange(
	DWORD	dwAspect,		//@parm Aspect that has changed
	LONG	lindex)			//@parm unused
{
	CStabilize	stabilize(this);
	CCallMgr	callmgr(_ped);
		
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnViewChange");
	
	if(!_punkobj)
		return;		// E_UNEXPECTED

    if(IsZombie())
        return;

	if (_fInUndo)	// This object has been deleted, forget view change
		return;

	_pi.dwFlags &= ~REO_BLANK;
	// Richedit 1.0 ignored errors on getting object extents

  	FetchObjectExtents();

    // bug fix 6073
    // Need to set viewchange flag so we resize the ole object properly on ITextServices::TxDraw
    CObjectMgr * pobjmgr = _ped->GetObjectMgr();
	if (pobjmgr && pobjmgr->GetInPlaceActiveObject() == this)
		_fViewChange = TRUE;
      
	CDisplay *pdp = _ped->_pdp;
	if(pdp)
		pdp->OnPostReplaceRange(CP_INFINITE, 0, 0, _cp, _cp + 1, NULL);

	_ped->GetCallMgr()->SetChangeEvent(CN_GENERIC);
	return;
}
	
/*
 *	COleObject::OnRename (pmk)
 *
 *	@mfunc	implementation of IAdviseSink::OnRename.  Notifies the container
 *			that the object has been renamed
 *
 *	@rdesc	E_NOTIMPL
 */
STDMETHODIMP_(void) COleObject::OnRename(
	IMoniker *pmk)			//@parm Object's new name
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnRename");
	
	TRACEWARNSZ("IAdviseSink::OnRename not implemented!");

	return;	// E_NOTIMPL;
}

/*
 *	COleObject::OnSave ()
 *
 *	@mfunc	implementation of IAdviseSink::OnSave.  Notifies the container
 *			that an object has been saved
 *
 *	@rdesc	NOERROR
 */
STDMETHODIMP_(void) COleObject::OnSave()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnSave");
	_pi.dwFlags &= ~REO_BLANK;
}

/*
 *	COleObject::OnClose	()
 *
 *	@mfunc	implementation of IAdviseSink::OnClose.  Notifies the container
 *			that an object has been closed.
 *
 *	@rdesc	NOERROR
 */
STDMETHODIMP_(void) COleObject::OnClose()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::OnClose");
	
    if(IsZombie())
        return;
        
	// If the object is blank (i.e. no data in it), we don't want to leave
	// it in the backing store--there is nothing for us to draw && therefore
	// nothing for the user to click on!  So just delete the object with
	// a space.  Note that 1.0 actually deleted the object; we'll just
	// replace it with a space to make everything work out right.
	if(_pi.dwFlags & REO_BLANK)
	{
		CCallMgr	callmgr(_ped);
		CStabilize	stabilize(this);
		CRchTxtPtr	rtp(_ped, _cp);

		// We don't want the delete of this object to go on the undo
		// stack.  We use a space so that cp's will work out right for
		// other undo actions.
		rtp.ReplaceRange(1, 1, L" ", NULL, -1);
	}
	_ped->TxSetForegroundWindow();
}
				
/*
 *	COleObject::OnPreReplaceRange
 *
 *	@mfunc	implementation of ITxNotify::OnPreReplaceRange
 *			called before changes are made to the backing store
 */
void COleObject::OnPreReplaceRange(
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	Assert(_fInUndo == FALSE);
}

/*
 *	COleObject::OnPostReplaceRange
 *
 *	@mfunc	implementation of ITxNotify::OnPostReplaceRange
 *			called after changes are made to the backing store
 *	
 *	@comm	we use this method to keep our cp's up-to-date
 */
void COleObject::OnPostReplaceRange(
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	// The only case we need to worry about is when changes
	// come before our object

	Assert(_fInUndo == FALSE);
#ifndef NOINKOBJECT
	LONG	cpOld = _cp;
#endif
	_fDraw = TRUE;
	if(cp <= _cp)
	{		
		if(cp + cchDel > _cp)
		{
			_fDraw = FALSE;
			return;
		}
		_cp += (cchNew - cchDel);
	}
#ifndef NOINKOBJECT
	if (_ped && _fIsInkObject && _pILineInfo)
	{
		if (cpFormatMin <= cpOld && cpOld < cpFormatMax)
		{
			// Inform Ink object for the formatting changes
			UINT	iInkWidth = 0;

			_ped->SetInkProps(_cp, _pILineInfo, &iInkWidth);

		}
	}
#endif
}
		
/*
 *	COleObject::Zombie ()
 *
 *	@mfunc
 *		Turn this object into a zombie
 *
 */
void COleObject::Zombie ()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::Zombie");

	_ped = NULL;
}

/*
 *	COleObject::COleObject(ped)
 *
 *	@mfunc	constructor
 */
COleObject::COleObject(
	CTxtEdit *ped)	//@parm context for this object
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::COleObject");

	AddRef();

	// Most values will be NULL by virtue of the allocator
	_ped = ped;

	CNotifyMgr *pnm = ped->GetNotifyMgr();
	if(pnm)
		pnm->Add((ITxNotify *)this);
}

/*
 *	COleObject::GetObjectData(preobj, dwFlags)
 *
 *	@mfunc	fills out an REOBJECT structure with information relevant
 *			to this object
 *
 *	@rdesc	HRESULT
 */
HRESULT	COleObject::GetObjectData(
	REOBJECT *preobj, 		//@parm Struct to fill out
	DWORD dwFlags)			//@parm Indicate what data is requested
{
	IOleObject *poo = NULL;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::GetObjectData");

	Assert(preobj);
	Assert(_punkobj);

	preobj->cp = _cp;
	
	if(_punkobj->QueryInterface(IID_IOleObject, (void **)&poo) == NOERROR)
	{
		// Don't worry about failures here
		poo->GetUserClassID(&(preobj->clsid));
	}
	
	preobj->dwFlags 	= _pi.dwFlags;
	preobj->dvaspect 	= _pi.dvaspect;
	preobj->dwUser 		= _pi.dwUser;
	memcpy(&preobj->sizel, &_size, sizeof(SIZE));

   	if(dwFlags & REO_GETOBJ_POLEOBJ)
	{
		preobj->poleobj = poo;
		if(poo)
			poo->AddRef();
	}
	else
		preobj->poleobj = NULL;

    if(poo)
        poo->Release();

    if(IsZombie())
        return CO_E_RELEASED;
        
	if(dwFlags & REO_GETOBJ_PSTG)
	{
		preobj->pstg = _pstg;
		if(_pstg)
			_pstg->AddRef();
	}
	else
		preobj->pstg = NULL;

	if(dwFlags & REO_GETOBJ_POLESITE)
	{
		// COMPATIBILITY HACK!!  Note that we don't 'release' any pointer that
		// may already be in the stored in the site.  RichEdit1.0 always sets
		// the value, consequently several apps pass in garbage for the site.
		//
		// If the site was previously set, we will get a reference counting
		// bug, so be sure that doesn't happen!
     
       	preobj->polesite = (IOleClientSite *)this;
       	AddRef();
 	}
	else
		preobj->polesite = NULL;

	return NOERROR;
}	

/*
 *	COleObject::IsLink()
 *
 *	@mfunc	returns TRUE if the object is a link
 *
 *	@rdesc	BOOL
 */
BOOL COleObject::IsLink()
{
	return !!(_pi.dwFlags & REO_LINK);
}

/*
 *	COleObject::InitFromREOBJECT(cp, preobj)
 *
 *	@mfunc	initializes this object's state from the given
 *			REOBJECT data structure
 *
 *	@rdesc	HRESULT
 */
HRESULT COleObject::InitFromREOBJECT(
	LONG	cp,			//@parm cp for the object
	REOBJECT *preobj)	//@parm	Data to use for initialization
{
	IOleLink *plink;
	HRESULT	hr = E_INVALIDARG;
	
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::InitFromREOBJECT");
	
	Assert(_punkobj == NULL);
    if(IsZombie())
        return CO_E_RELEASED;

	_cp = cp;

	if(preobj->poleobj)
		hr = preobj->poleobj->QueryInterface(IID_IUnknown, (void **)&_punkobj);
	else
	{
		_punkobj = (IOleClientSite *) this;
		AddRef();
		hr = NOERROR;
	}
        
	if(hr != NOERROR)
		return hr;
	
	_pstg = preobj->pstg;
	if(_pstg)
		_pstg->AddRef();

	_pi.dwFlags	 = preobj->dwFlags & REO_READWRITEMASK;
	_pi.dwUser	 = preobj->dwUser;
	_pi.dvaspect = preobj->dvaspect;

	preobj->sizel.cx = max(preobj->sizel.cx, 0);
	preobj->sizel.cy = max(preobj->sizel.cy, 0);

	memcpy(&_size, &preobj->sizel, sizeof(SIZE));	// COMPATIBILITY ISSUE: the RE 1.0 code had
													// some stuff to deal with REO_DYNAMICSIZE
													// here. We do not currently support that.
	
	if(_punkobj->QueryInterface(IID_IOleLink, (void **)&plink) == NOERROR)
	{
		_pi.dwFlags |= REO_LINK | REO_LINKAVAILABLE;
		plink->Release();
	}

    if(IsZombie())
        return CO_E_RELEASED;
        
	if (IsEqualCLSID(preobj->clsid, CLSID_StaticMetafile) ||
		IsEqualCLSID(preobj->clsid, CLSID_StaticDib) ||
		IsEqualCLSID(preobj->clsid, CLSID_Picture_EnhMetafile))
	{
		_pi.dwFlags |= REO_STATIC;
	}
	else if(IsExcelCLSID(preobj->clsid))
		_pi.dwFlags |= REO_GETMETAFILE;

	else if(IsEqualCLSID(preobj->clsid, CLSID_WordArt))
		_fIsWordArt2 = TRUE;

	else if(IsEqualCLSID(preobj->clsid, CLSID_PaintbrushPicture) ||
			IsEqualCLSID(preobj->clsid, CLSID_BitmapImage))
	{
		_fIsPaintBrush = TRUE;

		// These calls will initialize the flag, _fPBUseLocalSize, which
		// indicates that for this PB object, SetExtent calls are not 
		// acknowledged by the object, and we are to use our local value
		// of _size as the object extents.
		FetchObjectExtents();
		SetExtent(SE_NOTACTIVATING);
	}
#ifndef NOINKOBJECT
	else if(IsEqualCLSID(preobj->clsid, CLSID_Ink))
	{
		ILineInfo	*pILineInfo;

		_fIsInkObject = TRUE;

		_pi.dwFlags &= ~REO_RESIZABLE;		// No resizing for ink objects
		_pi.dwFlags |= REO_BELOWBASELINE | REO_INVERTEDSELECT;

		if(_punkobj->QueryInterface(IID_ILineInfo, (void **)&pILineInfo) == NOERROR)
		{
			UINT	iInkWidth = 0;

			_pILineInfo = pILineInfo;
			
			// Inform Ink object for the formatting changes
			_ped->SetInkProps(_cp, _pILineInfo, &iInkWidth);
		}
	}
#endif

	hr = ConnectObject();

    if(IsZombie())
        return CO_E_RELEASED;
        
	if(preobj->sizel.cx || preobj->sizel.cy)
		memcpy(&_size, &preobj->sizel, sizeof(SIZE));
	else
		FetchObjectExtents();

    if(IsZombie())
        return CO_E_RELEASED;

	// We don't do the following anymore although it was originally spec'd as the correct
	// behavior for applications in OLE 2.01. The reason is that no one else seems to and
	// it seems to result in some odd behavior.
#if 0
    // Finally, lock down Link objects so they we don't try to refetch their
	// extents from the server.  After initialization, link object size is
	// entirely determined by the container.
	if(_pi.dwFlags & REO_LINK)
    {
        // so we don't call GetExtents on remeasuring.
        _fSetExtent = TRUE;
	}
#endif 

	return NOERROR;
}

/*
 *	COleObject::MeasureObj(dypInch, dxpInch, dup, dvpAscent, dvpDescent, dvpDescentFont, tflow)
 *
 * Review: (keithcu) Should yDescentFont be descent of font
 * or descent of entire line? LS does one thing, old measurer
 * does another. I'm hoping that this feature is only used on
 * lines with one font..
 *	@mfunc	calculates the size of this object in device units
 */
void COleObject::MeasureObj(
	LONG	dvpInch,		//@parm	resolution of device
	LONG	dupInch,
	LONG &	dup,			//@parm Object width
	LONG &	dvpAscent,		//@parm Object ascent
	LONG &  dvpDescent,		//@parm Object descent
	SHORT	dvpDescentFont,	//@parm object's font descent
	TFLOW	tflow)			//@parm textflow
{
	LONG dvp;

	//If we are rotated and the OLE object doesn't understand rotation, then
	//we will measure and draw the object unrotated
	if (IsUVerticalTflow(tflow) && !(_pi.dwFlags & REO_CANROTATE))
	{
		dup = W32->HimetricToDevice(_size.dv, dupInch);
		dvp = W32->HimetricToDevice(_size.du, dvpInch);
	}
	else
	{
		dup = W32->HimetricToDevice(_size.du, dupInch);
		dvp = W32->HimetricToDevice(_size.dv, dvpInch);
	}

	if (_pi.dwFlags & REO_BELOWBASELINE)
	{
		dvpDescent = dvpDescentFont;
		dvpAscent = max(0, dvp - dvpDescent);
	}
	else //The normal case
	{
		dvpAscent = dvp;
		dvpDescent = 0;
	}
}

/* 
 * COleObject::InHandle(x, y, &pt)
 *
 * @mfunc  See if a point is in the rectangle defined by the handle at
 *		the given coordinates.
 *
 * @rdesc True if point is in handle.
 */
BOOL COleObject::InHandle(
	int		up,		//@parm Upper left corner x coordinate of handle box
	int		vp,		//@parm Upper left corner y coordinate of handle box
	const POINTUV &pt)//@parm Point to check
{
    RECTUV    rc;
    
    rc.left = up;
    rc.top = vp;

	// Add one to bottom right because PtInRect does not consider
	// points on bottom or right to be in rect.
    rc.right = rc.left + dxyHandle + 1;
    rc.bottom = rc.top + dxyHandle + 1;
    return PtInRect(&rc, pt);
}  

/*
 *	COleObject::CheckForHandleHit(&pt)
 *
 *	@mfunc	Check for a hit on any of the frame handles.
 *
 *	@rdesc	 NULL if no hit, cursor resource ID if there is a hit.
 */
LPTSTR COleObject::CheckForHandleHit(
	const POINTUV &pt,	//@parm POINT containing client coord. of the cursor.
	BOOL fLogical)		//@parm TRUE if you want the logical as opposed to visual cursor
{
	// If object is not resizeable, no chance of hitting a resize handle!
	if(!(_pi.dwFlags & REO_RESIZABLE) || FWrapTextAround())
		return NULL;

	RECTUV rc;
	GetRectuv(rc);

	if(!_dxyFrame)
		_dxyFrame = dxyFrameDefault;

	// Check to see if point is farther into the interior of the
	// object than the handles extent. If it is we can just bail.
	InflateRect(&rc, -(_dxyFrame + dxyHandle), -(_dxyFrame + dxyHandle));
	if(PtInRect(&rc, pt))
		return NULL;

	WCHAR *idcur = 0;

	// Check to see if point is in any of the handles and
	// return the proper cursor ID if it is.
	InflateRect(&rc, dxyHandle, dxyHandle);

	if(InHandle(rc.left, rc.top, pt) ||
	   InHandle(rc.right-dxyHandle, rc.bottom-dxyHandle, pt))
	{
		idcur = IDC_SIZENWSE;
	}
	else if(InHandle(rc.left, rc.top+(rc.bottom-rc.top-dxyHandle)/2, pt) ||
	   InHandle(rc.right-dxyHandle,
			rc.top+(rc.bottom-rc.top-dxyHandle)/2, pt))
	{
		idcur = IDC_SIZEWE;
	}
	else if(InHandle(rc.left, rc.bottom-dxyHandle, pt) ||
	   InHandle(rc.right-dxyHandle, rc.top, pt))
	{
		idcur = IDC_SIZENESW;
	}
	else if(InHandle(rc.left+(rc.right-rc.left-dxyHandle)/2, rc.top, pt) ||
	   InHandle(rc.left+(rc.right-rc.left-dxyHandle)/2,
			rc.bottom-dxyHandle, pt))
	{
		idcur = IDC_SIZENS;
	}

	//Convert logical cursor to visual cursor
	if (!fLogical && IsUVerticalTflow(_ped->_pdp->GetTflow()))
	{
		if (idcur == IDC_SIZENS)
			idcur = IDC_SIZEWE;
		else if (idcur == IDC_SIZEWE)
			idcur = IDC_SIZENS;
		else if (idcur == IDC_SIZENESW)
			idcur = IDC_SIZENWSE;
		else if (idcur == IDC_SIZENWSE)
			idcur = IDC_SIZENESW;
	}

	return idcur;
}

/* 
 * COleObject::DrawHandle(hdc, x, y)
 *
 * @mfunc  Draw a handle on the object frame at the specified coordinate
 */
void COleObject::DrawHandle(
	HDC hdc,	//@parm HDC to be drawn into
	int x,		//@parm x coordinate of upper-left corner of handle box
	int y)		//@parm y coordinate of upper-left corner of handle box
{
    RECT    rc;
    
	// Draw handle by inverting
    rc.left = x;
    rc.top = y;
    rc.right = x + dxyHandle;
    rc.bottom = y + dxyHandle;
    InvertRect(hdc, &rc);
}  

/*
 *	COleObject::DrawFrame(pdp, hdc, prc)
 *
 *	@mfunc	Draw a frame around the object.  Invert if required and
 *		include handles if required.
 */
void COleObject::DrawFrame(
	const CDisplay *pdp,    //@parm the display to draw to
	HDC             hdc,	//@parm the device context
	RECT           *prc)  //@parm the rect around which to draw
{
	if(_pi.dwFlags & REO_OWNERDRAWSELECT)
		return;

	RECT	rc;
	CopyRect(&rc, prc);

	if(_pi.dwFlags & REO_INVERTEDSELECT)
		InvertRect(hdc, &rc);				//Invert entire object

	else
	{
		// Just the border, so use a null brush
		int ropOld = SetROP2(hdc, R2_NOT);
		HBRUSH hbrOld = (HBRUSH) SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, hbrOld);
		SetROP2(hdc, ropOld);
	}

	if(_pi.dwFlags & REO_RESIZABLE)
	{
		int     bkmodeOld;
		HPEN	hpen;
		LOGPEN	logpen;

		bkmodeOld = SetBkMode(hdc, TRANSPARENT);

		// Get frame width
		_dxyFrame = dxyFrameDefault;
		hpen = (HPEN)GetCurrentObject(hdc, OBJ_PEN);
		if(W32->GetObject(hpen, sizeof(LOGPEN), &logpen))
		{
			if(logpen.lopnWidth.x)
				_dxyFrame = (SHORT)logpen.lopnWidth.x;
		}

		// Draw handles inside rectangle boundary
 		InflateRect(&rc, -_dxyFrame, -_dxyFrame);

		LONG x = rc.left;

		DrawHandle(hdc, x, rc.top);
		DrawHandle(hdc, x, rc.top	 + (rc.bottom - rc.top - dxyHandle)/2);
		DrawHandle(hdc, x, rc.bottom - dxyHandle);

		x = rc.left + (rc.right - rc.left - dxyHandle)/2; 
		DrawHandle(hdc, x, rc.top);
		DrawHandle(hdc, x, rc.bottom - dxyHandle);

		x = rc.right - dxyHandle;
		DrawHandle(hdc, x, rc.top);
		DrawHandle(hdc, x, rc.top + (rc.bottom - rc.top - dxyHandle)/2);
		DrawHandle(hdc, x, rc.bottom - dxyHandle);

		SetBkMode(hdc, bkmodeOld);
	}
}


/*
 *	COleObject::CreateDib (hdc)
 *
 *	@mfunc	Create DIB for Windows CE display
 */
void COleObject::CreateDib(
	HDC hdc)
{
	int				nCol = 0;
    BYTE            *pbDib;
	HGLOBAL			hnew = NULL;
	BYTE			*pbSrcBits;
	LPBITMAPINFO	pbmi = (LPBITMAPINFO) GlobalLock(_hdata);
	DWORD			dwPixelsPerRow = 0;
	DWORD			dwPixels = 0;

    if(pbmi->bmiHeader.biBitCount <= 8)
    {
	    nCol = 1 << pbmi->bmiHeader.biBitCount;

		// Calculate the number of pixels.  Account for DWORD alignment
		DWORD dwPixelsPerByte = 8 / pbmi->bmiHeader.biBitCount;
		DWORD dwBitsPerRow = pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount;
		dwBitsPerRow = (dwBitsPerRow + 7) & ~7;				// Round up to byte boundary
		DWORD dwBytesPerRow = dwBitsPerRow / 8;
		dwBytesPerRow = (dwBytesPerRow + 3) & ~3;			// Round up to DWORD
		dwPixelsPerRow = dwBytesPerRow * dwPixelsPerByte;

		// Double check with original
		#ifdef DEBUG
		DWORD dwBlockSize = GlobalSize(_hdata);
		DWORD dwBitMapBytes = dwBlockSize - sizeof(BITMAPINFOHEADER) - (nCol * sizeof(RGBQUAD));
		DWORD dwBitMapPixels = dwBitMapBytes * dwPixelsPerByte;
		dwPixels = dwPixelsPerRow * pbmi->bmiHeader.biHeight;
		Assert(dwPixels == dwBitMapPixels);
		#endif
    }
	else
		dwPixelsPerRow = pbmi->bmiHeader.biWidth;

	dwPixels = dwPixelsPerRow * pbmi->bmiHeader.biHeight;
    
	pbSrcBits = (BYTE*)(pbmi) + sizeof(BITMAPINFOHEADER) + (nCol * sizeof(RGBQUAD));

#ifdef UNDER_NT

	// For NT viewing convert four color bitmaps to 16 color bitmap
	if(nCol == 4)
	{
		// First let's figure out how big the new memory block needs to be.
		DWORD cb = sizeof(BITMAPINFOHEADER) + (16 * sizeof(RGBQUAD));
		cb += dwPixels / 2;
		hnew = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cb);
	
		// Now locate the interesting places
		LPBITMAPINFO pNewBmi = (LPBITMAPINFO) GlobalLock(hnew);
		BYTE *pNewBits = (BYTE*)(pNewBmi) + sizeof(BITMAPINFOHEADER) + (16 * sizeof(RGBQUAD));

		// Modify the header
		pNewBmi->bmiHeader = pbmi->bmiHeader;
		pNewBmi->bmiHeader.biBitCount = 4;
		pNewBmi->bmiHeader.biClrUsed = 4;

		// Set up the DIB RGB Colors.
		for (int i = 0; i < 16; i++)
		{
			BYTE data = 0;
			switch (i % 4)
			{
			case 0:
				break;
			case 1:
				data = 0x55;
				break;
			case 2:
				data = 0xAA;
				break;
			case 3:
				data = 0xFF;
				break;
			}
			pNewBmi->bmiColors[i].rgbBlue = data;
			pNewBmi->bmiColors[i].rgbGreen = data;
			pNewBmi->bmiColors[i].rgbRed = data;
			pNewBmi->bmiColors[i].rgbReserved = 0;
		}

		// Convert the byte array.
		for (DWORD j = 0; j < dwPixels; j++)
		{
			int iSrcByte = j / 4;

			BYTE bits = pbSrcBits[iSrcByte];
			bits >>= 6 - (j%4) * 2;
			bits &= 0x3;
			int iDstByte = j / 2;
			bits <<= 4 - (j%2) * 4;
			pNewBits[iDstByte] |= bits;
		}
		GlobalUnlock(_hdata);
		pbmi = pNewBmi;
		pbSrcBits = pNewBits;
	}
#endif

	_hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&pbDib, NULL, 0);
	if(_hdib == NULL)
	{
		_ped->GetCallMgr()->SetOutOfMemory();

        // V-GUYB:
        // Do not attempt to repaint this picture until the user starts typing in the
        // control. This allows the user to dismiss the oom that will appear and then
        // save the document, and then free up some space. If we don't do this here, 
        // every time the oom msg is dismissed it will appear again. This doesn't allow 
        // the user to save the document unless they can find some memory to free.
        _fDraw = FALSE;

		TRACEWARNSZ("Out of memory creating DIB");
		return;
	}

	DWORD nBytes;

	if(nCol)
	{
		DWORD nPixelsPerByte =  8 / pbmi->bmiHeader.biBitCount;
		nBytes = dwPixels / nPixelsPerByte;
	}
	else
		nBytes =  dwPixels * 4;			// Each pixel occupies 4 bytes in DIB
	CopyMemory(pbDib, pbSrcBits, nBytes);

	GlobalUnlock(_hdata);
	GlobalFree(hnew);
}

/*
 *	COleObject::DrawDib (hdc, prc)
 *
 *	@mfunc	Auxiliary function that draws the dib in the given dc
 */
void COleObject::DrawDib(
	HDC hdc,
	RECT *prc)
{
	if(!_hdib)
		CreateDib(hdc);

	// If _hdib is still NULL, just return.  Maybe out of memory.
	if(!_hdib)
		return;

	HDC hdcMem = CreateCompatibleDC(hdc);
	if (!hdcMem)
		return;

	LPBITMAPINFO	pbmi = (LPBITMAPINFO) GlobalLock(_hdata);
	SelectObject(hdcMem, _hdib);
    StretchBlt(hdc, prc->left, prc->top,
			prc->right - prc->left, prc->bottom - prc->top,
			hdcMem, 0, 0, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight, SRCCOPY);
	GlobalUnlock(_hdata);
	DeleteDC(hdcMem);
}

/*
 *	COleObject::DrawObj (pdp, hdc, fMetafile, ppt, prcRender)
 *
 *	@mfunc	draws the object.
 *	REVIEW (keithcu) How expensive is SaveDC/RestoreDC? We could
 *  have a flag when we know that the object isn't going to totally screw
 *	around with our DC and then not go through that expense.
 *
 */
void COleObject::DrawObj(
	const CDisplay *pdp,	//@parm Display object for the view
	LONG		dypInch,	//@parm Resolution of device
	LONG		dxpInch,
	HDC			hdc,		//@parm Drawing HDC (can differ from display's)
	const RECTUV *prcClip,	//@parm Clipping rectangle
	BOOL		fMetafile,	//@parm Whether the HDC is a metafile
	POINTUV	   *ppt,		//@parm Top left corner of where to draw
	LONG		dvpBaselineLine,
	LONG		dvpDescentMaxCur,
	TFLOW		tflow)
{
	BOOL			 fMultipleSelect = FALSE;
	CObjectMgr *	 pobjmgr = _ped->GetObjectMgr();
	IViewObject *	 pvo;
	CDisplayPrinter *pdpPrint;
	RECT			 rc, rcClip;
	RECTUV			 rcuv;
	LONG			 cpMin, cpMost;
	_ped->GetSelRangeForRender(&cpMin, &cpMost);
	BOOL			 fSelected = _cp >= cpMin && _cp < cpMost && !FWrapTextAround();

	SaveDC(hdc);	 //Anything in this function below could change things.

	if(pdp->IsMain() && !(_pi.dwFlags & REO_OWNERDRAWSELECT))
	{
		if (fSelected)
		{
			if(cpMost - cpMin > 1)
				fMultipleSelect = TRUE;

			// The following overwrites the selection colors currently
			// selected into the hdc. We do this for RE 2.0 compatibility,
			// e.g., else selected name links in Outlook appear yellow
			// after the InvertRect() in DrawFrame() (altho the semicolon
			// blanks appear in selection colors). Note: we could define
			// REO_OWNERDRAWSELECT, which would bypass the following 2 lines
			// and suppress the InvertRect below and in DrawFrame(). Then
			// Outlook's From:, To:, and CC: would have correct selection
			// colors throughout.
			::SetTextColor(hdc, _ped->TxGetForeColor());
			::SetBkColor  (hdc, _ped->TxGetBackColor());
		}
	}

	if(_fInPlaceActive || !_fDraw)
	{
		// If we're inplace active, don't do anything; the server is
		// drawing for us. We also don't do anything prior to the fDraw
		// property being set
		return;
	}

	// Draw object where we are asked within rendering rectangle
	rcuv.left = ppt->u;
	rcuv.bottom = ppt->v + dvpBaselineLine;

	if (IsUVerticalTflow(tflow) && !(_pi.dwFlags & REO_CANROTATE))
	{
		rcuv.right = rcuv.left + W32->HimetricToDevice(_size.dv, dypInch);
		rcuv.top = rcuv.bottom - W32->HimetricToDevice(_size.du, dxpInch);
	}
	else
	{
		rcuv.right = rcuv.left + W32->HimetricToDevice(_size.du, dxpInch);
		rcuv.top = rcuv.bottom - W32->HimetricToDevice(_size.dv, dypInch);
	}

	if (_pi.dwFlags & REO_BELOWBASELINE)
		OffsetRect((RECT*)&rcuv, 0, dvpDescentMaxCur);

	pdp->RectFromRectuv(rc, rcuv);
	pdp->RectFromRectuv(rcClip, *prcClip);

	if (fSelected)
		SetBkMode(hdc, OPAQUE);

	SetTextAlign(hdc, TA_TOP);

	SaveDC(hdc);  // calls to OLE object (IViewObject::Draw or OleDraw) might change HDC

	//Do clipping because OLE doesn't know where to draw
	IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

	if(_fIsEBookImage)
	{
		POINT pt = {rc.left, rc.top};
		WinLPtoDP(hdc, &pt, 1);
		if(_ped->fInHost2())
    		(_ped->GetHost())->TxEBookImageDraw(_EBookImageID, hdc, &pt, NULL/*&rc*/,
                                        (pobjmgr->GetSingleSelect()==this));
	}
	else if(_hdata)
	{
		// This is some Windows CE Dib, let's try the direct approach
		DrawDib(hdc, &rc);
	}
	else if(fMetafile)
	{
		if(_punkobj->QueryInterface(IID_IViewObject, (void **)&pvo) 
				== NOERROR)
		{
			pdpPrint = (CDisplayPrinter *)pdp;

			//REVIEW (keithcu) Has this ever been tested? GetPrintPage
			//is in twips while IViewObject::Draw is in metafile units!
			RECT rc1 = pdpPrint->GetPrintPage();

			// Fix up rc for Draw()
			rc1.bottom = rc1.bottom - rc1.top;			    
			rc1.right = rc1.right - rc1.left;

			pvo->Draw(_pi.dvaspect, -1, NULL, NULL, 0, hdc, (RECTL *)&rc,
					(RECTL *)&rc1, NULL, 0);
			pvo->Release();
		}
	}
	else
		OleDraw(_punkobj, _pi.dvaspect, hdc, &rc);

	RestoreDC(hdc, -1);

	// Do selection stuff if this is for the main (screen) view.
	if(pdp->IsMain() && !FWrapTextAround())
	{
		if(_pi.dwFlags & REO_OPEN)
			OleUIDrawShading(&rc, hdc);

		// If the object has been selected by clicking on it, draw
		// a frame and handles around it.  Otherwise, if we are selected
		// as part of a range, invert ourselves.
		if(!fMetafile && pobjmgr->GetSingleSelect() == this)
			DrawFrame(pdp, hdc, &rc);

		else if(fMultipleSelect)
			InvertRect(hdc, &rc);
	}

	RestoreDC(hdc, -1);
}

/*
 *	COleObject::Delete (publdr)
 *
 *	@mfunc	deletes this object from the backing store _without_
 *			making outgoing calls.  The commit on generated anti-events
 *			will handle the outgoing calls
 */
void COleObject::Delete(
	IUndoBuilder *publdr)
{

	Assert(_fInUndo == FALSE);
	_fInUndo = TRUE;

	//REVIEW (keithcu) Too heavy handed?
 //	if (FWrapTextAround())
//		_ped->_pdp->InvalidateRecalc();

	CNotifyMgr *pnm = _ped->GetNotifyMgr();
	if(pnm)
		pnm->Remove((ITxNotify *)this);

	if(publdr)
	{
		// The anti-event will take care of calling IOO::Close for us
		IAntiEvent *pae = gAEDispenser.CreateReplaceObjectAE(_ped, this);
		if(pae)
			publdr->AddAntiEvent(pae);
	}
	else
	{
		Close(OLECLOSE_NOSAVE);
		MakeZombie();
	}

	// If we're being deleted, we can't be selected anymore
	_pi.dwFlags &= ~REO_SELECTED;
	_fDraw = 0;
}

/*
 *	COleObject::Restore()
 *
 *	@mfunc	restores the object from the undo state back into the
 *			backing store
 *
 *			No outgoing calls will be made
 */
void COleObject::Restore()
{
	Assert(_fInUndo);

	_fInUndo = FALSE;
	_fDraw = TRUE;

	CNotifyMgr *pnm = _ped->GetNotifyMgr();
	if(pnm)
		pnm->Add((ITxNotify *)this);
}

/*
 *	COleObject::SetREOSELECTED (fSelect)
 *
 *	@mfunc	cmember set REO_SELECTED state
 */
void COleObject::SetREOSELECTED(
	BOOL fSelect)
{
	_pi.dwFlags &= ~REO_SELECTED;
	if(fSelect)
		_pi.dwFlags |= REO_SELECTED;
}
    
/*
 *	COleObject::Close(dwSave)
 *
 *	@mfunc	closes this object
 */
void COleObject::Close(
	DWORD	dwSave)		//same as IOleObject::Close
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::Close");

	if(!_punkobj)
		return;

	IOleObject *poo;
	if(_punkobj->QueryInterface(IID_IOleObject, (void **)&poo) == NOERROR)
	{
		poo->Close(dwSave);
		poo->Release();
	}
}

				
//
//	PRIVATE methods
//
/*
 *	COleObject::~COleObject()
 *
 *	@mfunc	destructor
 */
COleObject::~COleObject()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::~COleObject");

	CleanupState();
}

/*
 *	COleObject::SavePrivateState()
 *
 *	@mfunc	Saves information such as the aspect and various flags
 *	into the object's storage.
 *
 *	@devnote	This method is used mostly for compatibility with 
 *	richedit 1.0--we save the same information they did.
 *
 *	Also note that this method returns void--even if any particular
 *	call fails, we should be able to "recover" and limp aLONG.
 *	Richedit 1.0 also had this behavior.
 */
void COleObject::SavePrivateState()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::SavePrivateState");
	Assert(_pstg);

	IStream *	pstm;
	HRESULT hr = _pstg->CreateStream(szSiteFlagsStm, STGM_READWRITE |
					STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, 0, &pstm);
    if(IsZombie())
        return;
        
	if(hr == NOERROR)
	{
		pstm->Write(&_pi, sizeof(PersistedInfo), NULL);
		pstm->Release();
	}
}

/*
 *	COleObject::FetchObjectExtents()
 *
 *	@mfunc 	determines the object's size in himetric.  Typically, this
 *			is achieved via IOleObject::GetExtent, but some error 
 *			recovery is implemented
 *
 *	@rdesc	void.  _size is updated
 */
void COleObject::FetchObjectExtents()
{
	HRESULT hr = NOERROR;
	IOleObject *poo;
	IViewObject2 *pvo;

    if(IsZombie())
        return;
        
	// We _don't_ want to make calls to GetExtent if:
	// (1) We have outstanding updates to _size for which we
	//		haven't successfully called SetExtent
	// (2) This is a PaintBrush object and the most recent call 
	//		to SetExtent for this PB object failed

	if(_fIsEBookImage)
	{
		_size.du = _ped->_pdp->DUtoHimetricU(_sizeEBookImage.cx);
		_size.dv = _ped->_pdp->DVtoHimetricV(_sizeEBookImage.cy);
	}
	else
	if(_fAspectChanged || !(_fSetExtent || (_fIsPaintBrush && _fPBUseLocalSize)))
	{	
		// try IOleObject::GetExtent as LONG as we shouldn't try for
		// the metafile first.

		// If this flag was set, it has done its job so turn it off.
		_fAspectChanged = FALSE;

		if(!(_pi.dwFlags & REO_GETMETAFILE))
		{
			hr = _punkobj->QueryInterface(IID_IOleObject, (void **)&poo);
			if(hr == NOERROR)
			{
				hr = poo->GetExtent(_pi.dvaspect, (SIZE*)&_size);
				poo->Release();
			}
			if(IsZombie())
				return;
		}
		else
			hr = E_FAIL;
        
		if(hr != NOERROR)
		{
			if(_punkobj->QueryInterface(IID_IViewObject2, (void **)&pvo) == NOERROR)
			{
				hr = pvo->GetExtent(_pi.dvaspect, -1, NULL, (SIZE*)&_size);
				pvo->Release();
			}
		}

	    if(IsZombie())
	        return;
        
		if(hr != NOERROR || _size.du == 0 || _size.dv == 0)
			_size.du = _size.dv = 2000;
	}
}

/*
 *	COleObject::ConnectObject()
 *
 *	@mfunc	setup the necessary advises to the embedded object.
 *
 *	@rdesc 	HRESULT
 *
 *	@comm	This code is similar to ole2ui's OleStdSetupAdvises
 */
HRESULT COleObject::ConnectObject()
{
	IViewObject *pvo;
	IOleObject *poo;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::ConnectObject");
	
    if(IsZombie())
        return CO_E_RELEASED;
	
	Assert(_punkobj);

	if(_punkobj->QueryInterface(IID_IViewObject, (void **)&pvo) == NOERROR)
	{
		pvo->SetAdvise(_pi.dvaspect, ADVF_PRIMEFIRST, (IAdviseSink *)this);
		pvo->Release();
	}

    if(IsZombie())
        return CO_E_RELEASED;
	
	HRESULT hr = _punkobj->QueryInterface(IID_IOleObject, (void **)&poo);
	if(hr == NOERROR)
	{
		hr = poo->Advise((IAdviseSink *)this, &_dwConn);

		CObjectMgr *pobjmgr = _ped->GetObjectMgr();
		Assert(pobjmgr);		
		if(!pobjmgr)
		{
			poo->Release();
			return E_OUTOFMEMORY;
		}

		// The doc may be NULL, but not the app.  Don't do anything
		// if the app name is NULL
		if(pobjmgr->GetAppName())
		{
			hr = poo->SetHostNames(pobjmgr->GetAppName(), 
						pobjmgr->GetDocName());
		}
		poo->Release();
	}

    if(IsZombie())
        return CO_E_RELEASED;
	
	OleSetContainedObject(_punkobj, TRUE);
	return hr;
}

/*
 *	COleObject::DisconnectObject
 *
 *	@mfunc	reverses the connections made in ConnectObject and releases
 *			the object.  Note that the object's storage is _NOT_
 *			released.
 */
void COleObject::DisconnectObject()
{
	IOleObject * poo = NULL;
	IViewObject *pvo = NULL;

	if(IsZombie())
		return;		// Already Disconnected.

	if(_punkobj->QueryInterface(IID_IOleObject, (void **)&poo) == NOERROR)
	{
		poo->SetClientSite(NULL);
		if(_dwConn)
			poo->Unadvise(_dwConn);
	
		poo->Release();
	}

	if(_punkobj->QueryInterface(IID_IViewObject, (void **)&pvo) == NOERROR)
	{
		pvo->SetAdvise(_pi.dvaspect, ADVF_PRIMEFIRST, NULL);
		pvo->Release();
	}

#ifndef NOINKOBJECT
	SafeReleaseAndNULL((IUnknown**)&_pILineInfo);
#endif

	CoDisconnectObject(_punkobj, NULL);
	SafeReleaseAndNULL(&_punkobj);
}

/*
 *	COleObject::MakeZombie()
 *
 *	@mfunc	Force this object to enter a zombie state.  This
 *      is called when we should be gone but aren't.  It cleans
 *      up our state and flags us so we don't do nasty things
 *		between now and the time were are deleted.
 *
 */
void COleObject::MakeZombie()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::MakeZombie");

	CleanupState();
    Zombie();
}

/*
 *	COleObject::CleanupState()
 *
 *	@mfunc	Called on delete and when we become zombied.  It cleans
 *		up our member data and any other dependencies that need to
 *		be resolved.
 */
void COleObject::CleanupState()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::CleanupState");
	
	CDocInfo *pDocInfo = NULL;
	if(_ped)
	{
		pDocInfo = _ped->GetDocInfoNC();
		if(!_fInUndo)
		{
			CNotifyMgr *pnm = _ped->GetNotifyMgr();
			if(pnm)
				pnm->Remove((ITxNotify *)this);

			_ped = NULL;
		}
	}

	DisconnectObject();

	if(_pstg)
		SafeReleaseAndNULL((IUnknown**)&_pstg);

	if(_hdib)
	{
		::DeleteObject(_hdib);
		_hdib = NULL;
	}

	if(!pDocInfo || _hdata != pDocInfo->_hdata)
		GlobalFree(_hdata);	// Don't free _hdata if background is using it

	_hdata = NULL;
	if(_pimageinfo)
	{
		delete _pimageinfo;
		_pimageinfo = NULL;
	}
}	

/*
 *	COleObject::ActivateObj	(uiMsg, wParam, lParam)
 *	
 *	@mfunc Activates the object.
 *
 *	@rdesc
 *		BOOL	Whether object has been activated.
 */
BOOL COleObject::ActivateObj(
	UINT uiMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	LPOLEOBJECT		poo;
	HWND			hwnd;
	MSG				msg;
	DWORD			dwPos;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::AcitvateObj");

	if(_ped->TxGetWindow(&hwnd) != NOERROR)
		return FALSE;

	// Fill in message structure
	msg.hwnd = hwnd;
	msg.message = uiMsg;
	msg.wParam = wParam;
	msg.lParam = lParam;
	msg.time = GetMessageTime();
	dwPos = GetMessagePos();
	msg.pt.x = (LONG) LOWORD(dwPos);
	msg.pt.y = (LONG) HIWORD(dwPos);

	// Force this object to be selected, if it isn't already
	// Update the selection before making the outgoing call
	if(!(_pi.dwFlags & REO_SELECTED))
	{
		CTxtSelection *psel = _ped->GetSel();
		if(psel)
			psel->SetSelection(_cp, _cp + 1);
	}

	// Execute the primary verb
	if(_punkobj->QueryInterface(IID_IOleObject, (void **)&poo) == NOERROR)
	{
		_fActivateCalled = TRUE;

		// Make sure we tell the object its size has changed if we haven't
		// already notified it.
		if(_fSetExtent)
			SetExtent(SE_ACTIVATING);

		HRESULT	hr;
		RECTUV rc;
		RECT rcPos;

		GetRectuv(rc);
		_ped->_pdp->RectFromRectuv(rcPos, rc);
		hr = poo->DoVerb(OLEIVERB_PRIMARY, &msg, (LPOLECLIENTSITE)this, 0, hwnd, &rcPos);

		if(FAILED(hr))
		{
			ENOLEOPFAILED	enoleopfailed;

			enoleopfailed.iob = _ped->_pobjmgr->FindIndexForCp(GetCp());
			enoleopfailed.lOper = OLEOP_DOVERB;
			enoleopfailed.hr = hr;
	        _ped->TxNotify(EN_OLEOPFAILED, &enoleopfailed);
		}
	    poo->Release();
		if(_fInPlaceActive && !(_pi.dwFlags & REO_INPLACEACTIVE))
		{
			CObjectMgr *pobjmgr = _ped->GetObjectMgr();
			if(!pobjmgr)
			{
				_fActivateCalled = FALSE;
				return FALSE;
			}

			Assert(!pobjmgr->GetInPlaceActiveObject());	
			pobjmgr->SetInPlaceActiveObject(this);
			_pi.dwFlags |= REO_INPLACEACTIVE;
		}
		_fActivateCalled = FALSE;
	}
	else
		return FALSE;

	return TRUE;
}

/*
 *	COleObject::DeActivateObj
 *	
 *	@mfunc Deactivates the object.
 *
 */
HRESULT COleObject::DeActivateObj(void)
{
	IOleInPlaceObject * pipo;
	IOleObject *poo;
	MSG msg;
	HRESULT hr = NOERROR;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "COleObject::DeActivateObj");

	if (_fDeactivateCalled)
	{
		// There are multiple paths through the deactive code. The assumption
		// with this logic is that it is more disconcerting for an app to
		// get multiple deactivate calls than the opposite. This might
		// prove to be an incorrect assumption. This go put in because I
		// added a DeActivateObj call in DeActivateObj where there wasn't
		// one before and I was afraid that this could cause problems because
		// apps might get a call that they didn't have before. It is important
		// to note that the opposite might be the case. (a-rsail).
		return NOERROR;
	}

	_fDeactivateCalled = TRUE;

	if(_punkobj->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo) 
		== NOERROR)
	{
		hr  =_punkobj->QueryInterface(IID_IOleObject, (void **)&poo);
		if(hr == NOERROR) 
		{
			// This code is a bit different from 1.0, but seems to 
			// make things work a bit better.  Basically, we've taken a leaf
			// from various sample apps and do the most brute force "de-activate"
			// possible (you'd think just one call would be enough ;-)

			// Don't bother with the error return here.
			pipo->UIDeactivate();
			
			// Fake something
			ZeroMemory(&msg, sizeof(MSG));
			msg.message = WM_LBUTTONDOWN;	
			_ped->TxGetWindow(&msg.hwnd);

			RECT	rcPos;
			RECTUV rcuv;
			GetRectuv(rcuv);
			_ped->_pdp->RectFromRectuv(rcPos, rcuv);

			// Again, don't bother checking for errors; we need to
			// plow through and get rid of stuff as much as possible.
			poo->DoVerb(OLEIVERB_HIDE, &msg, (IOleClientSite *)this, -1, msg.hwnd, &rcPos);

			// COMPATIBILITY ISSUE (alexgo): the RE1.0 code did some funny
			// stuff with undo here, but I don't think it's necessary now
			// with our multi-level undo model.
			hr = pipo->InPlaceDeactivate();
			poo->Release();
		}
	    pipo->Release();
	}

	_fDeactivateCalled = FALSE;
	return hr; 
}

/*
 *	COleObject::Convert (rclsidNew,	lpstrUserTypeNew)
 *
 *	@mfunc	Converts the object to the specified class.  Does reload
 *		the object but does NOT force an update (caller must do this).
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
HRESULT COleObject::Convert(
	REFCLSID rclsidNew,			//@parm Destination clsid
	LPCSTR	 lpstrUserTypeNew)	//@parm New user type name
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::Convert");

	CLIPFORMAT cfOld;
	CLSID clsidOld;
	LPOLESTR szUserTypeOld = NULL;
	HRESULT hr;
	HRESULT hrLatest;
	UsesMakeOLESTR;


	// If object has no storage, return
	if(!_pstg)
		return ResultFromScode(E_INVALIDARG);

	// Read the old class, format, and user type in
	if ((hr = ReadClassStg(_pstg, &clsidOld)) ||
		(hr = ReadFmtUserTypeStg(_pstg, &cfOld, &szUserTypeOld)))
	{
		return hr;
	}

	// Unload object
	Close(OLECLOSE_SAVEIFDIRTY);
	_punkobj->Release();

    if(IsZombie())
        return CO_E_RELEASED;

	// Write new class and user type, but old format, into storage
	if ((hr = WriteClassStg(_pstg, rclsidNew)) ||
		(hr = WriteFmtUserTypeStg(_pstg, cfOld,
			(LPOLESTR) MakeOLESTR(lpstrUserTypeNew))) ||
		(hr = SetConvertStg(_pstg, TRUE)) ||
		((hr = _pstg->Commit(0)) && (hr = _pstg->Commit(STGC_OVERWRITE))))
	{
		// Uh oh, we're in a bad state; rewrite the original info
		(VOID) WriteClassStg(_pstg, clsidOld);
		(VOID) WriteFmtUserTypeStg(_pstg, cfOld, szUserTypeOld);
	}

    if(IsZombie())
        return CO_E_RELEASED;

	// Reload the object and connect. If we can't reload it, delete it.
	hrLatest = OleLoad(_pstg, IID_IOleObject, (LPOLECLIENTSITE) this,
			(void **)&_punkobj);

	if(hrLatest != NOERROR)
	{
		CRchTxtPtr	rtp(_ped, _cp);

		// we don't want the delete of this object to go on the undo
		// stack.  We use a space so that cp's will work out right for
		// other undo actions.
		rtp.ReplaceRange(1, 1, L" ", NULL, -1);
	}
	else
		ConnectObject();

	// Free the old
	CoTaskMemFree(szUserTypeOld);
	return hr ? hr : hrLatest;
}

/*
 *	COleObject::ActivateAs (rclsid, rclsidAs)
 *
 *	@mfunc	Handles a request by the user to activate all objects of a particular
 *		class as objects of another class.
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
HRESULT COleObject::ActivateAs(
	REFCLSID rclsid,
	REFCLSID rclsidAs)
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::ActivateAs");

	IOleObject * poo = NULL;
	CLSID	clsid;

	// Get clsid of object
	HRESULT hr = _punkobj->QueryInterface(IID_IOleObject, (void **)&poo);
	if(hr == NOERROR)
	{
		// NOTE:  We are depending on the behavior of GetUserClassID to
		// return the current clsid of the object (not the TreatAs id).
		// This should hold true as LONG as LONG as we haven't reloaded
		// it yet.  If there are problems with ActivateAs in the future,
		// this might be a suspect.
		hr = poo->GetUserClassID(&clsid);
		poo->Release();
	}

	if(hr != NOERROR)
		return hr;
	
    if(IsZombie())
        return CO_E_RELEASED;

	// Check to see if object clsid matches clsid to be treated as something
	// else. If it is we need to unload and reload the object.
	if(IsEqualCLSID(clsid, rclsid))
	{
		// Unload object
		Close(OLECLOSE_SAVEIFDIRTY);
		_punkobj->Release();

		if(IsZombie())
			return CO_E_RELEASED;

		// Reload object and connect. If we can't reload it, delete it.
		hr = OleLoad(_pstg, IID_IOleObject, (LPOLECLIENTSITE) this,
				(void **)&_punkobj);

		if(hr != NOERROR)
		{
			CRchTxtPtr	rtp(_ped, _cp);

			// We don't want the delete of this object to go on the undo
			// stack.  We use a space so that cp's will work out right for
			// other undo actions.
			rtp.ReplaceRange(1, 1, L" ", NULL, -1);
		}
		else
			ConnectObject();
	}
	return hr;
}

/*
 *	COleObject::SetLinkAvailable(fAvailable)
 *
 *	@mfunc
 *		Allows client to tell us whether the link is available or not.
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
HRESULT COleObject::SetLinkAvailable(
	BOOL fAvailable)	//@parm	if TRUE, make object linkable
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::SetLinkAvailable");
	
	// If this is not a link, return
	if(!(_pi.dwFlags & REO_LINK))
		return E_INVALIDARG;

	// Set flag as appropriate
	_pi.dwFlags &= ~REO_LINKAVAILABLE;
	if(fAvailable)
		_pi.dwFlags |= REO_LINKAVAILABLE;

	return NOERROR;
}

/*
 *	COleObject::WriteTextInfoToEditStream(pes, CodePage)
 *
 *	@mfunc
 *		Used for textize support,  Tries to determine the text representation
 *		for an object and then writes that info	to the given stream. If
 *		CodePage = 1200 (little-endian Unicode), the object is queried for
 *		Unicode text; else it's queried for Ansi text.
 *
 *	@rdesc
 *		LONG	count of chars written
 */
LONG COleObject::WriteTextInfoToEditStream(
	EDITSTREAM *pes,		//@parm Edit stream to write to
	UINT		CodePage)	//@parm CodePage: 1200 uses Unicode; else Ansi
{
	LONG		 cbWritten = 0;
	BOOL		 fUnicode = CodePage == 1200;
	STGMEDIUM	 med;
	IDataObject *pdataobj = NULL;
	IOleObject * poo;

	HRESULT hr = _punkobj->QueryInterface(IID_IOleObject, (void **)&poo);
	if(hr == NOERROR)
	{
		hr = poo->GetClipboardData(0, &pdataobj);
        poo->Release();
	}

	if(FAILED(hr))
	{
		hr = _punkobj->QueryInterface(IID_IDataObject, (void **)&pdataobj);
		if(FAILED(hr))
		{
			pes->dwError = (DWORD) E_FAIL;
			goto Default;
		}
	}

	med.tymed = TYMED_HGLOBAL;
	med.pUnkForRelease = NULL;
	med.hGlobal = NULL;

	hr = pdataobj->GetData(&g_rgFETC[fUnicode ? iUnicodeFETC : iAnsiFETC], &med);
	if(FAILED(hr))
		pes->dwError = (DWORD)hr;
	else
	{
		HANDLE	hGlobal = med.hGlobal;
		LONG	cb = 0;
		BYTE *	pb = (BYTE *)GlobalLock(hGlobal);
		if(pb)
		{
			if(fUnicode)
				for(; (WCHAR)pb[cb]; cb += 2);
			else
				for(; pb[cb]; cb++);

			pes->dwError = pes->pfnCallback(pes->dwCookie, pb, cb, &cbWritten);
			GlobalUnlock(hGlobal);
		}
		ReleaseStgMedium(&med);
	}

Default:
	if(cbWritten <= 0)
	{
		BYTE rgb[] = {' ', 0};

		pes->pfnCallback(pes->dwCookie, rgb, 1 + fUnicode, &cbWritten);
		pes->dwError = 0;
	}

    pdataobj->Release();
	return cbWritten;
}

/*
 *	COleObject::SetDvaspect (dvaspect)
 *
 *	@mfunc	Allows client to tell us which aspect to use and force us
 *		to recompute positioning and redraw.
 */
void COleObject::SetDvaspect(
	DWORD dvaspect)	//@parm	the aspect to use
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::SetDvaspect");

	_pi.dvaspect = dvaspect;
	
	// Force FetchObjectExtents to call through
	_fAspectChanged = TRUE;

	// Cause ourselves to redraw and update
	OnViewChange(dvaspect, (DWORD) -1);
}

/*
 *	COleObject::HandsOffStorage
 *
 *	@mfunc	See IPersistStore::HandsOffStorage.
 *
 */
void COleObject::HandsOffStorage(void)
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::HandsOffStorage");

	// Free storage we currently have, if we have one.
	SafeReleaseAndNULL((IUnknown**)&_pstg);
}

/*
 *	COleObject::SaveCompleted
 *
 *	@mfunc	See IPersistStore::SaveCompleted.
 */
void COleObject::SaveCompleted(
	LPSTORAGE lpstg)	//@parm	new storage
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "COleObject::SaveCompleted");

	// Did our caller give us a new storage to remember?
	if(lpstg)
	{
		// Free storage we currently have, if we have one
		if(_pstg)
			SafeReleaseAndNULL((IUnknown**)&_pstg);

		// Remember storage we are given, since we are given one
		lpstg->AddRef();
		_pstg = lpstg;
	}
}

/*
 *	SetAllowedResizeDirections
 *	
 *	@func Resizing helper function
 *
 */
static void SetAllowedResizeDirections(
	const POINTUV  & pt,
	const RECTUV   & rc,
	      LPTSTR   lphand,
	      BOOL   & fTop,
	      BOOL   & fBottom,
	      BOOL   & fLeft,
	      BOOL   & fRight)
{
   	fTop = abs(pt.v - rc.top) < abs(pt.v - rc.bottom);
	fBottom = !fTop;
	fLeft = abs(pt.u - rc.left) < abs(pt.u - rc.right);
	fRight = !fLeft;

	if(lphand == IDC_SIZENS)
		fLeft = fRight = FALSE; 
	else if(lphand == IDC_SIZEWE)
		fTop = fBottom = FALSE;
}

/*
 *	COleObject::HandleResize (&ptxy)
 *	
 *	@mfunc Deal with object resizing.
 *
 *	@rdesc	BOOL
 */
BOOL COleObject::HandleResize(
	const POINT &ptxy)
{
	LPTSTR lphand;
	DWORD  dwFlags = _pi.dwFlags;
 	HWND   hwnd;
	RECT   rcxyPos;
	RECTUV rcPos;
	BOOL   fTop, fBottom, fLeft, fRight, fEscape;
	POINTUV	pt;
	CDisplay *pdp = _ped->_pdp;
	int dupMin = pdp->GetDupSystemFont();
	int dvpMin = pdp->GetDvpSystemFont();

	pdp->PointuvFromPoint(pt, ptxy);

	if(!(dwFlags & REO_SELECTED) || !(dwFlags & REO_RESIZABLE) ||
		(lphand = CheckForHandleHit(pt, TRUE)) == NULL)
		return FALSE;
 	
	GetRectuv(rcPos);
	pdp->RectFromRectuv(rcxyPos, rcPos);

	HDC hdc = pdp->GetDC();
	_ped->TxGetWindow(&hwnd);
	SetCapture(hwnd);
	
	SetAllowedResizeDirections(pt, rcPos, lphand, fTop, fBottom, fLeft, fRight);
	
	// Erase and redraw frame without handles.
	DrawFrame(pdp, hdc, &rcxyPos);
	_pi.dwFlags = REO_NULL;
	DrawFrame(pdp, hdc, &rcxyPos);

	fEscape = FALSE;
	const INT vkey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
	while (GetAsyncKeyState(vkey) & 0x8000)
	{		
		POINT ptLast = ptxy;
		POINT ptxyCur;
		POINTUV ptCur;
		MSG msg;

		// Stop if the ESC key has been pressed
		if(GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			fEscape = TRUE;
			break;
		}
		
		GetCursorPos(&ptxyCur);
		ScreenToClient(hwnd, &ptxyCur);
		pdp->PointuvFromPoint(ptCur, ptxyCur);

#ifndef UNDER_CE
// GetCursorPos() isn't supported on WinCE. We have  it hacked to
// be GetMessagePos() which unfortunately in this case will cause
// ptCur to never change. By removing this check we end up drawing
// multiple times when the user pauses during a resize. 
		// If mouse hasn't moved, try again
		if((ptxyCur.x == ptLast.x) && (ptxyCur.y == ptLast.y))
			continue;
#endif

		ptLast = ptxyCur;

		// Erase old rectangle, update rectangle, and redraw
		DrawFrame(pdp, hdc, &rcxyPos);	

		if(fLeft)   rcPos.left   = min(ptCur.u, rcPos.right - dupMin);
		if(fRight)  rcPos.right  = max(ptCur.u, rcPos.left + dupMin);
		if(fTop)    rcPos.top    = min(ptCur.v, rcPos.bottom - dvpMin);
		if(fBottom) rcPos.bottom = max(ptCur.v, rcPos.top + dvpMin);

		pdp->RectFromRectuv(rcxyPos, rcPos);
		DrawFrame(pdp, hdc, &rcxyPos);

		// FUTURE: (joseogl): It would be cool if we could do something
		// better here, but for now, it appears to be necessary.
		Sleep(100);
		
		// Eat input messages
		if (PeekMessage(&msg, 0, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) ||
			PeekMessage(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE | PM_NOYIELD) ||
			PeekMessage(&msg, 0, WM_NCMOUSEMOVE, WM_NCMBUTTONDBLCLK, PM_REMOVE | PM_NOYIELD))
		{
			// Break out of the loop if the Escape key was pressed
		    if(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
			{
	        	fEscape = TRUE;
				break;
			}
		}
	}

	DrawFrame(pdp, hdc, &rcxyPos);
  	ReleaseCapture();
 	_pi.dwFlags = dwFlags;

	// Resize unless the user aborted
	if(!fEscape)
	{
		SIZEUV size;
		if (IsUVerticalTflow(pdp->GetTflow()) && !(_pi.dwFlags & REO_CANROTATE))
		{
			size.du = rcPos.bottom - rcPos.top;
			size.dv = rcPos.right - rcPos.left;
		}
		else
		{
			size.du = rcPos.right - rcPos.left;
			size.dv = rcPos.bottom - rcPos.top;
		}

		size.du = pdp->DUtoHimetricU(size.du);
		size.dv = pdp->DVtoHimetricV(size.dv);
		Resize(size, TRUE);
	}

	pdp->ReleaseDC(hdc);
	return TRUE;
}

/*
 *	COleObject::Resize(size)
 *	
 *	@mfunc Set new object size.  Handle undo details.
 */
void COleObject::Resize(
	const SIZEUV &size, 
	BOOL  fCreateAntiEvent)
{
	CDisplay *	pdp = _ped->_pdp;
	SIZEUV		sizeold = _size;

	// Change the size of our internal representation.
	_size = size;

	//If size didn't really change, don't do anything else.
	if(size.du != sizeold.du || size.dv != sizeold.dv)
	{
		if(_ped->_fUseUndo && fCreateAntiEvent)
		{
			CGenUndoBuilder undobldr(_ped, UB_AUTOCOMMIT);
			IAntiEvent *pae;

			pae = gAEDispenser.CreateResizeObjectAE(_ped, this, sizeold);
			if(pae)
				undobldr.AddAntiEvent(pae);
		}

		SetExtent(SE_NOTACTIVATING);

		// Force a redraw that will stretch the object.
		pdp->OnPostReplaceRange(CP_INFINITE, 0, 0, _cp, _cp + 1, NULL);

		_ped->GetCallMgr()->SetChangeEvent(CN_GENERIC);
	}
}

/*
 *	COleObject::OnReposition ()
 *	
 *	@mfunc Set object's new position.  May have changed as a result of scrolling.
 */
void COleObject::OnReposition()
{
	IOleInPlaceObject *pipo;

	if(!_fInPlaceActive)
	{
		// If we're not inplace active, don't do anything
		return;
	}

	if(_punkobj->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo) == NOERROR)
	{
		RECTUV rcuvClip, rcuv;
		RECT rcClip, rc;

		_ped->_pdp->GetViewRect(rcuvClip);
		_ped->_pdp->RectFromRectuv(rcClip, rcuvClip);

		GetRectuv(rcuv);
		_ped->_pdp->RectFromRectuv(rc, rcuv);

		pipo->SetObjectRects(&rc, &rcClip);
        pipo->Release();
	}
}

 /*
 *	COleObject::GetRectuv(rc)
 *	
 *	@mfunc Compute the object's position rectangle from its cp.
 */
void COleObject::GetRectuv(
	 RECTUV &rc)
{
	CRchTxtPtr rtp(_ped, _cp);
	POINTUV pt;
	CDispDim dispdim;

	DWORD grf = TA_BASELINE;
	if (_pi.dwFlags & REO_BELOWBASELINE)
		grf = TA_BOTTOM;
	
	if(_ped->_pdp->PointFromTp(rtp, NULL, FALSE, pt, NULL, grf, &dispdim) == -1)
		return;

	rc.left = pt.u;
	rc.right = rc.left + dispdim.dup;
	rc.bottom = pt.v;

	LONG dv = _size.dv;
	if (IsUVerticalTflow(_ped->_pdp->GetTflow()) && !(_pi.dwFlags & REO_CANROTATE))
		dv = _size.du;

	rc.top = rc.bottom - _ped->_pdp->HimetricVtoDV(dv);
}

#ifdef DEBUG
void COleObject::DbgDump(DWORD id)
{
	Tracef(TRCSEVNONE, "Object #%d %X: cp = %d , ",id,this,_cp);
}
#endif

/*	
 *	COleObject:SetExtent(iActivating)
 *
 *	@mfunc A wrapper around IOleObject::SetExtent which makes some additional
 *			checks if the first call to IOleObject::SetExtent fails.  
 *
 *	@rdesc HRESULT
 */
HRESULT COleObject::SetExtent(
	int iActivating) //@parm indicates if object is currently being activated
{
	LPOLEOBJECT poo;

	// If we are connected to a link object, the native extent can't be change,
	// so don't bother doing anything here.
	if(_pi.dwFlags & REO_LINK)
	{
		// So we don't call GetExtents on remeasuring.
		_fSetExtent = TRUE;
		return NOERROR;
	}

	HRESULT hr = _punkobj->QueryInterface(IID_IOleObject, (void **)&poo);
	if(hr != NOERROR)
		return hr;

	// If we are about to activate the object, fall through and OleRun the
	// object prior to attempting to SetExtent.  Otherwise, attempt a SetExtent
	// directly.
	if(iActivating == SE_NOTACTIVATING)
	{
		// By default, we will call SetExtent when the object is next activated.
		_fSetExtent = TRUE;

		hr = poo->SetExtent(_pi.dvaspect, (SIZE*)&_size);

		DWORD dwStatus;

		// If the server is not running we need to to some additional
		// checking. If it was, we do not need to call SetExtent again.

		// Find out if OLEMISC_RECOMPOSEONRESIZE is set.  If it is, we should
		// run the object and call setextent.  If not, we defer calling set
		// extent until we are ready to activate the object.
		if(!(hr == OLE_E_NOTRUNNING &&
			poo->GetMiscStatus(_pi.dvaspect, &dwStatus) == NOERROR &&
			(dwStatus & OLEMISC_RECOMPOSEONRESIZE)))
		{
			goto DontRunAndSetAgain;
		}
		// Fall through and attempt the SetExtent again after running the object
	}

	SIZEUV sizesave;
	sizesave = _size;
	OleRun(_punkobj);		// This call causes _size to be reset 
							// via OLE and FetchObjectExtents.
	_size = sizesave;
	poo->SetExtent(_pi.dvaspect, (SIZE*)&_size);

DontRunAndSetAgain:
	if((hr == NOERROR) || 
		(iActivating == SE_NOTACTIVATING && hr != OLE_E_NOTRUNNING))
	{
		_fSetExtent = FALSE;
	}
	// If the server is still not running, we try again at
	// activation time.  Otherwise the server has either 
	// done its thing or it doesn't do resize.  Either way
	// we don't bother trying again at activation time.

	if(hr == NOERROR && _fIsPaintBrush)
	{
		SIZEUV sizeChk;

		poo->GetExtent(_pi.dvaspect, (SIZE*)&sizeChk);
		_fPBUseLocalSize = !(sizeChk.du == _size.du && sizeChk.dv == _size.dv);
		// HACK:  Calls to SetExtent on PaintBrush objects may not
		// 	actually change the object extents.  In such cases, 
		//	we will rely on local _size for PaintBrush object extents.
	}
	poo->Release();
	return hr;
}
