#include "souistd.h"
#include "core/SNcPainter.h"
#include "core/SHostWnd.h"

SNSBEGIN

SNcPainter::SNcPainter(SHostWnd * pHost):m_pHost(pHost),m_root(NULL),m_htPart(0),m_bInPaint(FALSE),m_bSysNcPainter(FALSE)
{
}

SNcPainter::~SNcPainter(void)
{
}

void SNcPainter::Reset()
{
	m_borderWidth = SLayoutSize();
	m_titleHeight = SLayoutSize();
	m_skinBorder=NULL;
	m_memRT = NULL;
	m_memTop=NULL;
	m_memLeft=NULL;
	m_memBottom=NULL;
	m_memRight=NULL;
	m_htPart = 0;
	m_bSysNcPainter=FALSE;
	if(m_root)
	{
		m_root->Release();
		m_root = NULL;
	}
}

BOOL SNcPainter::InitFromXml(THIS_ IXmlNode *pXmlNode)
{
	if(!pXmlNode)
	{
		Reset();
		return TRUE;
	}
	else
	{
		__baseCls::InitFromXml(pXmlNode);
		SXmlNode xmlNode(pXmlNode);
		GETRENDERFACTORY->CreateRenderTarget(&m_memRT,0,0);
		GETRENDERFACTORY->CreateRenderTarget(&m_memLeft,0,0);
		GETRENDERFACTORY->CreateRenderTarget(&m_memRight,0,0);
		GETRENDERFACTORY->CreateRenderTarget(&m_memTop,0,0);
		GETRENDERFACTORY->CreateRenderTarget(&m_memBottom,0,0);
		m_root = new SOsrPanel(this,xmlNode,this);
		return TRUE;
	}
}

IWindow* SNcPainter::GetRoot(THIS)
{
	return m_root;
}


SWindow* SNcPainter::GetSRoot()
{
	return m_root;
}

int SNcPainter::GetScale() const
{
	return m_pHost->GetScale();
}

LRESULT SNcPainter::OnNcCalcSize(BOOL bCalcValidRects, LPARAM lParam)
{
	if(m_titleHeight.isZero() && m_borderWidth.isZero()) 
	{
		LPNCCALCSIZE_PARAMS pParam = (LPNCCALCSIZE_PARAMS)lParam;
		SNativeWnd *pNative = m_pHost->GetNative();
		if (bCalcValidRects && (pNative->GetStyle() & WS_CHILDWINDOW))
		{
			//�Ӵ��ڣ�����ڸ���������
			if (SWP_NOSIZE & pParam->lppos->flags)
				return 0;

			CRect rcWindow;
			pNative->GetWindowRect(rcWindow);
			POINT point = { rcWindow.left, rcWindow.top };
			::ScreenToClient(::GetParent(pNative->m_hWnd), &point);
			int w = rcWindow.Width();
			int h = rcWindow.Height();

			rcWindow.left = point.x;
			rcWindow.top = point.y;
			rcWindow.right = rcWindow.left + w;
			rcWindow.bottom = rcWindow.top + h;

			if (0 == (SWP_NOMOVE & pParam->lppos->flags))
			{
				rcWindow.left = pParam->lppos->x;
				rcWindow.top = pParam->lppos->y;
			}

			rcWindow.right = rcWindow.left + pParam->lppos->cx;
			rcWindow.bottom = rcWindow.top + pParam->lppos->cy;
			pParam->rgrc[0] = rcWindow;
		}
		else if (bCalcValidRects)
		{
			// top-level �������Ļ����
			if (SWP_NOSIZE & pParam->lppos->flags)
				return 0;

			CRect rcWindow;
			pNative->GetWindowRect(rcWindow);
			rcWindow = CRect(rcWindow.TopLeft(), CSize(pParam->lppos->cx, pParam->lppos->cy));
			if (0 == (SWP_NOMOVE & pParam->lppos->flags))
			{
				rcWindow.MoveToXY(pParam->lppos->x, pParam->lppos->y);
			}
			pParam->rgrc[0] = rcWindow;
		}
		else
		{
			pNative->GetWindowRect((LPRECT)lParam);
		}
	}else
	{
		if (bCalcValidRects && IsDrawNc() && !m_pHost->IsIconic())   
		{   
			LPNCCALCSIZE_PARAMS pParam = (LPNCCALCSIZE_PARAMS)lParam;
			CRect rcWnd = pParam->rgrc[0];
			rcWnd.MoveToXY(0,0);
			CRect& rc = (CRect&)pParam->rgrc[0];//get the client rectangle   
			int nBorderWid = m_borderWidth.toPixelSize(GetScale());
			int nTitleHei = m_titleHeight.toPixelSize(GetScale());
			rc.DeflateRect(nBorderWid,nBorderWid);
			rc.top += nTitleHei;

			CRect rcCaption(0,0,rc.Width(),nTitleHei);
			m_memRT->Resize(rcCaption.Size());
			m_memRT->SetViewportOrg(CPoint(-nBorderWid,-nBorderWid));

			m_root->Move(rcCaption);
			m_root->UpdateLayout();

			m_memLeft->Resize(CSize(nBorderWid,rcWnd.Height()));
			m_skinBorder->DrawByIndex(m_memLeft,&rcWnd,0);

			m_memRight->Resize(CSize(nBorderWid,rcWnd.Height()));
			m_memRight->SetViewportOrg(CPoint(-(rcWnd.right-nBorderWid),0));
			m_skinBorder->DrawByIndex(m_memRight,&rcWnd,0);

			m_memTop->Resize(CSize(rcWnd.Width()-2*nBorderWid,nBorderWid));
			m_memTop->SetViewportOrg(CPoint(-nBorderWid,0));
			m_skinBorder->DrawByIndex(m_memTop,&rcWnd,0);

			m_memBottom->Resize(CSize(rcWnd.Width()-2*nBorderWid,nBorderWid));
			m_memBottom->SetViewportOrg(CPoint(-nBorderWid,-(rcWnd.bottom-nBorderWid)));
			m_skinBorder->DrawByIndex(m_memBottom,&rcWnd,0);

		}else
		{
			SetMsgHandled(FALSE);
		}
	}
	return 0;
}


BOOL SNcPainter::OnNcActivate(BOOL bActive)
{
	if(IsDrawNc())
	{
		DWORD dwStyle = m_pHost->GetStyle();
		if(dwStyle&WS_VISIBLE)
			m_pHost->SetWindowLongPtr(GWL_STYLE,dwStyle&~WS_VISIBLE);
		m_pHost->DefWindowProc();
		if(dwStyle&WS_VISIBLE)
			m_pHost->SetWindowLongPtr(GWL_STYLE,dwStyle);

		m_root->EnableWindow(bActive);
		m_pHost->SendMessage(WM_NCPAINT);
	}
	return TRUE;
}

UINT SNcPainter::OnNcHitTest(CPoint point)
{
	if(m_pHost->IsIconic())
		return 0;
	m_htPart = HTCLIENT;
	if(IsDrawNc())
	{
		do{
		CRect rcWnd = m_pHost->GetWindowRect();
		CRect rcClient = m_pHost->GetClientRect();
		m_pHost->ClientToScreen2(&rcClient);
		if(rcClient.PtInRect(point))
		{
			m_htPart = HTCLIENT;
			break;
		}
		CRect rcInner = rcWnd;
		int nBorderWid = m_borderWidth.toPixelSize(GetScale());
		rcInner.DeflateRect(nBorderWid,nBorderWid);
		CRect rcTitle = rcInner;
		rcTitle.bottom = rcClient.top;
		if(rcTitle.PtInRect(point))
		{
			point-=rcTitle.TopLeft();
			SWND swnd = m_root->SwndFromPoint(point);
			if(swnd)
			{
				SWindow *pWnd = SWindowMgr::GetWindow(swnd);
				int nId = pWnd->GetID();
				if(nId == IDC_SYS_ICON)
				{
					m_htPart = HTSYSMENU;
					break;
				}
			}
			m_htPart = HTCAPTION;
			break;
		}

		if(m_pHost->m_hostAttr.m_bResizable)
		{
			//depart border region into 8 parts as expressed below
			//012
			//3*4
			//567
			CRect rcParts[8];
			UINT  htPart[8];
			
			htPart[0] = HTTOPLEFT; rcParts[0]=CRect(rcWnd.left,rcWnd.top,rcInner.left,rcInner.top);
			htPart[1] = HTTOP; rcParts[1]=CRect(rcInner.left,rcWnd.top,rcInner.right,rcInner.top);
			htPart[2] = HTTOPRIGHT; rcParts[2]=CRect(rcInner.right,rcWnd.top,rcWnd.right,rcInner.top);
			htPart[3] = HTLEFT; rcParts[3]=CRect(rcWnd.left,rcInner.top,rcInner.left,rcInner.bottom);
			htPart[4] = HTRIGHT; rcParts[4]=CRect(rcInner.left,rcInner.top,rcWnd.right,rcInner.bottom);
			htPart[5] = HTBOTTOMLEFT; rcParts[5]=CRect(rcWnd.left,rcInner.top,rcInner.left,rcWnd.bottom);
			htPart[6] = HTBOTTOM; rcParts[6]=CRect(rcInner.left,rcInner.bottom,rcInner.right,rcWnd.bottom);
			htPart[7] = HTBOTTOMRIGHT; rcParts[7]=CRect(rcInner.right,rcInner.bottom,rcWnd.right,rcWnd.bottom);
			for(int i=0;i<8;i++)
			{
				if(rcParts[i].PtInRect(point))
				{
					m_htPart = htPart[i];
					break;
				}
			}
			if(m_htPart==HTCLIENT)
				m_htPart = HTBORDER;
		}else
		{
			m_htPart = HTBORDER;
		}
		}while(false);
	}
	else if (m_pHost->m_hostAttr.m_bResizable)
	{
		do{
		m_pHost->ScreenToClient(&point);
		CRect rcMargin = m_pHost->m_hostAttr.GetMargin(GetScale());
		CRect rcWnd = m_pHost->GetRoot()->GetWindowRect();
		if (point.x > rcWnd.right - rcMargin.right)
		{
			if (point.y > rcWnd.bottom - rcMargin.bottom)
			{
				m_htPart = HTBOTTOMRIGHT;
			}
			else if (point.y < rcMargin.top)
			{
				m_htPart = HTTOPRIGHT;
			}else
			{
				m_htPart = HTRIGHT;
			}
		}
		else if (point.x < rcMargin.left)
		{
			if (point.y > rcWnd.bottom - rcMargin.bottom)
			{
				m_htPart = HTBOTTOMLEFT;
			}
			else if (point.y < rcMargin.top)
			{
				m_htPart = HTTOPLEFT;
			}else
			{
				m_htPart = HTLEFT;
			}
		}
		else if (point.y > rcWnd.bottom - rcMargin.bottom)
		{
			m_htPart = HTBOTTOM;
		}
		else if (point.y < rcMargin.top)
		{
			m_htPart = HTTOP;
		}
		}while(false);
	}
	return m_htPart;
}

void SNcPainter::OnNcPaint(HRGN hRgn)
{
	if(!IsDrawNc())
		return;
	//paint non client.
	CRect rcWnd = m_pHost->GetWindowRect();
	if ((WORD)hRgn > 1 && !::RectInRegion(hRgn, &rcWnd)) {
		m_pHost->DefWindowProc();// just do default thing
		return;						// and quit
	}

	m_bInPaint = TRUE;

	HDC hdc = ::GetWindowDC(m_pHost->m_hWnd);
	CRect rcClient = m_pHost->GetClientRect();
	m_pHost->ClientToScreen2(&rcClient);
	rcClient.OffsetRect(-rcWnd.TopLeft());
	::ExcludeClipRect(hdc,rcClient.left,rcClient.top,rcClient.right,rcClient.bottom);
	rcWnd.MoveToXY(0,0);
	int nBorderWid = m_borderWidth.toPixelSize(GetScale());
	int nTitleHei = m_titleHeight.toPixelSize(GetScale());

	//draw left
	{
		CRect rcDraw(0,0,nBorderWid,rcWnd.bottom);
		{
			HDC memdc = m_memLeft->GetDC(0);
			BitBlt(hdc,rcDraw.left,rcDraw.top,rcDraw.Width(),rcDraw.Height(),memdc,rcDraw.left,rcDraw.top,SRCCOPY);
			m_memLeft->ReleaseDC(memdc);
		}
	}
	//draw top
	{
		CRect rcDraw(nBorderWid,0,rcWnd.right-nBorderWid,nBorderWid);
		{
			HDC memdc = m_memTop->GetDC(0);
			BitBlt(hdc,rcDraw.left,rcDraw.top,rcDraw.Width(),rcDraw.Height(),memdc,rcDraw.left,rcDraw.top,SRCCOPY);
			m_memTop->ReleaseDC(memdc);
		}
	}
	//draw right
	{
		CRect rcDraw(rcWnd.right-nBorderWid,0,rcWnd.right,rcWnd.bottom);
		{
			HDC memdc = m_memRight->GetDC(0);
			BitBlt(hdc,rcDraw.left,rcDraw.top,rcDraw.Width(),rcDraw.Height(),memdc,rcDraw.left,rcDraw.top,SRCCOPY);
			m_memRight->ReleaseDC(memdc);
		}
	}
	//draw bottom
	{
		CRect rcDraw(nBorderWid,rcWnd.bottom-nBorderWid,rcWnd.right-nBorderWid,rcWnd.bottom);
		{
			HDC memdc = m_memBottom->GetDC(0);
			BitBlt(hdc,rcDraw.left,rcDraw.top,rcDraw.Width(),rcDraw.Height(),memdc,rcDraw.left,rcDraw.top,SRCCOPY);
			m_memBottom->ReleaseDC(memdc);
		}
	}

	{//draw title
		CRect rcClip;
		m_memRT->GetClipBox(&rcClip);
		CRect rcTitle(nBorderWid,nBorderWid,rcWnd.Width()-nBorderWid,nTitleHei+nBorderWid);
		rcTitle.IntersectRect(rcTitle,rcClip);

		m_root->Draw(m_memRT,rcTitle);
		HDC memdc = m_memRT->GetDC(0);
		::BitBlt(hdc,rcTitle.left,rcTitle.top,rcTitle.Width(),rcTitle.Height(),memdc,rcTitle.left,rcTitle.top,SRCCOPY);
		m_memRT->ReleaseDC(memdc);

	}

	::ReleaseDC(m_pHost->m_hWnd,hdc);
	m_bInPaint = FALSE;
}

BOOL SNcPainter::IsDrawNc() const
{
	return !m_pHost->IsTranslucent() && !(m_borderWidth.isZero()||m_titleHeight.isZero());
}

LRESULT SNcPainter::OnSetText(LPCTSTR pszText)
{
	if(!IsDrawNc())
	{
		return m_pHost->DefWindowProc();
	}else
	{
		//
		DWORD dwStyle = m_pHost->GetStyle();
		if (dwStyle & WS_VISIBLE)
			m_pHost->SetWindowLongPtr(GWL_STYLE, dwStyle & ~ WS_VISIBLE);
		m_pHost->DefWindowProc();
		if (dwStyle & WS_VISIBLE)
			m_pHost->SetWindowLongPtr(GWL_STYLE, dwStyle);
		SWindow *pRoot = GetSRoot();
		if(pRoot)
		{
			SWindow *pTitle = pRoot->FindChildByID(IDC_SYS_TITLE);
			if(pTitle) 
			{
				pTitle->SetWindowText(pszText);
			}
		}
		return 0;
	}
}

LRESULT SNcPainter::OnRepaint(UINT msg,WPARAM wp,LPARAM lp)
{
	if(!IsDrawNc())
	{
		return m_pHost->DefWindowProc();
	}else
	{
		SWindow *pRoot = GetSRoot();
		if(pRoot)
		{
			InvalidateHostRect(NULL);
		}
	}
	return 0;
}

void SNcPainter::OnNcDestroy()
{
	if(m_root)
	{
		m_root->Release();
		m_root = NULL;
	}
	m_memRT=NULL;
	m_memTop=NULL;
	m_memLeft=NULL;
	m_memBottom=NULL;
	m_memRight=NULL;

	SetMsgHandled(FALSE);
}

void SNcPainter::OnItemSetCapture(SOsrPanel *pItem, BOOL bCapture)
{
}

BOOL SNcPainter::OnItemGetRect(const SOsrPanel *pItem, CRect &rcItem) const
{
	rcItem = GetHostRect();
	return TRUE;
}

BOOL SNcPainter::IsItemRedrawDelay() const
{
	return FALSE;
}

IObject * SNcPainter::GetHost()
{
	return this;
}

BOOL SNcPainter::OnFireEvent(IEvtArgs *e)
{
	EventCmd *e2=sobj_cast<EventCmd>(e);
	if(e2)
	{
		bool bSysBtn = true;
		switch(e2->idFrom)
		{
		case IDC_SYS_CLOSE:
			m_pHost->PostMessage(WM_SYSCOMMAND,SC_CLOSE);
			break;
		case IDC_SYS_MIN:
			m_pHost->PostMessage(WM_SYSCOMMAND,SC_MINIMIZE);
			break;
		case IDC_SYS_MAX:
			m_pHost->PostMessage(WM_SYSCOMMAND,SC_MAXIMIZE);
			break;
		case IDC_SYS_RESTORE:
			m_pHost->PostMessage(WM_SYSCOMMAND,SC_RESTORE);
			break;
		default:
			bSysBtn = false;
			break;
		}
		if(bSysBtn)
			return TRUE;
	}
	return m_pHost->OnFireEvent(e);
}

BOOL SNcPainter::IsHostUpdateLocked() const
{
	return FALSE;
}

BOOL SNcPainter::IsHostVisible() const
{
	return TRUE;
}

CRect SNcPainter::GetHostRect() const
{
	CRect rcItem = m_pHost->GetWindowRect();
	rcItem.MoveToXY(0,0);
	int nBorderWid=m_borderWidth.toPixelSize(GetScale());
	int nTitleHei = m_titleHeight.toPixelSize(GetScale());
	rcItem.DeflateRect(nBorderWid,nBorderWid);
	rcItem.bottom = rcItem.top + nTitleHei;
	return rcItem;
}

void SNcPainter::InvalidateHostRect(LPCRECT pRc)
{
	if(m_bInPaint)
		return;
	m_memRT->SaveClip(NULL);
	if(pRc)
		m_memRT->PushClipRect(pRc,RGN_OR);
	else
	{
		CRect rc=m_root->GetClientRect();
		m_memRT->PushClipRect(&rc,RGN_OR);
	}
	m_pHost->SendMessage(WM_NCPAINT);
	m_memRT->PopClip();
}

ISwndContainer * SNcPainter::GetHostContainer()
{
	return m_pHost;
}

IRenderTarget * SNcPainter::OnGetHostRenderTarget(LPCRECT rc, GrtFlag gdcFlags)
{
	m_memRT->PushClipRect(rc,RGN_AND);
	return m_memRT;
}

void SNcPainter::OnReleaseHostRenderTarget(IRenderTarget *pRT,LPCRECT rc,GrtFlag gdcFlags)
{
	m_memRT->PopClip();

	HDC hdc = ::GetWindowDC(m_pHost->m_hWnd);
	int nBorderWid = m_borderWidth.toPixelSize(GetScale());
	HDC memdc = m_memRT->GetDC(0);
	::BitBlt(hdc,rc->left,rc->top,RectWidth(rc),RectHeight(rc),memdc,rc->left,rc->top,SRCCOPY);
	m_memRT->ReleaseDC(memdc);
	::ReleaseDC(m_pHost->m_hWnd,hdc);
}


LRESULT SNcPainter::OnNcMouseEvent(UINT msg,WPARAM wp,LPARAM lp)
{
	if(m_htPart == HTCAPTION && msg != WM_NCLBUTTONDBLCLK)
	{
		CPoint pt(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));
		CRect rcWnd = m_pHost->GetWindowRect();
		pt -= rcWnd.TopLeft();
		int nBorder = m_borderWidth.toPixelSize(GetScale());
		pt.x-=nBorder;
		pt.y-=nBorder;
		lp=MAKELPARAM(pt.x,pt.y);
		msg += WM_MOUSEMOVE - WM_NCMOUSEMOVE; //translate nc mouse event to normal mouse event.
		m_root->DoFrameEvent(msg,wp,lp);
		UpdateToolTip();
	}else
	{
		m_pHost->DefWindowProc();
	}
	return 0;
}

LRESULT SNcPainter::OnNcMouseLeave(UINT msg,WPARAM wp,LPARAM lp)
{
	m_root->DoFrameEvent(WM_MOUSELEAVE,wp,lp);
	UpdateToolTip();
	return m_pHost->DefWindowProc();
}

void SNcPainter::OnNcLButtonDown(UINT flag,CPoint pt)
{
	if(m_htPart == HTCAPTION)
	{
		CRect rcWnd = m_pHost->GetWindowRect();
		pt -= rcWnd.TopLeft();
		int nBorder = m_borderWidth.toPixelSize(GetScale());
		pt.x-=nBorder;
		pt.y-=nBorder;

		CPoint pt2(pt);
		SWND swnd = m_root->SwndFromPoint(pt2);
		if(swnd != 0)
		{
			LPARAM lp=MAKELPARAM(pt.x,pt.y);
			m_root->DoFrameEvent(WM_LBUTTONDOWN,flag,lp);
		}else
		{
			m_pHost->DefWindowProc();
		}
	}else
	{
		m_pHost->DefWindowProc();
	}
}

int SNcPainter::toNcBuiltinID(const SStringW & strValue)
{
	static struct SystemID
	{
		int id;
		LPCWSTR pszName;
	} systemID[] = { IDC_SYS_ICON,   L"sysid_icon",   IDC_SYS_TITLE, L"sysid_title", IDC_SYS_CLOSE,  L"sysid_close",
		IDC_SYS_MIN, L"sysid_min", IDC_SYS_MAX,  L"sysid_max",  IDC_SYS_RESTORE,    L"sysid_restore"};
	if (!strValue.IsEmpty())
	{
		if (strValue.Left(5).CompareNoCase(L"sysid") == 0)
		{
			for (int i = 0; i < ARRAYSIZE(systemID); i++)
			{
				if (strValue.CompareNoCase(systemID[i].pszName) == 0)
				{
					return systemID[i].id;
				}
			}
		}
	}
	return 0;
}

void SNcPainter::OnSize(UINT nType, CSize size)
{
	if(m_root)
	{
		SWindow *pMax = m_root->FindChildByID(IDC_SYS_MAX);
		SWindow *pRestore = m_root->FindChildByID(IDC_SYS_RESTORE);
		if(pMax && pRestore)
		{
			if(nType==SIZE_MAXIMIZED)
			{
				pMax->SetVisible(FALSE);
				pRestore->SetVisible(TRUE);
			}else if(nType==SIZE_RESTORED)
			{
				pMax->SetVisible(TRUE);
				pRestore->SetVisible(FALSE);
			}
		}
	}
}

void SNcPainter::UpdateToolTip()
{
	{
		CPoint pt;
		GetCursorPos(&pt);
		CRect rcWnd= m_pHost->GetWindowRect();
		pt-= rcWnd.TopLeft();

		SwndToolTipInfo tipInfo;
		BOOL bOK = m_root->UpdateToolTip(pt, tipInfo);
		if (bOK)
		{
			m_pHost->_SetToolTipInfo(&tipInfo,TRUE);
		}
		else
		{ // hide tooltip
			m_pHost->_SetToolTipInfo(NULL,TRUE);
		}
	}
}

CSize SNcPainter::GetNcSize() const
{
	CSize szRet;
	int nBorderWid=m_borderWidth.toPixelSize(GetScale());
	int nTitleHei = m_titleHeight.toPixelSize(GetScale());
	szRet.cx = nBorderWid*2;
	szRet.cy = nBorderWid*2 + nTitleHei;
	return szRet;
}


SNSEND