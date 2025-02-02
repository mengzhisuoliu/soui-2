﻿#include "souistd.h"
#include "SApp.h"
#include "control/SMessageBox.h"
#include "control/SCmnCtrl.h"
#include <control/STabCtrl.h>

SNSBEGIN
//////////////////////////////////////////////////////////////////////////

SMessageBoxImpl::SMessageBoxImpl()
    : SHostDialog()
{
}

static struct MsgBoxInfo
{
    LPCTSTR pszText;
    LPCTSTR pszCaption;
    UINT uType;
} s_MsgBoxInfo;

INT_PTR SMessageBoxImpl::MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
    SXmlNode xmlTemplate = SApplication::getSingletonPtr()->GetMessageBoxTemplate();
    if (!xmlTemplate)
        return ::MessageBox(hWnd, lpText, lpCaption, uType);
    s_MsgBoxInfo.pszText = lpText;
    s_MsgBoxInfo.pszCaption = lpCaption;
    s_MsgBoxInfo.uType = uType;
    return DoModal(hWnd);
}

BOOL SMessageBoxImpl::OnSetIcon(UINT uType)
{
    SImageWnd *pIcon = FindChildByName2<SImageWnd>(NAME_MSGBOX_ICON);
    if (!pIcon)
        return FALSE;
    switch (uType & 0xF0)
    {
    case MB_USERICON: // done
        pIcon->SetIcon(0);
        break;
    case MB_ICONWARNING:
        pIcon->SetIcon(1);
        break;
    case MB_ICONINFORMATION:
        pIcon->SetIcon(2);
        break;
    case MB_ICONSTOP:
        pIcon->SetIcon(3);
        break;
    case MB_ICONQUESTION:
        pIcon->SetIcon(4);
        break;
    default:
        pIcon->SetVisible(FALSE, TRUE);
        break;
    }
    return TRUE;
}

struct MSGBTN_TEXT
{
    int nBtns; //按钮数,<=3
    struct
    {
        UINT uBtnID;      //按钮ID
        WCHAR szText[20]; //按钮字符
    } btnInfo[3];
} g_msgBtnText[] = {
    // MB_OK
    { 1, { { IDOK, L"ok" }, { 0, L"" }, { 0, L"" } } },
    // MB_OKCANCEL
    { 2, { { IDOK, L"ok" }, { IDCANCEL, L"cancel" }, { 0, L"" } } },
    // MB_ABORTRETRYIGNORE
    { 3, { { IDABORT, L"abort" }, { IDRETRY, L"retry" }, { IDIGNORE, L"ignore" } } },
    // MB_YESNOCANCEL
    { 3, { { IDYES, L"yes" }, { IDNO, L"no" }, { IDCANCEL, L"cancel" } } },
    // MB_YESNO
    { 2, { { IDYES, L"yes" }, { IDNO, L"no" }, { 0, L"" } } },
    // MB_RETRYCANCEL
    { 2, { { IDRETRY, L"retry" }, { IDCANCEL, L"cancel" }, { 0, L"" } } }
};

const WCHAR *g_wcsNameOfBtns[] = { NAME_MSGBOX_BTN1, NAME_MSGBOX_BTN2, NAME_MSGBOX_BTN3 };

SStringT SMessageBoxImpl::OnGetButtonText(int nBtnID) const
{
    return SStringT();
}

SXmlNode SMessageBoxImpl::OnGetInitXmlNode(SXmlDoc &xmlDoc)
{
    return SApplication::getSingletonPtr()->GetMessageBoxTemplate();
}

BOOL SMessageBoxImpl::OnInitDialog(HWND wnd, LPARAM lInitParam)
{
    SXmlNode uiRoot = SApplication::getSingletonPtr()->GetMessageBoxTemplate();
    UINT uType = s_MsgBoxInfo.uType & 0x0F;

    STabCtrl *pBtnSwitch = FindChildByName2<STabCtrl>(NAME_MSGBOX_BTNSWITCH);
    SASSERT(pBtnSwitch);
    pBtnSwitch->SetCurSel(g_msgBtnText[uType].nBtns - 1);
    SWindow *pBtnPanel = pBtnSwitch->GetItem(g_msgBtnText[uType].nBtns - 1);
    SASSERT(pBtnPanel);

    SXmlNode nodeBtnTxt = uiRoot.child(L"buttonText");
    for (int i = 0; i < g_msgBtnText[uType].nBtns; i++)
    {
        SWindow *pBtn = pBtnPanel->FindChildByName(g_wcsNameOfBtns[i]);
        int nID = g_msgBtnText[uType].btnInfo[i].uBtnID;
        pBtn->SetID(nID);

        SStringT strText = OnGetButtonText(nID);
        SStringW strAccel;
        if (strText.IsEmpty())
        {
            SStringW strBtnText = g_msgBtnText[uType].btnInfo[i].szText;
            //先从模板中的buttonText节点里查按钮的文字
            SXmlNode nodeTxt = nodeBtnTxt.child(strBtnText);
            if (nodeTxt)
            {
                strBtnText = nodeTxt.Text();
                strBtnText.TrimBlank();
                strBtnText = GETSTRING(strBtnText);
                strAccel = nodeTxt.attribute(L"accel").as_string();
            }
            //从翻译引擎中翻译
            strText = S_CW2T(GetRoot()->tr(strBtnText));
        }
        if (!strAccel.IsEmpty())
        {
            pBtn->SetAttribute(L"accel", strAccel, TRUE);
        }
        pBtn->SetWindowText(strText);
    }

    SStringW strMinSize = uiRoot.attribute(L"minSize").value();
    SStringWList lstMinSize;
    SplitString(strMinSize, L',', lstMinSize);
    SASSERT(lstMinSize.GetCount() == 2);
    SLayoutSize szMin[2] = { SLayoutSize::fromString(lstMinSize[0]), SLayoutSize::fromString(lstMinSize[1]) };

    SWindow *pTitle = FindChildByName(NAME_MSGBOX_TITLE);
    SASSERT(pTitle);
    SStringT strTitle = pTitle->GetWindowText();
    if (s_MsgBoxInfo.pszCaption)
        strTitle = s_MsgBoxInfo.pszCaption;
    pTitle->SetWindowText(S_CW2T(TR(S_CT2W(strTitle), GetTranslatorContext())));

    SWindow *pMsg = FindChildByName(NAME_MSGBOX_TEXT);
    SASSERT(pMsg);
    pMsg->SetWindowText(S_CW2T(TR(S_CT2W(s_MsgBoxInfo.pszText), GetTranslatorContext())));

    OnSetIcon(s_MsgBoxInfo.uType);
    GetRoot()->UpdateLayout();

    CRect rcWnd = SHostWnd::GetWindowRect();
    CSize szWnd = rcWnd.Size();
    if (szWnd.cx < szMin[0].toPixelSize(GetScale()))
        szWnd.cx = szMin[0].toPixelSize(GetScale());
    if (szWnd.cy < szMin[1].toPixelSize(GetScale()))
        szWnd.cy = szMin[1].toPixelSize(GetScale());

    SetWindowPos(0, 0, 0, szWnd.cx, szWnd.cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    CenterWindow();
    SetForegroundWindow(m_hWnd);
    SetMsgHandled(FALSE); // chain to dpihandler.
    return 0;
}

//////////////////////////////////////////////////////////////////////////
INT_PTR SMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
    SMessageBoxImpl msgBox;
    return msgBox.MessageBox(hWnd, lpText, lpCaption, uType);
}

SNSEND
