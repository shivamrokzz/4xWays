

#ifndef WEBCTRL_H
#define WEBCTRL_H


#define WC_WEBCTRLA "WebCtrl32"
#define WC_WEBCTRLW L"WebCtrl32"

#define WC_WEBCTRL WC_WEBCTRLW 




// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <windowsx.h>
#include <ocidl.h>
#include <shlobj.h>
#include <tchar.h>
#include "WebControlEvents.h"
#include "resource.h"

#define THIS_PROP   _T("this")

#define MAX_INPUT_TAG 2

class CWebCtrl
{
public:

	CWebCtrl(HWND hParent, HINSTANCE m_hinst, int nCmdShow, int startx, int starty, int width, int height);
	~CWebCtrl();
	void CopySelection();
	void Print();
	void WriteToHtmlDoc(wchar_t* buffer);
	void ParseHTML (char * HiddenStr, char * SearchString, char * NumResults, int *AllWord, char * MetaString);
	int GetHtml(char *SelectOption,char *value, bool);
	void AppendToHtmlDoc(wchar_t* buffer);
	void SetVisible(bool visible);	
	void Navigate( LPTSTR );
	void Resize(int x, int y, int width, int height);
	CWebContainer *m_pContainer;
	HWND m_hwndWebCtrl;
	IWebBrowser2 *m_pweb;

private:
	
	bool WINAPI InitWebCtrl();
	bool WINAPI UninitWebCtrl();
	
	static LRESULT CALLBACK WebCtrlProc( HWND, UINT, WPARAM, LPARAM );
	
	int  m_cxScroll;
	int  m_cyScroll;
	
};


#endif // WEBCTRL_H
