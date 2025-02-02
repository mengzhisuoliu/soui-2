﻿//////////////////////////////////////////////////////////////////////////
//  Class Name: SListBox
// Description: A DuiWindow Based ListBox Control.
//     Creator: JinHui
//     Version: 2012.12.18 - 1.0 - Create
//////////////////////////////////////////////////////////////////////////
#include "souistd.h"
#include <control/SListbox.h>

#pragma warning(disable : 4018)
#pragma warning(disable : 4267)

SNSBEGIN

SListBox::SListBox()
    : m_itemHeight(20.f, SLayoutSize::dp)
    , m_iSelItem(-1)
    , m_iHoverItem(-1)
    , m_crItemBg(CR_INVALID)
    , m_crItemBg2(CR_INVALID)
    , m_crItemSelBg(RGBA(57, 145, 209, 255))
    , m_crItemHotBg(RGBA(57, 145, 209, 128))
    , m_crText(CR_INVALID)
    , m_crSelText(CR_INVALID)
    , m_pItemSkin(NULL)
    , m_pIconSkin(NULL)
    , m_bHotTrack(FALSE)
{
    m_bFocusable = TRUE;
    m_bClipClient = TRUE;
    m_ptIcon[0].fSize = m_ptIcon[1].fSize = SIZE_UNDEF;
    m_ptText[0].fSize = m_ptText[1].fSize = SIZE_UNDEF;
    m_evtSet.addEvent(EVENTID(EventLBSelChanging));
    m_evtSet.addEvent(EVENTID(EventLBSelChanged));
    m_evtSet.addEvent(EVENTID(EventLBDbClick));
}

SListBox::~SListBox()
{
}

int SListBox::GetCount() const
{
    return m_arrItems.GetCount();
}

int SListBox::GetCurSel() const
{
    return m_iSelItem;
}

BOOL SListBox::SetCurSel(int nIndex, BOOL bNotifyChange)
{
    if (nIndex >= GetCount())
        return FALSE;

    if (nIndex < 0)
        nIndex = -1;

    if (m_iSelItem == nIndex)
        return 0;
    int nOldSelItem = m_iSelItem;
    m_iSelItem = nIndex;

    if (IsVisible(TRUE))
    {
        if (nOldSelItem != -1)
            RedrawItem(nOldSelItem);
        if (m_iSelItem != -1)
            RedrawItem(m_iSelItem);
    }
    if (bNotifyChange)
    {
        NotifySelChange(nOldSelItem, nIndex);
    }
    return TRUE;
}

int SListBox::GetTopIndex() const
{
    return m_siVer.nPos / m_itemHeight.toPixelSize(GetScale());
}

BOOL SListBox::SetTopIndex(int nIndex)
{
    if (nIndex < 0 || nIndex >= GetCount())
        return FALSE;

    OnScroll(TRUE, SB_THUMBPOSITION, nIndex * m_itemHeight.toPixelSize(GetScale()));
    return TRUE;
}

LPARAM SListBox::GetItemData(int nIndex) const
{
    if (nIndex < 0 || nIndex >= GetCount())
        return 0;

    return m_arrItems[nIndex]->lParam;
}

BOOL SListBox::SetItemData(int nIndex, LPARAM lParam)
{
    if (nIndex < 0 || nIndex >= GetCount())
        return FALSE;

    m_arrItems[nIndex]->lParam = lParam;
    return TRUE;
}

BOOL SListBox::SetItemImage(THIS_ int nIndex, int iImage)
{
    if (nIndex < 0 || nIndex >= GetCount())
        return FALSE;

    m_arrItems[nIndex]->nImage = iImage;
    return TRUE;
}

int SListBox::GetItemImage(THIS_ int nIndex)
{
    if (nIndex < 0 || nIndex >= GetCount())
        return -1;

    return m_arrItems[nIndex]->nImage;
}

BOOL SListBox::GetIText(int nIndex, BOOL bRawText, IStringT *str) const
{
    if (nIndex < 0 || nIndex >= GetCount())
        return FALSE;

    SStringT strRet = m_arrItems[nIndex]->strText.GetText(bRawText);
    str->Copy(&strRet);
    return TRUE;
}

int SListBox::GetItemHeight() const
{
    return m_itemHeight.toPixelSize(GetScale());
}

void SListBox::SetItemHeight(int cyItemHeight)
{
    m_itemHeight = SLayoutSize((float)cyItemHeight, SLayoutSize::px);
    UpdateScrollBar();
}

void SListBox::DeleteAll()
{
    for (int i = 0; i < GetCount(); i++)
    {
        if (m_arrItems[i])
            delete m_arrItems[i];
    }
    m_arrItems.RemoveAll();

    m_iSelItem = -1;
    m_iHoverItem = -1;

    Invalidate();
    UpdateScrollBar();
}

BOOL SListBox::DeleteString(int nIndex)
{
    if (nIndex < 0 || nIndex >= GetCount())
        return FALSE;

    if (m_arrItems[nIndex])
        delete m_arrItems[nIndex];
    m_arrItems.RemoveAt(nIndex);

    if (m_iSelItem == nIndex)
        m_iSelItem = -1;
    else if (m_iSelItem > nIndex)
        m_iSelItem--;
    if (m_iHoverItem == nIndex)
        m_iHoverItem = -1;
    else if (m_iHoverItem > nIndex)
        m_iHoverItem--;

    UpdateScrollBar();

    return TRUE;
}

int SListBox::AddString(LPCTSTR lpszItem, int nImage, LPARAM lParam)
{
    return InsertString(-1, lpszItem, nImage, lParam);
}

int SListBox::InsertString(int nIndex, LPCTSTR lpszItem, int nImage, LPARAM lParam)
{
    // SASSERT(lpszItem);

    LPLBITEM pItem = new LBITEM(this);
    pItem->strText.SetText(lpszItem, false);
    pItem->nImage = nImage;
    pItem->lParam = lParam;

    return InsertItem(nIndex, pItem);
}

void SListBox::EnsureVisible(int nIndex)
{
    if (nIndex < 0 || nIndex >= GetCount())
        return;

    CRect rcClient;
    GetClientRect(&rcClient);

    int nItemHei = m_itemHeight.toPixelSize(GetScale());
    int iFirstVisible = (m_siVer.nPos + nItemHei - 1) / nItemHei;
    int nVisibleItems = rcClient.Height() / nItemHei;
    if (nIndex < iFirstVisible || nIndex > iFirstVisible + nVisibleItems - 1)
    {
        int nOffset = GetScrollPos(TRUE);
        if (nIndex < iFirstVisible)
            nOffset = (nIndex - iFirstVisible) * nItemHei;
        else
            nOffset = (nIndex - iFirstVisible - nVisibleItems + 1) * nItemHei;
        nOffset -= nOffset % nItemHei; //让当前行刚好显示
        OnScroll(TRUE, SB_THUMBPOSITION, nOffset + GetScrollPos(TRUE));
    }
}

//自动修改pt的位置为相对当前项的偏移量
int SListBox::HitTest(CPoint &pt)
{
    CRect rcClient;
    GetClientRect(&rcClient);
    if (!rcClient.PtInRect(pt))
        return -1;

    CPoint pt2 = pt;
    pt2.y -= rcClient.top - m_siVer.nPos;
    int nItemHei = m_itemHeight.toPixelSize(GetScale());
    int nRet = pt2.y / nItemHei;
    if (nRet >= GetCount())
        nRet = -1;
    else
    {
        pt.x -= rcClient.left;
        pt.y = pt2.y % nItemHei;
    }

    return nRet;
}

int SListBox::FindString(int iFindAfter, LPCTSTR pszText) const
{
    if (iFindAfter < 0)
        iFindAfter = -1;
    int iStart = iFindAfter + 1;
    for (int i = 0; i < m_arrItems.GetCount(); i++)
    {
        int iTarget = (i + iStart) % m_arrItems.GetCount();
        if (m_arrItems[iTarget]->strText.GetText(TRUE).StartsWith(pszText))
            return iTarget;
    }
    return -1;
}

BOOL SListBox::CreateChildren(SXmlNode xmlNode)
{
    if (!xmlNode)
        return TRUE;

    SXmlNode xmlItems = xmlNode.child(L"items");
    if (xmlItems)
    {
        SXmlNode xmlItem = xmlItems.child(L"item");
        while (xmlItem)
        {
            LPLBITEM pItemObj = new LBITEM(this);
            LoadItemAttribute(xmlItem, pItemObj);
            InsertItem(-1, pItemObj);
            xmlItem = xmlItem.next_sibling();
        }
    }

    int nSelItem = xmlNode.attribute(L"curSel").as_int(-1);
    SetCurSel(nSelItem);

    return TRUE;
}

void SListBox::LoadItemAttribute(SXmlNode xmlNode, LPLBITEM pItem)
{
    pItem->nImage = xmlNode.attribute(L"icon").as_int(pItem->nImage);
    pItem->lParam = xmlNode.attribute(L"data").as_uint((UINT)pItem->lParam);
    SStringW strText = GETSTRING(xmlNode.attribute(L"text").value());
    if (strText.IsEmpty())
        strText = GetXmlText(xmlNode);
    pItem->strText.SetText(S_CW2T(GETSTRING(strText)));
}

int SListBox::InsertItem(int nIndex, LPLBITEM pItem)
{
    SASSERT(pItem);

    if (nIndex == -1 || nIndex > GetCount())
    {
        nIndex = GetCount();
    }

    m_arrItems.InsertAt(nIndex, pItem);

    if (m_iSelItem >= nIndex)
        m_iSelItem++;
    if (m_iHoverItem >= nIndex)
        m_iHoverItem++;

    UpdateScrollBar();

    return nIndex;
}

void SListBox::RedrawItem(int iItem)
{
    if (!IsVisible(TRUE))
        return;

    CRect rcClient;
    GetClientRect(&rcClient);
    int iFirstVisible = GetTopIndex();
    int nItemHei = m_itemHeight.toPixelSize(GetScale());
    int nPageItems = (rcClient.Height() + nItemHei - 1) / nItemHei + 1;

    if (iItem >= iFirstVisible && iItem < GetCount() && iItem < iFirstVisible + nPageItems)
    {
        CRect rcItem(0, 0, rcClient.Width(), nItemHei);
        rcItem.OffsetRect(0, nItemHei * iItem - m_siVer.nPos);
        rcItem.OffsetRect(rcClient.TopLeft());
        InvalidateRect(rcItem);
    }
}

void SListBox::DrawItem(IRenderTarget *pRT, CRect &rc, int iItem)
{
    if (iItem < 0 || iItem >= GetCount())
        return;

    BOOL bTextColorChanged = FALSE;
    int nBgImg = 0;
    COLORREF crOldText = RGBA(0xFF, 0xFF, 0xFF, 0xFF);
    COLORREF crItemBg = m_crItemBg;
    COLORREF crText = m_crText;
    LPLBITEM pItem = m_arrItems[iItem];
    CRect rcIcon, rcText;

    if (iItem % 2)
    {
        if (CR_INVALID != m_crItemBg2)
            crItemBg = m_crItemBg2;
    }

    if (iItem == m_iSelItem)
    { //和下面那个if的条件分开，才会有sel和hot的区别
        if (m_pItemSkin != NULL)
            nBgImg = 2;
        else if (CR_INVALID != m_crItemSelBg)
            crItemBg = m_crItemSelBg;

        if (CR_INVALID != m_crSelText)
            crText = m_crSelText;
    }
    else if ((iItem == m_iHoverItem || (m_iHoverItem == -1 && iItem == m_iSelItem)) && m_bHotTrack)
    {
        if (m_pItemSkin != NULL)
            nBgImg = 1;
        else if (CR_INVALID != m_crItemHotBg)
            crItemBg = m_crItemHotBg;

        if (CR_INVALID != m_crSelText)
            crText = m_crSelText;
    }

    if (CR_INVALID != crItemBg) //先画背景
        pRT->FillSolidRect(rc, crItemBg);

    if (m_pItemSkin != NULL) //有skin，则覆盖背景
        m_pItemSkin->DrawByIndex(pRT, rc, nBgImg);

    if (CR_INVALID != crText)
    {
        bTextColorChanged = TRUE;
        crOldText = pRT->SetTextColor(crText);
    }

    int nItemHei = m_itemHeight.toPixelSize(GetScale());
    if (pItem->nImage != -1 && m_pIconSkin)
    {
        int nOffsetX = m_ptIcon[0].toPixelSize(GetScale()), nOffsetY = m_ptIcon[1].toPixelSize(GetScale());
        CSize sizeSkin = m_pIconSkin->GetSkinSize();
        rcIcon.SetRect(0, 0, sizeSkin.cx, sizeSkin.cy);

        if (!m_ptIcon[0].isValid())
            nOffsetX = nItemHei / 6;

        if (!m_ptIcon[1].isValid())
            nOffsetY = (nItemHei - sizeSkin.cy) / 2; // y 默认居中

        rcIcon.OffsetRect(rc.left + nOffsetX, rc.top + nOffsetY);
        m_pIconSkin->DrawByIndex(pRT, rcIcon, pItem->nImage);
    }

    UINT align = DT_SINGLELINE;
    rcText = rc;

    if (!m_ptText[0].isValid())
        rcText.left = rcIcon.Width() > 0 ? rcIcon.right + nItemHei / 6 : rc.left;
    else
        rcText.left = rc.left + m_ptText[0].toPixelSize(GetScale());

    if (!m_ptText[1].isValid())
        align |= DT_VCENTER;
    else
        rcText.top = rc.top + m_ptText[1].toPixelSize(GetScale());

    pRT->DrawText(pItem->strText.GetText(FALSE), -1, rcText, align);

    if (bTextColorChanged)
        pRT->SetTextColor(crOldText);
}

void SListBox::NotifySelChange(int nOldSel, int nNewSel)
{
    EventLBSelChanging evt1(this);
    evt1.bCancel = FALSE;
    evt1.nOldSel = nOldSel;
    evt1.nNewSel = nNewSel;

    FireEvent(evt1);
    if (evt1.bCancel)
        return;

    m_iSelItem = nNewSel;
    if (nOldSel != -1)
        RedrawItem(nOldSel);

    if (m_iSelItem != -1)
        RedrawItem(m_iSelItem);

    EventLBSelChanged evt2(this);
    evt2.nOldSel = nOldSel;
    evt2.nNewSel = nNewSel;
    FireEvent(evt2);
}

void SListBox::OnPaint(IRenderTarget *pRT)
{
    SPainter painter;
    BeforePaint(pRT, painter);

    int nItemHei = m_itemHeight.toPixelSize(GetScale());
    int iFirstVisible = GetTopIndex();
    int nPageItems = (m_rcClient.Height() + nItemHei - 1) / nItemHei + 1;

    for (int iItem = iFirstVisible; iItem < GetCount() && iItem < iFirstVisible + nPageItems; iItem++)
    {
        CRect rcItem(0, 0, m_rcClient.Width(), nItemHei);
        rcItem.OffsetRect(0, nItemHei * iItem - m_siVer.nPos);
        rcItem.OffsetRect(m_rcClient.TopLeft());
        DrawItem(pRT, rcItem, iItem);
    }

    AfterPaint(pRT, painter);
}

void SListBox::OnSize(UINT nType, CSize size)
{
    __baseCls::OnSize(nType, size);
    UpdateScrollBar();
}

void SListBox::OnLButtonDown(UINT nFlags, CPoint pt)
{
    __baseCls::OnLButtonDown(nFlags, pt);
    if (!m_bHotTrack)
    {
        m_iHoverItem = HitTest(pt);
        if (m_iHoverItem != m_iSelItem)
            NotifySelChange(m_iSelItem, m_iHoverItem);
    }
}

void SListBox::OnLButtonUp(UINT nFlags, CPoint pt)
{
    if (m_bHotTrack)
    {
        CPoint pt2(pt);
        m_iHoverItem = HitTest(pt2);
        if (m_iHoverItem != m_iSelItem)
            NotifySelChange(m_iSelItem, m_iHoverItem);
    }
    __baseCls::OnLButtonUp(nFlags, pt);
}

void SListBox::OnLButtonDbClick(UINT nFlags, CPoint pt)
{
    __baseCls::OnLButtonDbClick(nFlags, pt);
    m_iHoverItem = HitTest(pt);
    if (m_iHoverItem != m_iSelItem)
        NotifySelChange(m_iSelItem, m_iHoverItem);

    EventLBDbClick evt2(this);
    evt2.nCurSel = m_iHoverItem;
    FireEvent(evt2);
}

void SListBox::OnMouseMove(UINT nFlags, CPoint pt)
{
    int nOldHover = m_iHoverItem;
    m_iHoverItem = HitTest(pt);

    if (m_bHotTrack && nOldHover != m_iHoverItem)
    {
        if (nOldHover != -1)
            RedrawItem(nOldHover);
        if (m_iHoverItem != -1)
            RedrawItem(m_iHoverItem);
        if (m_iSelItem != -1)
            RedrawItem(m_iSelItem);
    }
}

void SListBox::OnKeyDown(TCHAR nChar, UINT nRepCnt, UINT nFlags)
{
    int nNewSelItem = -1;
    int iCurSel = m_iSelItem;
    if (m_bHotTrack && m_iHoverItem != -1)
        iCurSel = m_iHoverItem;
    if (nChar == VK_DOWN && m_iSelItem < GetCount() - 1)
        nNewSelItem = iCurSel + 1;
    else if (nChar == VK_UP && m_iSelItem > 0)
        nNewSelItem = iCurSel - 1;

    if (nNewSelItem != -1)
    {
        int iHover = m_iHoverItem;
        if (m_bHotTrack)
            m_iHoverItem = -1;
        EnsureVisible(nNewSelItem);
        NotifySelChange(m_iSelItem, nNewSelItem);
        if (iHover != -1 && iHover != m_iSelItem && iHover != nNewSelItem)
            RedrawItem(iHover);
    }
}

void SListBox::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    SWindow *pOwner = GetOwner();
    if (pOwner)
        pOwner->SSendMessage(WM_CHAR, nChar, MAKELONG(nFlags, nRepCnt));
}

UINT SListBox::OnGetDlgCode() const
{
    return SC_WANTARROWS | SC_WANTCHARS;
}

void SListBox::OnDestroy()
{
    DeleteAll();
    __baseCls::OnDestroy();
}

void SListBox::OnShowWindow(BOOL bShow, UINT nStatus)
{
    if (!bShow)
    {
        m_iHoverItem = -1;
    }
    __baseCls::OnShowWindow(bShow, nStatus);
}

void SListBox::OnMouseLeave()
{
    __baseCls::OnMouseLeave();
    if (m_iHoverItem != -1)
    {
        int nOldHover = m_iHoverItem;
        m_iHoverItem = -1;
        RedrawItem(nOldHover);
    }
}

HRESULT SListBox::OnLanguageChanged()
{
    HRESULT hr = __baseCls::OnLanguageChanged();
    for (size_t i = 0; i < m_arrItems.GetCount(); i++)
    {
        m_arrItems[i]->strText.TranslateText();
    }
    Invalidate();
    return hr;
}

void SListBox::OnScaleChanged(int nScale)
{
    __baseCls::OnScaleChanged(nScale);
    GetScaleSkin(m_pItemSkin, nScale);
    GetScaleSkin(m_pIconSkin, nScale);
}

void SListBox::UpdateScrollBar()
{
    CRect rcClient = SWindow::GetClientRect();
    CSize size = rcClient.Size();
    CSize szView;
    szView.cx = rcClient.Width();
    szView.cy = GetCount() * GetItemHeight();

    //  关闭滚动条
    m_wBarVisible = SSB_NULL;

    if (size.cy < szView.cy)
    {
        //  需要纵向滚动条
        m_wBarVisible |= SSB_VERT;
        m_siVer.nMin = 0;
        m_siVer.nMax = szView.cy - 1;
        m_siVer.nPage = size.cy;
        m_siVer.nPos = smin(m_siVer.nPos, m_siVer.nMax - (int)m_siVer.nPage);
    }
    else
    {
        //  不需要纵向滚动条
        m_siVer.nPage = size.cy;
        m_siVer.nMin = 0;
        m_siVer.nMax = size.cy - 1;
        m_siVer.nPos = 0;
    }

    SetScrollPos(TRUE, m_siVer.nPos, FALSE);

    //  重新计算客户区及非客户区
    SSendMessage(WM_NCCALCSIZE);

    InvalidateRect(NULL);
}

void SListBox::GetDesiredSize(THIS_ SIZE *psz, int nParentWid, int nParentHei)
{
    __baseCls::GetDesiredSize(psz, nParentWid, nParentHei);
    ILayoutParam *pLayoutParam = GetLayoutParam();
    if (pLayoutParam->IsWrapContent(Vert))
    {
        CRect rcPadding = GetStyle().GetPadding();
        psz->cy = GetItemHeight() * GetCount() + rcPadding.top + rcPadding.bottom;
        if (nParentHei > 0 && psz->cy > nParentHei)
            psz->cy = nParentHei;
    }
}

SNSEND