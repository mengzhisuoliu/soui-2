﻿// imgdecoder-gdip.h : Defines the exported functions for the DLL application.
//


#include <helper/obj-ref-impl.hpp>
#include <interface/SImgDecoder-i.h>

namespace Gdiplus
{
	class Bitmap;
}

SNSBEGIN

class SImgFrame_GDIP : public TObjRefImpl<IImgFrame> {
public:
	SImgFrame_GDIP();
	~SImgFrame_GDIP();

	void Attach(const BYTE * pdata,int nWid,int nHei,int nDelay);

	STDMETHOD_(BOOL,GetSize)(THIS_ UINT *pWid,UINT *pHei) OVERRIDE;
	STDMETHOD_(int,GetDelay)(THIS) OVERRIDE {return m_nFrameDelay;}
    STDMETHOD_(const VOID *,GetPixels)(CTHIS) SCONST OVERRIDE;
protected:
	int     m_nFrameDelay;
	int     m_nWid, m_nHei;
	BYTE   *m_pdata;
};

class SImgX_GDIP : public TObjRefImpl<IImgX>
{
	friend class SImgDecoderFactory_GDIP;
public:
	STDMETHOD_(int,LoadFromMemory)(THIS_ void *pBuf,size_t bufLen) OVERRIDE;
	STDMETHOD_(int,LoadFromFileW)(THIS_ LPCWSTR pszFileName) OVERRIDE;
	STDMETHOD_(int,LoadFromFileA)(THIS_ LPCSTR pszFileName) OVERRIDE;
	STDMETHOD_(UINT,GetFrameCount)(THIS) OVERRIDE;
	STDMETHOD_(IImgFrame *, GetFrame)(THIS_ UINT iFrame) OVERRIDE;
protected:
	SImgX_GDIP(BOOL bPremultiplied);
	~SImgX_GDIP(void);

	int _InitFromGdipBitmap(Gdiplus::Bitmap * pSrcBmp);

	BOOL m_bPremultiplied;

	int                  m_nFrameCount;
	SImgFrame_GDIP  *    m_pImgArray;
};

#define DESC_IMGDECODER L"gdi+"
class SImgDecoderFactory_GDIP : public TObjRefImpl<IImgDecoderFactory>
{
	friend class SImgX_GDIP;
public:
	SImgDecoderFactory_GDIP();
	~SImgDecoderFactory_GDIP();

	STDMETHOD_(BOOL,CreateImgX)(THIS_ IImgX **ppImgDecoder) OVERRIDE;
	STDMETHOD_(HRESULT,SaveImage)(THIS_ BYTE* pBits, int nWid,int nHei, LPCWSTR pszFileName, const void* pFormat) SCONST OVERRIDE;
	STDMETHOD_(HRESULT,SaveImage2)(THIS_ BYTE* pBits, int nWid,int nHei, LPCWSTR pszFileName, ImgFmt imgFmt) SCONST OVERRIDE;
	STDMETHOD_(LPCWSTR,GetDescription)(THIS) SCONST OVERRIDE;
protected:


	ULONG_PTR _gdiPlusToken;
};

//////////////////////////////////////////////////////////////////////////
namespace IMGDECODOR_GDIP
{
	SOUI_COM_C BOOL SOUI_COM_API SCreateInstance(IObjRef **pImgDecoderFactory);
}

SNSEND

EXTERN_C BOOL SOUI_COM_API Decoder_Gdip_SCreateInstance(IObjRef **pImgDecoderFactory);
