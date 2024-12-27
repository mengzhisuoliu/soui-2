﻿//
// cmake config.h.in
//
/* #undef ENABLE_SOUI_COM_LIB */
/* #undef ENABLE_SOUI_CORE_LIB */
/* #undef OUTPATH_WITHOUT_TYPE */

#ifdef ENABLE_SOUI_COM_LIB
    #define LIB_SOUI_COM		//SOUI组件编译为lib
#else
    #define DLL_SOUI_COM	//SOUI组件编译为dll
#endif

#ifdef ENABLE_SOUI_CORE_LIB
    #define LIB_CORE		//SOUI 内核编译为lib
#else
    #define DLL_CORE		//SOUI 内核编译为dll
#endif

#if defined(OUTPATH_WITHOUT_TYPE)
#define NO_DEBUG_SUFFIX
#endif
