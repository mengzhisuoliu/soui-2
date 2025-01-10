﻿#include "souistd.h"
#include "control/SHeaderCtrl.h"
#include "helper/SDragWnd.h"

SNSBEGIN
SHeaderCtrl::SHeaderCtrl(void)
    : m_bFixWidth(FALSE)
    , m_bItemSwapEnable(TRUE)
    , m_bSortHeader(TRUE)
    , m_pSkinItem(GETBUILTINSKIN(SKIN_SYS_HEADER))
    , m_pSkinSort(NULL)
    , m_dwHitTest((DWORD)-1)
    , m_bDragging(FALSE)
    , m_hDragImg(NULL)
    , m_nScale(100)
{
    m_bClipClient = TRUE;
    m_evtSet.addEvent(EVENTID(EventHeaderClick));
    m_evtSet.addEvent(EVENTID(EventHeaderItemChanged));
    m_evtSet.addEvent(EVENTID(EventHeaderItemChanging));
    m_evtSet.addEvent(EVENTID(EventHeaderItemSwap));
    m_evtSet.addEvent(EVENTID(EventHeaderRelayout));
}

SHeaderCtrl::~SHeaderCtrl(void)
{
}

int SHeaderCtrl::InsertItem(int iItem, LPCTSTR pszText, int nWidth, UINT fmt, LPARAM lParam, BOOL bDpiAware /*=FALSE*/, float fWeight /*=0.0f*/)
{
    SASSERT(pszText);
    SASSERT(nWidth >= 0);
    if (iItem == -1)
        iItem = (int)m_arrItems.GetCount();
    SHDITEMEX item;
    item.mask = SHDI_ALL;
    item.fmt = fmt;
    item.fWeight = fWeight;
    SLayoutSize szWid((float)nWidth, bDpiAware ? SLayoutSize::dp : SLayoutSize::px);
    item.cx = szWid.toPixelSize(GetScale());
    item.bDpiAware = bDpiAware;
    item.strText.SetOwner(this);
    item.strText.SetText(pszText, false);
    item.state = 0;
    item.iOrder = iItem;
    item.lParam = lParam;
    item.bVisible = TRUE;
    m_arrItems.InsertAt(iItem, item);
    //需要更新列的序号
    for (size_t i = 0; i < GetItemCount(); i++)
    {
        if (i == (size_t)iItem)
            continue;
        if (m_arrItems[i].iOrder >= iItem)
            m_arrItems[i].iOrder++;
    }
    EventHeaderRelayout e(this);
    FireEvent(e);

    Invalidate();
    return iItem;
}

BOOL SHeaderCtrl::GetItem(int iItem, SHDITEM *pItem) const
{
    if ((UINT)iItem >= m_arrItems.GetCount())
        return FALSE;
    if (pItem->mask & SHDI_TEXT && pItem->pszText && pItem->cchMaxText)
    {
        _tcscpy_s(pItem->pszText, pItem->cchMaxText, m_arrItems[iItem].strText.GetText(FALSE).c_str());
    }
    if (pItem->mask & SHDI_WIDTH)
    {
        pItem->cx = GetItemWidth(iItem);
    }
    if (pItem->mask & SHDI_LPARAM)
        pItem->lParam = m_arrItems[iItem].lParam;
    if (pItem->mask & SHDI_FORMAT)
        pItem->fmt = m_arrItems[iItem].fmt;
    if (pItem->mask & SHDI_ORDER)
        pItem->iOrder = m_arrItems[iItem].iOrder;
    if (pItem->mask & SHDI_VISIBLE)
        pItem->bVisible = m_arrItems[iItem].bVisible;
    if (pItem->mask & SHDI_WEIGHT)
        pItem->fWeight = m_arrItems[iItem].fWeight;
    return TRUE;
}

BOOL SHeaderCtrl::SetItem(int iItem, const SHDITEM *pItem)
{
    if ((UINT)iItem >= m_arrItems.GetCount())
        return FALSE;

    SHDITEMEX &item = m_arrItems[iItem];

    if (pItem->mask & SHDI_TEXT && pItem->pszText)
    {
        item.strText.SetText(pItem->pszText, false);
    }
    if (pItem->mask & SHDI_WIDTH)
        item.cx = pItem->cx;
    if (pItem->mask & SHDI_LPARAM)
        item.lParam = pItem->lParam;
    if (pItem->mask & SHDI_WEIGHT)
        item.fWeight = pItem->fWeight;
    if (pItem->mask & SHDI_FORMAT)
        item.fmt = pItem->fmt;
    if (pItem->mask & SHDI_VISIBLE)
        item.bVisible = pItem->bVisible;
    Invalidate();
    return TRUE;
}

void SHeaderCtrl::OnPaint(IRenderTarget *pRT)
{
    SPainter painter;
    BeforePaint(pRT, painter);
    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcItem(rcClient.left, rcClient.top, rcClient.left, rcClient.bottom);
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (!m_arrItems[i].bVisible)
            continue;
        rcItem.left = rcItem.right;
        rcItem.right = rcItem.left + GetItemWidth(i);
        DrawItem(pRT, rcItem, m_arrItems.GetData() + i);
        if (rcItem.right >= rcClient.right)
            break;
    }
    if (rcItem.right < rcClient.right)
    {
        rcItem.left = rcItem.right;
        rcItem.right = rcClient.right;
        if (m_pSkinItem)
            m_pSkinItem->DrawByState(pRT, rcItem, WndState_Disable);
    }
    AfterPaint(pRT, painter);
}

void SHeaderCtrl::DrawItem(IRenderTarget *pRT, CRect rcItem, const LPSHDITEMEX pItem)
{
    if (!pItem->bVisible)
        return;
    if (m_pSkinItem)
        m_pSkinItem->DrawByState(pRT, rcItem, pItem->state);
    UINT align = DT_SINGLELINE | DT_VCENTER;
    if (pItem->fmt & HDF_CENTER)
        align |= DT_CENTER;
    else if (pItem->fmt & HDF_RIGHT)
        align |= DT_RIGHT;

    BOOL bDrawSortFlag = (pItem->fmt & SORT_MASK) != 0 && m_pSkinSort;
    if (!bDrawSortFlag)
    {
        pRT->DrawText(pItem->strText.GetText(FALSE), pItem->strText.GetText(FALSE).GetLength(), rcItem, align);
        return;
    }

    CSize szSort = m_pSkinSort->GetSkinSize();
    rcItem.right -= szSort.cx;

    pRT->DrawText(pItem->strText.GetText(FALSE), pItem->strText.GetText(FALSE).GetLength(), rcItem, align);
    CPoint ptSort;
    ptSort.x = rcItem.right;
    ptSort.y = rcItem.top + (rcItem.Height() - szSort.cy) / 2;

    m_pSkinSort->DrawByIndex(pRT, CRect(ptSort, szSort), (pItem->fmt & HDF_SORTUP) ? 0 : 1);
}

BOOL SHeaderCtrl::DeleteItem(int iItem)
{
    if (iItem < 0 || (UINT)iItem >= m_arrItems.GetCount())
        return FALSE;

    int iOrder = m_arrItems[iItem].iOrder;
    m_arrItems.RemoveAt(iItem);
    //更新排序
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (m_arrItems[i].iOrder > iOrder)
            m_arrItems[i].iOrder--;
    }
    EventHeaderRelayout e(this);
    FireEvent(e);

    Invalidate();
    return TRUE;
}

void SHeaderCtrl::DeleteAllItems()
{
    m_arrItems.RemoveAll();
    EventHeaderRelayout e(this);
    FireEvent(e);
    Invalidate();
}

void SHeaderCtrl::OnDestroy()
{
    GetEventSet()->setMutedState(true);
    DeleteAllItems();
    GetEventSet()->setMutedState(false);
    SWindow::OnDestroy();
}

CRect SHeaderCtrl::GetItemRect(int iItem) const
{
    CRect rcClient;
    GetClientRect(&rcClient);
    if (!m_arrItems[iItem].bVisible)
        return CRect();
    CRect rcItem(rcClient.left, rcClient.top, rcClient.left, rcClient.bottom);
    for (int i = 0; i <= iItem && i < (int)m_arrItems.GetCount(); i++)
    {
        if (!m_arrItems[i].bVisible)
            continue;
        rcItem.left = rcItem.right;
        rcItem.right = rcItem.left + GetItemWidth(i);
    }
    return rcItem;
}

void SHeaderCtrl::GetItemRect(CTHIS_ int iItem, LPRECT prc) SCONST
{
    *prc = GetItemRect(iItem);
}

int SHeaderCtrl::GetOriItemIndex(int iOrder) const
{
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (m_arrItems[i].iOrder == iOrder)
            return i;
    }
    return -1;
}

void SHeaderCtrl::RedrawItem(int iItem)
{
    CRect rcItem = GetItemRect(iItem);
    IRenderTarget *pRT = GetRenderTarget(rcItem, GRT_PAINTBKGND);
    DrawItem(pRT, rcItem, m_arrItems.GetData() + iItem);
    ReleaseRenderTarget(pRT);
}

void SHeaderCtrl::OnLButtonDown(UINT nFlags, CPoint pt)
{
    SetCapture();
    m_ptClick = pt;

    m_dwHitTest = HitTest(pt);
    if (IsItemHover(m_dwHitTest))
    {
        if (m_bSortHeader)
        {
            m_arrItems[LOWORD(m_dwHitTest)].state = WndState_PushDown;
            RedrawItem(LOWORD(m_dwHitTest));
        }
    }
    else if (m_dwHitTest != -1)
    {
        m_nAdjItemOldWidth = GetItemWidth(LOWORD(m_dwHitTest));
    }
}

void SHeaderCtrl::OnLButtonUp(UINT nFlags, CPoint pt)
{
    if (IsItemHover(m_dwHitTest))
    {
        if (m_bDragging)
        { //拖动表头项
            if (m_bItemSwapEnable)
            {
                SDragWnd::EndDrag();
                DeleteObject(m_hDragImg);
                m_hDragImg = NULL;

                m_arrItems[LOWORD(m_dwHitTest)].state = WndState_Normal;

                if (m_dwDragTo != m_dwHitTest && IsItemHover(m_dwDragTo))
                {
                    SHDITEMEX t = m_arrItems[LOWORD(m_dwHitTest)];
                    m_arrItems.RemoveAt(LOWORD(m_dwHitTest));
                    int nPos = LOWORD(m_dwDragTo);
                    if (nPos > LOWORD(m_dwHitTest))
                        nPos--; //要考虑将自己移除的影响
                    m_arrItems.InsertAt(LOWORD(m_dwDragTo), t);
                    //发消息通知宿主表项位置发生变化
                    EventHeaderItemSwap evt(this);
                    evt.iOldIndex = LOWORD(m_dwHitTest);
                    evt.iNewIndex = nPos;
                    FireEvent(evt);

                    EventHeaderRelayout e(this);
                    FireEvent(e);
                }

                m_dwHitTest = HitTest(pt);
                m_dwDragTo = (DWORD)-1;
                Invalidate();
            }
        }
        else
        { //点击表头项
            if (m_bSortHeader)
            {
                m_arrItems[LOWORD(m_dwHitTest)].state = WndState_Hover;
                RedrawItem(LOWORD(m_dwHitTest));
                EventHeaderClick evt(this);
                evt.iItem = LOWORD(m_dwHitTest);
                FireEvent(evt);
            }
        }
    }
    else if (m_dwHitTest != -1)
    { //调整表头宽度，发送一个调整完成消息
        EventHeaderItemChanged evt(this);
        evt.iItem = LOWORD(m_dwHitTest);
        evt.nWidth = GetItemWidth(evt.iItem);
        FireEvent(evt);

        EventHeaderRelayout e(this);
        FireEvent(e);
    }
    m_bDragging = FALSE;
    ReleaseCapture();
}

void SHeaderCtrl::OnMouseMove(UINT nFlags, CPoint pt)
{
    if (m_bDragging || nFlags & MK_LBUTTON)
    {
        if (!m_bDragging)
        {
            if (IsItemHover(m_dwHitTest) && m_bItemSwapEnable)
            {
                m_dwDragTo = m_dwHitTest;
                CRect rcItem = GetItemRect(LOWORD(m_dwHitTest));
                DrawDraggingState(m_dwDragTo);
                m_hDragImg = CreateDragImage(LOWORD(m_dwHitTest));
                CPoint pt = m_ptClick - rcItem.TopLeft();
                SDragWnd::BeginDrag(m_hDragImg, pt, 0, 128, LWA_ALPHA | LWA_COLORKEY);
                m_bDragging = TRUE;
            }
        }
        if (IsItemHover(m_dwHitTest))
        {
            if (m_bItemSwapEnable)
            {
                DWORD dwDragTo = HitTest(pt);
                CPoint pt2(pt.x, m_ptClick.y);
                ClientToScreen(GetContainer()->GetHostHwnd(), &pt2);
                if (IsItemHover(dwDragTo) && m_dwDragTo != dwDragTo)
                {
                    m_dwDragTo = dwDragTo;
                    DrawDraggingState(dwDragTo);
                }
                SDragWnd::DragMove(pt2);
            }
        }
        else if (m_dwHitTest != -1)
        { //调节宽度
            if (!m_bFixWidth)
            {
                int iItem = LOWORD(m_dwHitTest);
                int cxNew = m_nAdjItemOldWidth + pt.x - m_ptClick.x;
                if (cxNew < 0)
                    cxNew = 0;
                CRect rc = GetClientRect();
                int nTotalWid = 0;
                float fTotalWeight = 0.0f;
                for (UINT i = 0; i < m_arrItems.GetCount(); i++)
                {
                    if (!m_arrItems[i].bVisible)
                        continue;
                    nTotalWid += m_arrItems[i].cx;
                    fTotalWeight += m_arrItems[i].fWeight;
                }
                if (fTotalWeight > 0.0f)
                {
                    if (nTotalWid != rc.Width())
                    { // first adjust, split the remain size into columns based on column weight.
                        int nRemain = rc.Width() - nTotalWid;
                        for (UINT i = 0; i < m_arrItems.GetCount() && nRemain > 0 && fTotalWeight > 0.0f; i++)
                        {
                            if (!m_arrItems[i].bVisible)
                                continue;
                            int nAppend = (int)(nRemain * m_arrItems[i].fWeight / fTotalWeight);
                            m_arrItems[i].cx += nAppend;
                            nRemain -= nAppend;
                            fTotalWeight -= m_arrItems[i].fWeight;
                        }
                    }
                    if (iItem == m_arrItems.GetCount() - 1) // can't adjust last column
                        return;
                    int nDelta = cxNew - m_arrItems[iItem].cx;
                    if (m_arrItems[iItem].cx + nDelta < 0)
                        nDelta = -m_arrItems[iItem].cx;
                    if (m_arrItems[iItem + 1].cx - nDelta < 0)
                        nDelta = m_arrItems[iItem + 1].cx;
                    m_arrItems[iItem].cx += nDelta;     // add the delta the the select column.
                    m_arrItems[iItem + 1].cx -= nDelta; // sub the delta from the next column.
                }
                else
                { // no weight data
                    m_arrItems[iItem].cx = cxNew;
                }

                Invalidate();
                //发出调节宽度消息
                EventHeaderItemChanging evt(this);
                evt.iItem = iItem;
                evt.nWidth = cxNew;
                FireEvent(evt);

                EventHeaderRelayout e(this);
                FireEvent(e);
            }
        }
    }
    else
    {
        DWORD dwHitTest = HitTest(pt);
        if (dwHitTest != m_dwHitTest)
        {
            if (m_bSortHeader)
            {
                if (IsItemHover(m_dwHitTest))
                {
                    WORD iHover = LOWORD(m_dwHitTest);
                    m_arrItems[iHover].state = WndState_Normal;
                    RedrawItem(iHover);
                }
                if (IsItemHover(dwHitTest))
                {
                    WORD iHover = LOWORD(dwHitTest);
                    m_arrItems[iHover].state = WndState_Hover;
                    RedrawItem(iHover);
                }
            }

            m_dwHitTest = dwHitTest;
        }
    }
}

void SHeaderCtrl::OnMouseLeave()
{
    if (!m_bDragging)
    {
        if (IsItemHover(m_dwHitTest))
        {
            if (m_bSortHeader)
            {
                m_arrItems[LOWORD(m_dwHitTest)].state = WndState_Normal;
                RedrawItem(LOWORD(m_dwHitTest));
            }
            m_dwHitTest = (DWORD)-1;
        }
    }
}

BOOL SHeaderCtrl::CreateChildren(SXmlNode xmlNode)
{
    SXmlNode xmlItems = xmlNode.child(L"items");
    if (xmlItems)
        xmlItems.set_userdata(1);
    __baseCls::CreateChildren(xmlNode);

    if (!xmlItems)
        return FALSE;
    SXmlNode xmlItem = xmlItems.child(L"item");
    int iOrder = 0;
    while (xmlItem)
    {
        SHDITEMEX item;
        item.strText.SetOwner(this);
        item.mask = 0xFFFFFFFF;
        item.iOrder = iOrder++;
        SStringW strText = xmlItem.attribute(L"text").as_string();
        if (strText.IsEmpty())
        {
            strText = SWindow::GetXmlText(xmlItem);
        }
        item.strText.SetText(S_CW2T(GETSTRING(strText)));
        SLayoutSize szItem = GETLAYOUTSIZE(xmlItem.attribute(L"width").as_string(L"50"));
        item.cx = szItem.toPixelSize(GetScale());
        item.fWeight = xmlItem.attribute(L"weight").as_float();
        item.bDpiAware = (szItem.unit != SLayoutSize::px);
        item.lParam = xmlItem.attribute(L"userData").as_uint(0);
        item.bVisible = xmlItem.attribute(L"visible").as_bool(true);
        item.fmt = 0;
        item.state = 0;
        SStringW strSort = xmlItem.attribute(L"sortFlag").as_string();
        strSort.MakeLower();
        if (strSort == L"down")
            item.fmt |= HDF_SORTDOWN;
        else if (strSort == L"up")
            item.fmt |= HDF_SORTUP;
        SStringW strAlign = xmlItem.attribute(L"align").as_string();
        strAlign.MakeLower();
        if (strAlign == L"left")
            item.fmt |= HDF_LEFT;
        else if (strAlign == L"center")
            item.fmt |= HDF_CENTER;
        else if (strAlign == L"right")
            item.fmt |= HDF_RIGHT;
        else
        {
            int align = GetStyle().GetTextAlign();
            if (align & DT_CENTER)
                item.fmt |= HDF_CENTER;
            else if (align & DT_RIGHT)
                item.fmt |= HDF_RIGHT;
            else
                item.fmt |= HDF_LEFT;
        }
        m_arrItems.InsertAt(m_arrItems.GetCount(), item);

        xmlItem = xmlItem.next_sibling(L"item");
    }

    return TRUE;
}

BOOL SHeaderCtrl::OnSetCursor(const CPoint &pt)
{
    if (m_bFixWidth)
        return FALSE;
    DWORD dwHit = HitTest(pt);
    if (HIWORD(dwHit) == LOWORD(dwHit))
        return FALSE;
    HCURSOR hCursor = GETRESPROVIDER->LoadCursor(IDC_SIZEWE);
    SetCursor(hCursor);
    return TRUE;
}

DWORD SHeaderCtrl::HitTest(CPoint pt)
{
    CRect rcClient;
    GetClientRect(&rcClient);
    if (!rcClient.PtInRect(pt))
        return (DWORD)-1;

    CRect rcItem(rcClient.left, rcClient.top, rcClient.left, rcClient.bottom);
    int nMargin = m_bFixWidth ? MARGIN_ADJUST_DISABLE : MARGIN_ADJUST_ENABLE;
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (m_arrItems[i].cx == 0 || !m_arrItems[i].bVisible)
            continue; //越过宽度为0的项

        rcItem.left = rcItem.right;
        rcItem.right = rcItem.left + GetItemWidth(i);
        if (pt.x < rcItem.left + nMargin)
        {
            int nLeft = i > 0 ? i - 1 : 0;
            return MAKELONG(nLeft, i);
        }
        else if (pt.x < rcItem.right - nMargin)
        {
            return MAKELONG(i, i);
        }
        else if (pt.x < rcItem.right)
        {
            WORD nRight = (WORD)i + 1;
            if (nRight >= m_arrItems.GetCount())
                nRight = (WORD)-1; //采用-1代表末尾
            return MAKELONG(i, nRight);
        }
    }
    return (DWORD)-1;
}

HBITMAP SHeaderCtrl::CreateDragImage(UINT iItem)
{
    if (iItem >= m_arrItems.GetCount())
        return NULL;
    CRect rcClient;
    GetClientRect(rcClient);
    CRect rcItem(0, 0, GetItemWidth(iItem), rcClient.Height());

    SAutoRefPtr<IRenderTarget> pRT;
    GETRENDERFACTORY->CreateRenderTarget(&pRT, rcItem.Width(), rcItem.Height());
    pRT->BeginDraw();
    BeforePaintEx(pRT);
    DrawItem(pRT, rcItem, m_arrItems.GetData() + iItem);
    pRT->EndDraw();

    HBITMAP hBmp = CreateBitmap(rcItem.Width(), rcItem.Height(), 1, 32, NULL);
    HDC hdc = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hdc);
    ::SelectObject(hMemDC, hBmp);
    HDC hdcSrc = pRT->GetDC(0);
    ::BitBlt(hMemDC, 0, 0, rcItem.Width(), rcItem.Height(), hdcSrc, 0, 0, SRCCOPY);
    pRT->ReleaseDC(hdcSrc, NULL);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hdc);
    return hBmp;
}

void SHeaderCtrl::DrawDraggingState(DWORD dwDragTo)
{
    CRect rcClient;
    GetClientRect(&rcClient);
    IRenderTarget *pRT = GetRenderTarget(rcClient, GRT_PAINTBKGND);
    SPainter painter;
    BeforePaint(pRT, painter);
    CRect rcItem(rcClient.left, rcClient.top, rcClient.left, rcClient.bottom);
    int iDragTo = LOWORD(dwDragTo);
    int iDragFrom = LOWORD(m_dwHitTest);

    SArray<UINT> items;
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (i != (UINT)iDragFrom)
            items.Add(i);
    }
    items.InsertAt(iDragTo, iDragFrom);

    if (m_pSkinItem)
        m_pSkinItem->DrawByIndex(pRT, rcClient, 0);
    for (UINT i = 0; i < items.GetCount(); i++)
    {
        rcItem.left = rcItem.right;
        rcItem.right = rcItem.left + GetItemWidth(items[i]);
        if (items[i] != (UINT)iDragFrom)
            DrawItem(pRT, rcItem, m_arrItems.GetData() + items[i]);
    }
    AfterPaint(pRT, painter);
    ReleaseRenderTarget(pRT);
}

int SHeaderCtrl::GetTotalWidth(BOOL bMinWid) const
{
    CRect rc = GetClientRect();

    int nTotalWidth = 0;
    float fTotalWeight = 0.0f;
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (!m_arrItems[i].bVisible)
            continue;
        nTotalWidth += m_arrItems[i].cx;
        fTotalWeight += m_arrItems[i].fWeight;
    }
    if (!bMinWid && fTotalWeight > 0.0f)
    {
        return smax(nTotalWidth, rc.Width());
    }
    else
    {
        return nTotalWidth;
    }
}

int SHeaderCtrl::GetItemWidth(int iItem) const
{
    if (iItem < 0 || (UINT)iItem >= m_arrItems.GetCount())
        return -1;
    if (!m_arrItems[iItem].bVisible)
        return 0;
    const SHDITEM &item = m_arrItems[iItem];
    if (SLayoutSize::fequal(item.fWeight, 0.0f))
        return item.cx;
    else
    {
        CRect rc = GetClientRect();
        if (rc.IsRectEmpty())
        {
            return item.cx;
        }
        else
        {
            float fTotalWeight = 0.0f;
            int nTotalWidth = 0;
            for (UINT i = 0; i < m_arrItems.GetCount(); i++)
            {
                if (!m_arrItems[i].bVisible)
                    continue;
                fTotalWeight += m_arrItems[i].fWeight;
                nTotalWidth += m_arrItems[i].cx;
            }
            int nRemain = rc.Width() - nTotalWidth;
            if (nRemain <= 0)
            {
                return m_arrItems[iItem].cx;
            }
            for (int i = 0; i < iItem; i++)
            {
                if (!m_arrItems[i].bVisible)
                    continue;
                int nAppend = (int)(nRemain * m_arrItems[i].fWeight / fTotalWeight);
                nRemain -= nAppend;
                fTotalWeight -= m_arrItems[i].fWeight;
            }
            return item.cx + (int)(nRemain * item.fWeight / fTotalWeight);
        }
    }
}

void SHeaderCtrl::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    if (m_bDragging)
    {
        if (m_bSortHeader && m_dwHitTest != -1)
        {
            m_arrItems[LOWORD(m_dwHitTest)].state = WndState_Normal;
        }
        m_dwHitTest = (DWORD)-1;

        SDragWnd::EndDrag();
        DeleteObject(m_hDragImg);
        m_hDragImg = NULL;
        m_bDragging = FALSE;
        ReleaseCapture();
        Invalidate();
    }
}

void SHeaderCtrl::SetItemSort(int iItem, UINT sortFlag)
{
    SASSERT(iItem >= 0 && iItem < (int)m_arrItems.GetCount());

    if ((sortFlag & SORT_MASK) != (m_arrItems[iItem].fmt & SORT_MASK))
    {
        m_arrItems[iItem].fmt &= ~SORT_MASK;
        m_arrItems[iItem].fmt |= sortFlag & SORT_MASK;
        CRect rcItem = GetItemRect(iItem);
        InvalidateRect(rcItem);
    }
}

void SHeaderCtrl::OnColorize(COLORREF cr)
{
    __baseCls::OnColorize(cr);
    if (m_pSkinItem)
        m_pSkinItem->OnColorize(cr);
    if (m_pSkinSort)
        m_pSkinSort->OnColorize(cr);
}

HRESULT SHeaderCtrl::OnLanguageChanged()
{
    __baseCls::OnLanguageChanged();
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        m_arrItems[i].strText.TranslateText();
    }
    return S_OK;
}

void SHeaderCtrl::OnScaleChanged(int nScale)
{
    SWindow::OnScaleChanged(nScale);
    if (nScale != m_nScale)
    {
        for (size_t i = 0; i < m_arrItems.GetCount(); i++)
        {
            if (!m_arrItems[i].bDpiAware)
                continue;
            m_arrItems[i].cx = m_arrItems[i].cx * nScale / m_nScale;
        }
        m_nScale = nScale;
    }
}

BOOL SHeaderCtrl::OnRelayout(const CRect &rcWnd)
{
    BOOL bRet = __baseCls::OnRelayout(rcWnd);
    if (bRet)
    {
        EventHeaderRelayout e(this);
        FireEvent(e);
    }
    return bRet;
}

void SHeaderCtrl::SetItemVisible(int iItem, BOOL visible)
{
    SASSERT(iItem >= 0 && iItem < (int)m_arrItems.GetCount());
    m_arrItems[iItem].bVisible = visible;

    Invalidate();
    //发出调节宽度消息
    EventHeaderItemChanged evt(this);
    evt.iItem = iItem;
    evt.nWidth = GetItemWidth(iItem);
    FireEvent(evt);
}

BOOL SHeaderCtrl::IsItemVisible(int iItem) const
{
    SASSERT(iItem >= 0 && iItem < (int)m_arrItems.GetCount());
    return m_arrItems[iItem].bVisible;
}

BOOL SHeaderCtrl::IsAutoResize() const
{
    for (UINT i = 0; i < m_arrItems.GetCount(); i++)
    {
        if (m_arrItems[i].fWeight > 0.0f)
            return TRUE;
    }
    return FALSE;
}

UINT SHeaderCtrl::GetItemCount() const
{
    return (UINT)m_arrItems.GetCount();
}

SNSEND