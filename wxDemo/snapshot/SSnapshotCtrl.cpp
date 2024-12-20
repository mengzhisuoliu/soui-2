#include "stdafx.h"
#include "SSnapshotCtrl.h"

SSnapshotCtrl::SSnapshotCtrl(void)
{
	m_bCapture = false;

	m_emPosType = Null;
}

SSnapshotCtrl::~SSnapshotCtrl(void)
{
}

void SSnapshotCtrl::OnPaint(IRenderTarget* pRT)
{
	HDC hDC = pRT->GetDC(0);
	CDC dcCompatible;
	dcCompatible.CreateCompatibleDC(hDC);
	dcCompatible.SelectBitmap(*m_pBitmap);

	SIZE szBMP;
	m_pBitmap->GetSize(szBMP);
	CRect rcWindow(0, 0, szBMP.cx, szBMP.cy);

	BitBlt(hDC, 0, 0, szBMP.cx, szBMP.cy, dcCompatible, 0, 0, SRCCOPY);

	//���ƻ�ɫ����
	{
		CAutoRefPtr<IPath> path;
		GETRENDERFACTORY->CreatePath(&path);
		path->addRect(rcWindow);

		//CRect rcCapture(200, 200, 1000, 1000);
		pRT->PushClipRect(m_rcCaptureArea, RGN_XOR);

		CAutoRefPtr<IBrush> brush, oldbrush;
		pRT->CreateSolidColorBrush(RGBA(0, 0, 0, 100), &brush);
		pRT->SelectObject(brush, (IRenderObj**)&oldbrush);
		pRT->FillPath(path);
		pRT->SelectObject(oldbrush, NULL);
	}


	{//����8��������
        CAutoRefPtr<IBrush> brush, oldbrush;
        pRT->CreateSolidColorBrush(RGBA(255, 0, 0, 255), &brush);
        pRT->SelectObject(brush, (IRenderObj **)&oldbrush);
        for (int i = 0; i < 8; ++i)
        {
            CRect rcDot(m_rcPos[i]);
            pRT->FillRectangle(rcDot);
        }
        pRT->SelectObject(oldbrush, NULL);
    }
}

void SSnapshotCtrl::OnMouseMove(UINT nFlags, SOUI::CPoint point)
{
	SetMsgHandled(FALSE);
	SOUI::CRect rcWnd = GetWindowRect();

	if ((nFlags & MK_LBUTTON))
	{
		if (m_ptDown == point)
			return;

		switch (m_emPosType)
		{
		case Null:
		{
			if (m_bCapture) return;
			if (m_ptDown == point)
				return;

			m_rcCaptureArea.SetRect(m_ptDown, point);
			m_rcCaptureArea.NormalizeRect();
		}
		break;
		case TopLeft:
			break;
		case TopCenter:
			break;
		case TopRight:
			break;
		case RightCenter:
			break;
		case BottomRight:
			break;
		case BottomCenter:
			break;
		case BottomLeft:
			break;
		case LeftCenter:
			break;
		case SelectRect:
		{
			SOUI::CPoint ptLT = point - m_ptDown;			// ��� ����� ʱ  �� ƫ����  Ҳ���� �ƶ� �� ֵ 
			if (ptLT.x < rcWnd.left)
				ptLT.x = rcWnd.left;
			else if (ptLT.x > rcWnd.right - m_rcCaptureArea.Width())
				ptLT.x = rcWnd.right - m_rcCaptureArea.Width();
			if (ptLT.y < rcWnd.top)
				ptLT.y = rcWnd.top;
			else if (ptLT.y > rcWnd.bottom - m_rcCaptureArea.Height())
				ptLT.y = rcWnd.bottom - m_rcCaptureArea.Height();
			m_rcCaptureArea.MoveToXY(ptLT);
		}
		break;
		default:
			break;
		}
	}

	CalcPos();
	Invalidate();
}

void SSnapshotCtrl::OnLButtonDown(UINT nFlags, SOUI::CPoint point)
{
	SetCapture();
	
	if (m_rcCaptureArea.IsRectEmpty())
	{
		m_emPosType = Null;
		m_ptDown = point;
	}
	else if (PtInRect(m_rcCaptureArea, point))
	{
		m_emPosType = SelectRect;
        m_ptDown = point - m_rcCaptureArea.TopLeft();
	}
}

void SSnapshotCtrl::OnLButtonUp(UINT nFlags, SOUI::CPoint point)
{
	SetMsgHandled(FALSE);
	ReleaseCapture();
	SOUI::CRect rcWnd = GetWindowRect();

	m_bCapture = true;
	switch (m_emPosType)
	{
	case Null:
		break;
	case TopLeft:
		break;
	case TopCenter:
		break;
	case TopRight:
		break;
	case RightCenter:
		break;
	case BottomRight:
		break;
	case BottomCenter:
		break;
	case BottomLeft:
		break;
	case LeftCenter:
		break;
	case SelectRect:
	{
		SOUI::CPoint ptPos = point - m_ptDown;			// ��� ����� ʱ  �� ƫ����  Ҳ���� �ƶ� �� ֵ 
		if (ptPos.x < rcWnd.left)
			ptPos.x = rcWnd.left;
		else if (ptPos.x > rcWnd.right - m_rcCaptureArea.Width())
			ptPos.x = rcWnd.right - m_rcCaptureArea.Width();
		if (ptPos.y < rcWnd.top)
			ptPos.y = rcWnd.top;
		else if (ptPos.y > rcWnd.bottom - m_rcCaptureArea.Height())
			ptPos.y = rcWnd.bottom - m_rcCaptureArea.Height();
		m_rcCaptureArea.MoveToXY(ptPos);
	}
	break;
	default:
		break;
	}

	m_emPosType = Null;
	Invalidate();
}

void SSnapshotCtrl::OnLButtonDblClk(UINT nFlags, SOUI::CPoint point)
{
	//
}

void SSnapshotCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	//
}

void SSnapshotCtrl::SetBmpResource(CBitmap* pBmp)
{
	m_pBitmap = pBmp;

	//m_vecBitmap.push_back(pBmp);
	Invalidate();
}

void SSnapshotCtrl::CalcPos()
{
    SOUI::CRect rcLine(m_rcCaptureArea);
    rcLine.InflateRect(1, 1);
    CAutoRefPtr<IPen> curPen, oldPen;

    SOUI::CPoint center = rcLine.CenterPoint();
    // ���� ����
    m_rcPos[TopLeft].SetRect(rcLine.left, rcLine.top, rcLine.left + 14, rcLine.top + 14);
    // ���� ����
    m_rcPos[TopCenter].SetRect(center.x - 2, rcLine.top, center.x + 2, rcLine.top + 4);
    // ���� ����
    m_rcPos[TopRight].SetRect(rcLine.right - 4, rcLine.top, rcLine.right, rcLine.top + 4);
    // ���� ����
    m_rcPos[RightCenter].SetRect(rcLine.right - 4, center.y - 2, rcLine.right, center.y + 2);
    // ���� ����
    m_rcPos[BottomRight].SetRect(rcLine.right - 4, rcLine.bottom - 4, rcLine.right, rcLine.bottom);
    // ���� ����
    m_rcPos[BottomCenter].SetRect(center.x - 2, rcLine.bottom - 4, center.x + 2, rcLine.bottom);
    // ���� ����
    m_rcPos[BottomLeft].SetRect(rcLine.left, rcLine.bottom - 4, rcLine.left + 4, rcLine.bottom);
    // ���� ����
    m_rcPos[LeftCenter].SetRect(rcLine.left, center.y - 2, rcLine.left + 4, center.y + 2);
}