﻿/********************************************************************
    created:	2014/11/03
    created:	3:11:2014   16:13
    filename: 	SIPAddressCtrl.h
    author:		冰

    purpose:	SOUI版的IP控件
*********************************************************************/
#pragma once

namespace SOUI
{

class SEditIP;

class SIPAddressCtrl : public SWindow {
    DEF_SOBJECT(SWindow, L"ipctrl")
  public:
    SIPAddressCtrl(void);
    ~SIPAddressCtrl(void);

    BOOL IsBlank() const;
    void ClearAddress();
    int GetAddress(BYTE &nField0, BYTE &nField1, BYTE &nField2, BYTE &nField3) const;
    int GetAddress(DWORD &dwAddress) const;

    void SetAddress(DWORD dwAddress);
    void SetAddress(BYTE nField0, BYTE nField1, BYTE nField2, BYTE nField3);

    void SetFieldFocus(WORD nField);
    void SetFieldRange(int nField, BYTE nLower, BYTE nUpper);

  protected:
    void OnPaint(IRenderTarget *pRT);
    LRESULT OnCreate(LPVOID);
    void OnSize(UINT nType, CSize size);

    SOUI_MSG_MAP_BEGIN()
    MSG_WM_CREATE(OnCreate)
    MSG_WM_SIZE(OnSize)
    MSG_WM_PAINT_EX(OnPaint)
    SOUI_MSG_MAP_END()
  private:
    SEditIP *m_editFields[4];
};
} // namespace SOUI
