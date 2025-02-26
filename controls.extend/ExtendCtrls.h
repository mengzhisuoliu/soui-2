﻿#pragma once
#include "core/SWnd.h"

namespace SOUI
{
class SWindowEx : public SWindow {
  public:
    DEF_SOBJECT(SWindow, L"windowex")

  protected:
    SOUI_MSG_MAP_BEGIN()
    MSG_WM_LBUTTONDBLCLK(OnLButtonDown) //将双击消息处理为单击
    SOUI_MSG_MAP_END()
};

}; // namespace SOUI