#include "SkFontMgr.h"
#include <windows.h>
#include "SkTypeface_win.h"

SkFontMgr* SkFontMgr::Factory() {
    return SkFontMgr_New_GDI();
}
