#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "TTF.h"

#ifdef _DEBUG
#define APPDEGUGASSERT(expression) (assert(expression))
#else
#define APPDEGUGASSERT(expression) (expression)
#endif 

HANDLE g_hStdOutput;
wchar_t *g_strInputFileName;
wchar_t *g_strOutputFileNameCmap;
wchar_t *g_strOutputFileNameGlyf;
float g_interval;

TTHFont g_hFont;
float g_FontRectxMin;
float g_FontRectyMin;
float g_FontRectWidth;
float g_FontRectHeight;
uint8_t g_bIsUCS2Supported;
uint8_t g_bIsUCS4Supported;

inline bool ttf_init();
inline void ttf_free();

HANDLE g_hFileFontCmap;
#ifdef _DEBUG
DWORD g_hFilePointerFontCmap;
#endif
HANDLE g_hFileFontGlyf;
DWORD g_hFilePointerFontGlyf;


inline bool ttf_processUCS2();
inline bool ttf_processUCS4();

struct APPINITEXCEPT
{

};

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	g_hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	if (argc != 2&& argc != 3)
	{
		DWORD numberOfCharsToWrite;
		APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Usage TTFPreprocess <InputFile> [InvertOfInterval (default 20)]", 64, &numberOfCharsToWrite, NULL) != FALSE);
		APPDEGUGASSERT(numberOfCharsToWrite == 64);
		return 0;
	}
	
	if (argc == 3)
	{
		unsigned int interval_inv;
		if (swscanf_s(argv[2], L"%u", &interval_inv) != 1)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Invalid Input Interval!", 23, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 23);
			return 1;
		}
		if (interval_inv == 0U)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Invalid Input Interval!", 23, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 23);
			return 1;
		}
		g_interval = static_cast<float>(1U) / static_cast<float>(interval_inv);
	}
	else
		g_interval = 0.05f;
	
	int rtval;

	g_strInputFileName = argv[1];
	g_strOutputFileNameCmap = NULL;
	g_strOutputFileNameGlyf = NULL;	
	
	g_hFont = NULL;
	g_hFileFontCmap = INVALID_HANDLE_VALUE;
	g_hFileFontGlyf = INVALID_HANDLE_VALUE;

	try {
		int strInputLen = lstrlenW(g_strInputFileName);
		
		g_strOutputFileNameCmap = static_cast<wchar_t *>(CoTaskMemAlloc(sizeof(wchar_t)*(strInputLen + 6)));
		if (g_strOutputFileNameCmap == NULL)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Fail to alloc memory!\n", 22, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 22);
			throw APPINITEXCEPT();
		}
		lstrcpynW(g_strOutputFileNameCmap, g_strInputFileName, strInputLen + 1);
		lstrcatW(g_strOutputFileNameCmap, L".cmap");

		g_strOutputFileNameGlyf = static_cast<wchar_t *>(CoTaskMemAlloc(sizeof(wchar_t)*(strInputLen + 6)));
		if (g_strOutputFileNameGlyf == NULL)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Fail to alloc memory!\n", 22, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 22);
			throw APPINITEXCEPT();
		}
		lstrcpynW(g_strOutputFileNameGlyf, g_strInputFileName, strInputLen + 1);
		lstrcatW(g_strOutputFileNameGlyf, L".glyf");

		if (!ttf_init())
			throw APPINITEXCEPT();
		
		g_hFileFontCmap = CreateFileW(g_strOutputFileNameCmap, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (g_hFileFontCmap == INVALID_HANDLE_VALUE)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Can not open the output cmap file!\n", 35, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 35);
			throw APPINITEXCEPT();
		}
#ifdef _DEBUG
		g_hFilePointerFontCmap = 0U;
#endif
		g_hFileFontGlyf = CreateFileW(g_strOutputFileNameGlyf, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (g_hFileFontGlyf == INVALID_HANDLE_VALUE)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Can not open the output glyf file!\n", 35, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 35);
			throw APPINITEXCEPT();
		}
		g_hFilePointerFontGlyf = 0U;

		if (g_bIsUCS4Supported)
		{
			
			ttf_processUCS4();
		}
		else if (g_bIsUCS2Supported)
		{
			ttf_processUCS2();
		}
		else
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"The input file does not support UCS2 or UCS4!\n", 46, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 46);
			throw APPINITEXCEPT();
		}

		rtval = 0;
	}
	catch (APPINITEXCEPT)
	{
		rtval = 1;
	}


	if (g_strOutputFileNameCmap != NULL)
		CoTaskMemFree(g_strOutputFileNameCmap);
	if (g_strOutputFileNameGlyf != NULL)
		CoTaskMemFree(g_strOutputFileNameGlyf);
	ttf_free();
	if (g_hFileFontCmap != INVALID_HANDLE_VALUE)
		APPDEGUGASSERT(CloseHandle(g_hFileFontCmap) == TRUE);
	if (g_hFileFontGlyf != INVALID_HANDLE_VALUE)
		APPDEGUGASSERT(CloseHandle(g_hFileFontGlyf) == TRUE);

    return rtval;
}


inline void TTFReportError(uint8_t rs)
{
	switch (rs)
	{
	case TTMEMORYALLOCFAILED:
	{
		DWORD numberOfCharsToWrite;
		APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Fail to alloc memory!\n", 22, &numberOfCharsToWrite, NULL) != FALSE);
		APPDEGUGASSERT(numberOfCharsToWrite == 22);
		break;
	}
	case TTINVALIEFONTFORMAT:
	{
		DWORD numberOfCharsToWrite;
		APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"The input file is not in a invalid TTF format!\n", 47, &numberOfCharsToWrite, NULL) != FALSE);
		APPDEGUGASSERT(numberOfCharsToWrite == 47);
		break;
	}
	default:
		APPDEGUGASSERT(FALSE);
	}
}

struct TTFINITEXCPT
{

};

inline bool ttf_init()
{
	bool rtval;
	HANDLE hTTFFile = INVALID_HANDLE_VALUE;
	void *pTTFFile = NULL;
	try {
		uint8_t rsTTF;
		DWORD szTTFFile;

		rsTTF = TTLibraryInit((void *(__stdcall *)(size_t))CoTaskMemAlloc, CoTaskMemFree);
		if (rsTTF != TTSUCCESS)
		{
			TTFReportError(rsTTF);
			throw TTFINITEXCPT();
		}

		hTTFFile = CreateFileW(g_strInputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hTTFFile == INVALID_HANDLE_VALUE)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Can not open the input file!\n", 29, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 29);
			throw TTFINITEXCPT();
		}

		szTTFFile = GetFileSize(hTTFFile, NULL);
		if (szTTFFile == 0U)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"The input file is not in a invalid TTF format!\n", 47, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 47);
			throw TTFINITEXCPT();
		}
		
		pTTFFile = CoTaskMemAlloc(szTTFFile);
		if (pTTFFile == NULL)
		{
			DWORD numberOfCharsToWrite;
			APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Fail to alloc memory!\n", 22, &numberOfCharsToWrite, NULL) != FALSE);
			APPDEGUGASSERT(numberOfCharsToWrite == 22);
			throw TTFINITEXCPT();
		}
		
		DWORD numberOfBytesRead;
		APPDEGUGASSERT(ReadFile(hTTFFile, pTTFFile, szTTFFile, &numberOfBytesRead, NULL) != FALSE);
		APPDEGUGASSERT(szTTFFile == numberOfBytesRead);

		rsTTF = TTFontLoadFromMemory(pTTFFile, szTTFFile, &g_hFont);
		if (rsTTF != TTSUCCESS)
		{
			TTFReportError(rsTTF);
			throw TTFINITEXCPT();
		}

		rsTTF = TTFontIsUCS2Supported(g_hFont, &g_bIsUCS2Supported);
		if (rsTTF != TTSUCCESS)
		{
			TTFReportError(rsTTF);
			throw TTFINITEXCPT();
		}
		rsTTF = TTFontIsUCS4Supported(g_hFont, &g_bIsUCS4Supported);
		if (rsTTF != TTSUCCESS)
		{
			TTFReportError(rsTTF);
			throw TTFINITEXCPT();
		}

		const TTRect *pFontRect;
		rsTTF = TTFontGetBound(g_hFont, &pFontRect);
		if (rsTTF != TTSUCCESS)
		{
			TTFReportError(rsTTF);
			throw TTFINITEXCPT();
		}
		g_FontRectxMin = static_cast<float>(pFontRect->xMin);
		g_FontRectyMin = static_cast<float>(pFontRect->yMin);
		g_FontRectWidth = static_cast<float>(pFontRect->xMax - pFontRect->xMin);
		g_FontRectHeight = static_cast<float>(pFontRect->yMax - pFontRect->yMin);

		rtval = true;
	}
	catch (TTFINITEXCPT)
	{
		rtval = false;
	}

	if (hTTFFile != INVALID_HANDLE_VALUE)
		APPDEGUGASSERT(CloseHandle(hTTFFile) == TRUE);
	if (pTTFFile != NULL)
		CoTaskMemFree(pTTFFile);

	return rtval;
}


inline void ttf_free()
{
	TTFontFree(g_hFont);
	TTLibraryFree();
}

struct TTFPROCESSEXCEPT
{

};

inline void ttf_processglyphsimple(TTHGlyph hGlyph);

inline bool ttf_processUCS2()
{
	bool rtval = true;
	uint8_t rsTTF;
	union
	{
		uint32_t ui;
		float f;
	}value32bit;
	DWORD numberOfBytesWritten;

	uint16_t charcode = 0U;
	do
	{
		TTHGlyph hGlyph = NULL;
		try {
			uint16_t glyphID;
			rsTTF = TTFontGetGlyphIDUCS2(g_hFont, charcode, &glyphID);
			if (rsTTF != TTSUCCESS)
			{
				TTFReportError(rsTTF);
				throw TTFPROCESSEXCEPT();
			}

			if (glyphID == 0)
			{
				value32bit.ui = (uint32_t)-1;
				APPDEGUGASSERT(WriteFile(g_hFileFontCmap, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
				APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
#ifdef _DEBUG
				g_hFilePointerFontCmap += sizeof(uint32_t);
#endif
			}
			else
			{
				rsTTF = TTGlyphInit(g_hFont, glyphID, &hGlyph);
				if (rsTTF != TTSUCCESS)
				{
					TTFReportError(rsTTF);
					throw TTFPROCESSEXCEPT();
				}

				uint8_t bIsSimple;
				rsTTF = TTGlyphIsSimple(hGlyph, &bIsSimple);
				if (rsTTF != TTSUCCESS)
				{
					TTFReportError(rsTTF);
					throw TTFPROCESSEXCEPT();
				}

				if (bIsSimple)
				{
					value32bit.ui = g_hFilePointerFontGlyf;
					APPDEGUGASSERT(WriteFile(g_hFileFontCmap, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
					APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);

#ifdef _DEBUG
					g_hFilePointerFontCmap += sizeof(uint32_t);
#endif

					ttf_processglyphsimple(hGlyph);
				}
				else
				{
					DWORD numberOfCharsToWrite;
					wchar_t str[7];
					APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Compound Glyph Not Supported! Charcode 0X", 41, &numberOfCharsToWrite, NULL) != FALSE);
					APPDEGUGASSERT(numberOfCharsToWrite == 41);
					wsprintfW(str, L"%06x", charcode);
					APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, str, 6, &numberOfCharsToWrite, NULL) != FALSE);
					APPDEGUGASSERT(numberOfCharsToWrite == 6);
					APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"\n", 1, &numberOfCharsToWrite, NULL) != FALSE);
					APPDEGUGASSERT(numberOfCharsToWrite == 1);

					value32bit.ui = (uint32_t)-1;
					APPDEGUGASSERT(WriteFile(g_hFileFontCmap, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
					APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
#ifdef _DEBUG
					g_hFilePointerFontCmap += sizeof(uint32_t);
#endif
				}
			}

		}
		catch (TTFPROCESSEXCEPT)
		{
			rtval = false;
		}
		if (hGlyph != NULL)
			TTGlyphFree(hGlyph);

		++charcode;//考虑到charcode为0时溢出的情形
	}while (rtval && (charcode != 0U));
	return rtval;
}

inline bool ttf_processUCS4()
{
	bool rtval = true;
	uint8_t rsTTF;
	union
	{
		uint32_t ui;
		float f;
	}value32bit;
	DWORD numberOfBytesWritten;

	for (uint32_t charcode = 0U; rtval && (charcode <= 0X10FFFFU); ++charcode)
	{
		TTHGlyph hGlyph = NULL;
		try {
			uint16_t glyphID;
			rsTTF = TTFontGetGlyphIDUCS2(g_hFont, charcode, &glyphID);
			if (rsTTF != TTSUCCESS)
			{
				TTFReportError(rsTTF);
				throw TTFPROCESSEXCEPT();
			}


			if (glyphID == 0)
			{
				value32bit.ui = (uint32_t)-1;
				APPDEGUGASSERT(WriteFile(g_hFileFontCmap, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
				APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);

#ifdef _DEBUG
				g_hFilePointerFontCmap += sizeof(uint32_t);
#endif

			}
			else
			{
				rsTTF = TTGlyphInit(g_hFont, glyphID, &hGlyph);
				if (rsTTF != TTSUCCESS)
				{
					TTFReportError(rsTTF);
					throw TTFPROCESSEXCEPT();
				}

				uint8_t bIsSimple;
				rsTTF = TTGlyphIsSimple(hGlyph, &bIsSimple);
				if (rsTTF != TTSUCCESS)
				{
					TTFReportError(rsTTF);
					throw TTFPROCESSEXCEPT();
				}

				if (bIsSimple)
				{
					value32bit.ui = g_hFilePointerFontGlyf;
					APPDEGUGASSERT(WriteFile(g_hFileFontCmap, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
					APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);

#ifdef _DEBUG
					g_hFilePointerFontCmap += sizeof(uint32_t);
#endif

					ttf_processglyphsimple(hGlyph);
				}
				else
				{
					DWORD numberOfCharsToWrite;
					wchar_t str[7];
					APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"Compound Glyph Not Supported! Charcode 0X", 41, &numberOfCharsToWrite, NULL) != FALSE);
					APPDEGUGASSERT(numberOfCharsToWrite == 41);
					wsprintfW(str, L"%06x", charcode);
					APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, str, 6, &numberOfCharsToWrite, NULL) != FALSE);
					APPDEGUGASSERT(numberOfCharsToWrite == 6);
					APPDEGUGASSERT(WriteConsoleW(g_hStdOutput, L"\n", 1, &numberOfCharsToWrite, NULL) != FALSE);
					APPDEGUGASSERT(numberOfCharsToWrite == 1);

					value32bit.ui = (uint32_t)-1;
					APPDEGUGASSERT(WriteFile(g_hFileFontCmap, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
					APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
#ifdef _DEBUG
					g_hFilePointerFontCmap += sizeof(uint32_t);
#endif
				}
			}
		}
		catch (TTFPROCESSEXCEPT)
		{
			rtval = false;
		}
		if (hGlyph != NULL)
			TTGlyphFree(hGlyph);
	}
	return rtval;
}

inline void ttf_processglyphsimple(TTHGlyph hGlyph)
{
	uint8_t rsTTF;
	union
	{
		uint32_t ui;
		float f;
	}value32bit;
	DWORD numberOfBytesWritten;

	const TTRect *pGlyphRect;
	rsTTF = TTGlyphGetBound(hGlyph, &pGlyphRect);
	if (rsTTF != TTSUCCESS)
	{
		TTFReportError(rsTTF);
		throw TTFPROCESSEXCEPT();
	}

	value32bit.f = static_cast<float>(pGlyphRect->xMin - g_FontRectxMin) / g_FontRectWidth;
	APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
	APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
	g_hFilePointerFontGlyf += sizeof(uint32_t);

	value32bit.f = static_cast<float>(pGlyphRect->xMax - g_FontRectxMin) / g_FontRectWidth;
	APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
	APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
	g_hFilePointerFontGlyf += sizeof(uint32_t);

	value32bit.f = static_cast<float>(pGlyphRect->yMin - g_FontRectyMin) / g_FontRectHeight;
	APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
	APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
	g_hFilePointerFontGlyf += sizeof(uint32_t);

	value32bit.f = static_cast<float>(pGlyphRect->yMax - g_FontRectyMin) / g_FontRectHeight;
	APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
	APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
	g_hFilePointerFontGlyf += sizeof(uint32_t);

	uint16_t PointerNumber;
	const TTGlyphSimplePoint *PointArray;
	uint16_t ContourNumber;
	const uint16_t *pContourEndPointerIndexArray;

	rsTTF = TTGlyphSimpleGetPointNumber(hGlyph, &PointerNumber);
	if (rsTTF != TTSUCCESS)
	{
		TTFReportError(rsTTF);
		throw TTFPROCESSEXCEPT();
	}
	rsTTF = TTGlyphSimpleGetPointArray(hGlyph, &PointArray);
	if (rsTTF != TTSUCCESS)
	{
		TTFReportError(rsTTF);
		throw TTFPROCESSEXCEPT();
	}
	rsTTF = TTGlyphSimpleGetContourNumber(hGlyph, &ContourNumber);
	if (rsTTF != TTSUCCESS)
	{
		TTFReportError(rsTTF);
		throw TTFPROCESSEXCEPT();
	}
	rsTTF = TTGlyphSimpleGetContourEndPointerIndex(hGlyph, &pContourEndPointerIndexArray);
	if (rsTTF != TTSUCCESS)
	{
		TTFReportError(rsTTF);
		throw TTFPROCESSEXCEPT();
	}

	//写入轮廓个数
	value32bit.ui = static_cast<uint32_t>(ContourNumber);
	APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
	APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
	g_hFilePointerFontGlyf += sizeof(uint32_t);

	uint16_t ContourIndexBegin;
	uint16_t ContourIndexEnd = (uint16_t)-1;

	for (uint16_t ci = 0U; ci < ContourNumber; ++ci)
	{
		ContourIndexBegin = ContourIndexEnd + 1U;
		ContourIndexEnd = pContourEndPointerIndexArray[ci];

		uint32_t vbi = 0U;//轮廓中的顶点个数
						  //为轮廓中的顶点个数预留位置
		SetFilePointer(g_hFileFontGlyf, 4, NULL, FILE_CURRENT);
		g_hFilePointerFontGlyf += sizeof(uint32_t);

		//写入顶点数据
		uint16_t pi = ContourIndexBegin;
		do {
			if (PointArray[pi].bIsOnCurve)//顶点在轮廓上
			{
				value32bit.f = static_cast<float>(PointArray[pi].x - g_FontRectxMin) / g_FontRectWidth;
				APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
				APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
				g_hFilePointerFontGlyf += sizeof(uint32_t);

				value32bit.f = static_cast<float>(PointArray[pi].y - g_FontRectyMin) / g_FontRectHeight;
				APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
				APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
				g_hFilePointerFontGlyf += sizeof(uint32_t);

				++vbi;//添加一个顶点
			}
			else//顶点是二次贝塞尔曲线的控制点
			{
				float p0[2];
				float p1[2];
				float p2[2];
				p1[0] = static_cast<float>(PointArray[pi].x - g_FontRectxMin) / g_FontRectWidth;
				p1[1] = static_cast<float>(PointArray[pi].y - g_FontRectyMin) / g_FontRectHeight;

				uint16_t lastindex = pi - 1;
				if (lastindex == uint16_t(-1) //考虑到lastindex为0时溢出的情形
					|| lastindex < ContourIndexBegin)
					lastindex = ContourIndexEnd - (ContourIndexBegin - lastindex - 1U);
				if (PointArray[lastindex].bIsOnCurve)//顶点在轮廓上
				{
					p0[0] = static_cast<float>(PointArray[lastindex].x - g_FontRectxMin) / g_FontRectWidth;
					p0[1] = static_cast<float>(PointArray[lastindex].y - g_FontRectyMin) / g_FontRectHeight;
					//在当顶点在轮廓上成立的判断分支中会添加该顶点，不需要在此添加
				}
				else//两个Off之间的中点On被省略
				{
					int16_t x = PointArray[lastindex].x / 2U + PointArray[pi].x / 2U;
					int16_t y = PointArray[lastindex].y / 2U + PointArray[pi].y / 2U;
					p0[0] = static_cast<float>(x - g_FontRectxMin) / g_FontRectWidth;
					p0[1] = static_cast<float>(y - g_FontRectxMin) / g_FontRectHeight;

					value32bit.f = p0[0];
					APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
					APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
					g_hFilePointerFontGlyf += sizeof(uint32_t);

					value32bit.f = p0[1];
					APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
					APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
					g_hFilePointerFontGlyf += sizeof(uint32_t);

					++vbi;//添加被省略的顶点
				}

				uint16_t nextindex = pi + 1;
				if (nextindex == 0 //考虑到nextindex为uint16_t(-1)时溢出的情形
					|| nextindex > ContourIndexEnd)
					nextindex = ContourIndexBegin + (nextindex - ContourIndexEnd - 1U);
				if (PointArray[nextindex].bIsOnCurve)
				{
					p2[0] = static_cast<float>(PointArray[nextindex].x - g_FontRectxMin) / g_FontRectWidth;
					p2[1] = static_cast<float>(PointArray[nextindex].y - g_FontRectyMin) / g_FontRectHeight;
					//在当顶点在轮廓上成立的判断分支中会添加该顶点，不需要在此添加
				}
				else//两个Off之间的中点On被省略
				{
					int16_t x = PointArray[nextindex].x / 2U + PointArray[pi].x / 2U;
					int16_t y = PointArray[nextindex].y / 2U + PointArray[pi].y / 2U;
					//该被省略的顶点会在下一个二次贝塞尔曲线中被添加，不需要在此添加
					p2[0] = static_cast<float>(x - g_FontRectxMin) / g_FontRectWidth;
					p2[1] = static_cast<float>(y - g_FontRectyMin) / g_FontRectHeight;
				}

				float dst = 0.0f;
				float x = p1[0] - p0[0];
				float y = p1[1] - p0[1];
				dst += sqrtf(x*x + y*y);
				x = p2[0] - p1[0];
				y = p2[1] - p1[1];
				dst += sqrtf(x*x + y*y);

				//显然，间隔越小，产生的顶点就越多，绘制的曲线也就越平滑，当然，还可以结合MSAA（见4.4.2.1）
				uint32_t NumFloat2 = static_cast<uint32_t>(dst / g_interval);
				if (NumFloat2 > 1U)//否则，由于计算的顶点中不包括二次贝塞尔曲线的端点，不需要计算顶点
				{
					float l_interval = 1.0f / static_cast<float>(NumFloat2);
					--NumFloat2;
					for (uint32_t vi = 1U; vi <= NumFloat2; ++vi)
					{
						float t = g_interval*vi;
						float t_inv = 1.0f - t;

						value32bit.f = (t_inv*t_inv)*p0[0] + (2 * t_inv*t)*p1[0] + (t*t)*p2[0];
						APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
						APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
						g_hFilePointerFontGlyf += sizeof(uint32_t);

						value32bit.f = (t_inv*t_inv)*p0[1] + (2 * t_inv*t)*p1[1] + (t*t)*p2[1];
						APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
						APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
						g_hFilePointerFontGlyf += sizeof(uint32_t);
					}
					vbi += NumFloat2;//产生NumFloat2个顶点
				}
			}
			++pi;
			if (pi == 0 //考虑到pi为uint16_t(-1)时溢出的情形
				|| pi > ContourIndexEnd)
				pi = ContourIndexBegin + (pi - ContourIndexEnd - 1U);
		} while (pi != ContourIndexBegin);

		//写入轮廓中的顶点个数
		SetFilePointer(g_hFileFontGlyf, (-4) + (-8)*static_cast<int32_t>(vbi), NULL, FILE_CURRENT);
		value32bit.ui = vbi;
		APPDEGUGASSERT(WriteFile(g_hFileFontGlyf, &value32bit.ui, sizeof(uint32_t), &numberOfBytesWritten, NULL) != FALSE);
		APPDEGUGASSERT(sizeof(uint32_t) == numberOfBytesWritten);
		SetFilePointer(g_hFileFontGlyf, 8 * static_cast<int32_t>(vbi), NULL, FILE_CURRENT);
	}
}