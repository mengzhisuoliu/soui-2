﻿/********************************************************************
created:	2014/10/01
created:	01:10:2014   16:32
filename: 	SSkinAPNG.h
file base:	SSkinAPNG
file ext:	h
author:		huangjianxiong

purpose:	动画PNG（APNG）扩展支持，依赖于当前使用的图片解码器。
            目前只有imgdecoder-png支持APNG解码
*********************************************************************/
#pragma once
#include <interface/SSkinobj-i.h>
#include <helper/obj-ref-impl.hpp>

#include "SSkinAni.h"

namespace SOUI
{

    /**
    * @class     SSkinAPNG
    * @brief     APNG图片加载及显示对象
    * 
    * Describe
    */
    class SSkinAPNG : public SSkinAni
    {
        DEF_SOBJECT(SSkinAni, L"apng")
    public:
		SSkinAPNG();
		~SSkinAPNG();
        
        /**
         * LoadFromFile
         * @brief    从文件加载APNG
         * @param    LPCTSTR pszFileName --  文件名
         * @return   int -- APNG帧数，0-失败
         * Describe  
         */    
        int LoadFromFile(LPCTSTR pszFileName);

        /**
         * LoadFromMemory
         * @brief    从内存加载APNG
         * @param    LPVOID pBits --  内存地址
         * @param    size_t szData --  内存数据长度
         * @return   int -- APNG帧数，0-失败
         * Describe  
         */    
        int LoadFromMemory(LPVOID pBits,size_t szData);

        SOUI_ATTRS_BEGIN()
            ATTR_CUSTOM(L"src",OnAttrSrc)   //XML文件中指定的图片资源名,(type:name)
        SOUI_ATTRS_END()

	public:
		STDMETHOD_(SIZE,GetSkinSize)(THIS) SCONST OVERRIDE;
		STDMETHOD_(int,GetStates)(THIS) SCONST OVERRIDE;
	protected:
        /**
        * GetFrameDelay
        * @brief    获得指定帧的显示时间
        * @param    int iFrame --  帧号,为-1时代表获得当前帧的延时
        * @return   long -- 延时时间(*10ms)
        * Describe  
        */    
        virtual long GetFrameDelay(int iFrame=-1) const override;
        
        /**
        * Draw
        * @brief    绘制指定帧的GIF图
        * @param    IRenderTarget * pRT --  绘制目标
        * @param    LPCRECT rcDraw --  绘制范围
        * @param    DWORD dwState --  绘制状态，这里被解释为帧号
        * @param    BYTE byAlpha --  透明度
        * @return   void
        * Describe  
        */    
        virtual void _DrawByIndex2(IRenderTarget *pRT, LPCRECT rcDraw, int dwState,BYTE byAlpha=0xFF) const  override;

		virtual void _Scale(ISkinObj *pObj, int nScale) override;

		IBitmapS * GetFrameImage(int iFrame=-1);

    protected:
        HRESULT OnAttrSrc(const SStringW &strValue,BOOL bLoading);
        
        int _InitImgFrame(IImgX *pImgX);

	protected:
		SAniFrame * m_pFrames;
    };
}//end of name space SOUI
