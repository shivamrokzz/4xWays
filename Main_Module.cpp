

#define	MAIN_MODULE

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <ERRNO.H>
#include <atltypes.h>
#include <gdiplus.h>
#include <Exdisp.h>

#include <stdlib.h>
#include <crtdbg.h>
#include <shlwapi.h>
#include "resource.h"
#include "4xWays.h"
#include "WebControlEvents.h"
#include "WebControl.h"
#include <string>
#include <fstream> 
#include <sstream>
#include <ctime>
#include <codecvt>
#include <vector>
#include <algorithm>

using namespace std;
	
static HINSTANCE    ghInstance;
static wchar_t		*LastScriptResult;	//Pointer to a buffer to hold the search results
static wchar_t		*ScriptResults;
static wchar_t		*HTMLResults;
static wchar_t		*ErrorCaption = L"4xWays Error";
#define _CRTDBG_MAP_ALLOC
//Variable to store window coordinates
static  int			cxOld;
static  int			cyOld;
static HANDLE		hLogUpdated;
static bool			bRunButton = true;

// Variables for reading profiles folder
wstring profileNameTxt[NUM_PROFILES];
wstring friendlyName[NUM_PROFILES];


//Gdi
Gdiplus::Graphics *gs;
Gdiplus::Image *image ;

//Local global web control
CWebCtrl*	htmlWindow;

HCURSOR		Cursor;

//
// Function protypes
//
BOOL CALLBACK MainDlgProc	(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDlgProc	(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
int InitDialog(HWND hDlg, PWSTR lpCmdLine);
void ChangeCurser(CURSOR_TYPE cursor);
void OnSize(HWND hwnd, int x, int y); 
int  OnMaxMinInfo(HWND hwnd, LPMINMAXINFO WinInfo);
void OnBrowseDump(HWND hDlg);
void OnGetProcessList(HWND hDlg);
void OnRunCommand(HWND hDlg);
void OnCommandInfo(HWND hDlg);
void OnStopCommand(HWND hDlg);
void OnCopyToClipboard(HWND hwnd, wchar_t *szData);
void OnClearLog(HWND hwnd);
void OnSaveToFile(HWND hwnd);
void CreateHTMLControl(HWND hDlg);
void DisplayHTMLResult();
bool ReadConfigFile(wchar_t *szCfgFileName);
void SaveConfigFile(wchar_t *szCfgFileName);
void InitConfig(void);
void InitProcessList(void);
bool IsValidNumber(const wchar_t* pszText, bool allowMinus);
bool IsValidHex(const wchar_t* pszText, bool allowMinus);
bool RunVolatilityScript(wchar_t *CmdParameters);
void InitDialogControls(HWND hDlg);
void DisableControls(HWND hDlg);
void EnableControls(HWND hDlg);
void ShowCmdCheckControls(HWND hDlg, int cmdIndex);
void Cleanup(HWND hDlg);
void EnableCmdCheckControls(HWND hDlg, int cmdIndex);
void EnableCmdValueControl(HWND hDlg, int cmdIndex, int paramIndex);
void ShowCmdValueControl(HWND hDlg, int cmdIndex, int paramIndex);
void HideCmdValueControl(HWND hDlg, int cmdIndex, int paramIndex);
void UpdateCommandList(HWND hDlg, wchar_t* platform);
int GetCommandIndex(int index, wchar_t* platform);

bool SearchImgFile(wchar_t *pszExt, wchar_t *pszFileName);
wchar_t *str_replace(wchar_t *orig, wchar_t *rep, wchar_t *with, UINT64 *len);
void UpdateControl(HWND hDlg, int iControl, int cxDelta, int cyDelta, int iType);
bool PopFileLoadDlg(HWND hwnd, wchar_t* pstrFileName, wchar_t* filter, wchar_t* dialog_title);
BOOL PopFileSaveDlg(HWND hwnd, wchar_t* pstrFileName, wchar_t* pstrTitleName, wchar_t* filter, wchar_t* def_extension, wchar_t* dialog_title);
void GetPersonalFolder(wchar_t * szPath);
void PopFileInitialize(HWND hwnd);
void GetProcessesThread(LPVOID lpParameter);
void RunCommandThread(LPVOID lpParameter);
void CommandInfoThread(LPVOID lpParameter);
void AddLogEntry(wchar_t *pszLog, bool bDisplayTime, bool bAddToLog);
int SetProfileList(HWND hDlg);
int ReadProfilesTextFile(HWND hDlg, wchar_t* directory, const vector<wstring> &files);
void InitLog(void);


/********************************/
//WinMain
//
//Entry point
/********************************/
int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	MSG		msg;
	
	ghInstance = hInstance;

	//Get the applications directory
	GetModuleFileName (NULL, RunDirName, sizeof(RunDirName));
	*wcsrchr (RunDirName, '\\') = '\0';

	// Create and display the window	
	ghMainWnd = CreateDialog (hInstance, MAKEINTRESOURCE(IDD_CGIFRONTEND), 0, MainDlgProc);
	if (InitDialog(ghMainWnd, lpCmdLine) == NOK)
		return -1;
	ShowWindow (ghMainWnd, nCmdShow) ;

	SetTimer(ghMainWnd, 1, 500, NULL);
	// Set the window icon
	SetClassLongPtr(ghMainWnd, GCLP_HICON, (LONG_PTR)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)));

    while (GetMessage (&msg, NULL, 0, 0) )
    {
        if (!IsDialogMessage (ghMainWnd, &msg) )
        {
            TranslateMessage ( & msg );
            DispatchMessage ( & msg );
        }
    }

	//Memory leak detection
	_CrtDumpMemoryLeaks();
	return msg.wParam ;
}

int html_resize_offset = 0;

/********************************/
// WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
//
// Windows callback procedure.
// Handles all the messages for the dialog box
/********************************/
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{

	Gdiplus::Unit  units;
	CRect rect;
	WCHAR filename[200] = (L"VW-Logo-box-small.png");
	ULONG_PTR	m_gdiplusToken;
	int cmdIndex;
	bool bChecked = true;
	int width , height;

	switch (iMsg)
	{
	case WM_APP:
		DisplayHTMLResult();
		break;

	case WM_TIMER:
		SendMessage(GetDlgItem(ghMainWnd, IDC_STATUS), WM_SETTEXT, 0, (LPARAM)gStrProgress);
		break;

	case WM_GETMINMAXINFO:
		OnMaxMinInfo(hDlg, (LPMINMAXINFO)lParam);
		break;

	case WM_SIZE:
		OnSize(hDlg, LOWORD(lParam), HIWORD(lParam));
		width = LOWORD(lParam) - WEBCTRL_WIDTHOFFSET_X;
		height = HIWORD(lParam) - WEBCTRL_HEIGHTOFFSET_X;
		htmlWindow->Resize(WEBCTRL_OFFSET_X, WEBCTRL_OFFSET_Y + html_resize_offset, width, height - html_resize_offset);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_COPYTOCLIPBOARD:
			OnCopyToClipboard(hDlg, ScriptResults);
			break;

		case IDC_CLEARLOG:
			OnClearLog(hDlg); 
			break;

		case IDC_SAVE:
			OnSaveToFile(hDlg);
			break;

		case IDC_COMMAND:
			if (HIWORD(wParam) == CBN_SELENDOK)
			{
				int index;
				wchar_t command[MAXFILENAME];

				index = SendMessage(GetDlgItem(hDlg, IDC_COMMAND), CB_GETCURSEL, 0, 0);
				index = GetCommandIndex(index, gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
				if (index != -1)
				{
					ComboBox_GetText(GetDlgItem(hDlg, IDC_COMMAND), command, MAXFILENAME);
					ShowCmdCheckControls(hDlg, index);
					EnableWindow(GetDlgItem(hDlg, IDC_RUN), (gProcessList.NumItems > 0) ? TRUE : FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_CMDINFO), ((gNumScript == 0 || (gNumScript > 0  && index > 0)) && gProcessList.NumItems > 0) ? TRUE : FALSE);
				}
				else
				{
					SetDlgItemText(hDlg, IDC_DESCRIPTION, L"");
					EnableWindow(GetDlgItem(hDlg, IDC_RUN), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_CMDINFO), FALSE);
				}
			}
			break;

		case IDC_BROWSEDUMP:
			OnBrowseDump(hDlg);
			break;

		case IDC_PLATFORM:
			if (HIWORD(wParam) == CBN_SELENDOK)
			{
				int index;
				index = SendMessage(GetDlgItem(hDlg, IDC_PLATFORM), CB_GETCURSEL, 0, 0);
				wchar_t platform[20];

				GetWindowText(GetDlgItem(hDlg, IDC_PLATFORM), platform, 20);
				UpdateCommandList(hDlg, platform);
				EnableWindow(GetDlgItem(hDlg, IDC_COMMAND), (gProcessList.NumItems > 0) ? TRUE : FALSE); // Show command list
				wcscpy_s(gPlatform, gPlatforms[index]);
			}
			break;

		case IDC_GETPROCESSLIST:
			if (gCommandRunning)
			{
				gCommandRunning = false;
				OnStopCommand(hDlg);
				SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Run"));

			}
			else
			{
				gCommandRunning = true;
				OnGetProcessList(hDlg);
				SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Stop"));
			}
			break;

		case IDC_RUN:
			if (gCommandRunning)
			{
				gCommandRunning = false;
				OnStopCommand(hDlg);
				SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Run"));
			}
			else
			{
				gCommandRunning = true;
				OnRunCommand(hDlg);
				SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Stop"));
			}
			break;

		case IDC_CMDINFO:
			if (gCommandRunning)
			{
				gCommandRunning = false;
				OnStopCommand(hDlg);
				SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Run"));
			}
			else
			{
				gCommandRunning = true;
				OnCommandInfo(hDlg);
				SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Stop"));
			}
			break;

		case IDC_CHECK1:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK1) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO1:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if(bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 0);
			else
				HideCmdValueControl(hDlg, cmdIndex, 0);
			break;

		case IDC_CHECK2:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK2) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO2:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if (bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 1);
			else
				HideCmdValueControl(hDlg, cmdIndex, 1);
			break;

		case IDC_CHECK3:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK3) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO3:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if (bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 2);
			else
				HideCmdValueControl(hDlg, cmdIndex, 2);
			break;

		case IDC_CHECK4:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK4) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO4:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if (bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 3);
			else
				HideCmdValueControl(hDlg, cmdIndex, 3);
			break;

		case IDC_CHECK5:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK5) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO5:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if (bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 4);
			else
				HideCmdValueControl(hDlg, cmdIndex, 4);
			break;

		case IDC_CHECK6:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK6) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO6:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if (bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 5);
			else
				HideCmdValueControl(hDlg, cmdIndex, 5);
			break;

		case IDC_CHECK7:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK7) == BST_UNCHECKED)
				bChecked = false;
		case IDC_RADIO7:
			cmdIndex = GetCommandIndex(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)), gPlatforms[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM))]);
			if (bChecked)
				ShowCmdValueControl(hDlg, cmdIndex, 6);
			else
				HideCmdValueControl(hDlg, cmdIndex, 6);
			break;

		case IDC_ABOUT:
			DialogBox(ghInstance, MAKEINTRESOURCE(IDD_ABOUT), hDlg, AboutDlgProc);
			break;

		case IDCANCEL:
		case IDOK:
			Cleanup(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return FALSE;
}

//********************************
//AboutDlgProc
//
//Dialog box handler for the about window
//********************************
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	wchar_t  tstr[500];

	switch (iMsg)
	{
	case WM_INITDIALOG:
		swprintf_s(tstr, 500, L"%s %s Build: %s", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
		SetDlgItemText(hDlg, IDC_VERSION, tstr);

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		case IDOK:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/********************************/
//InitDialog
//
//Called just after the dialog is created
/********************************/
int InitDialog(HWND hDlg, PWSTR lpCmdLine)
{
	wchar_t	CfgFileName[MAXFILENAME + MAXDIRNAME];
	int		xoffset, yoffset, dlgWidth, dlgHeight;
	RECT	dlgRect;
	bool	bPlatformFound = false;

	//Allocate a buffer for the search results.
	ScriptResults = (wchar_t *)malloc(MAX_RESULT_BUFF_LEN*sizeof(wchar_t));
	if (!ScriptResults) {
		MessageBox(hDlg, L"Could not allocate enough RAM", ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
		return (NOK);
	}
	memset(ScriptResults, 0, MAX_RESULT_BUFF_LEN*sizeof(wchar_t));

	//Allocate a buffer for the search results.
	LastScriptResult = (wchar_t *)malloc(MAX_RESULT_BUFF_LEN*sizeof(wchar_t));
	if (!LastScriptResult) {
		MessageBox(hDlg, L"Could not allocate enough RAM", ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
		return (NOK);
	}
	memset(LastScriptResult, 0, MAX_RESULT_BUFF_LEN*sizeof(wchar_t));

	InitializeCriticalSection(&LogCritSection);

	hLogUpdated = CreateEvent(NULL, true, true, NULL);

	//center dialog
	//Get the current dialog size
	GetWindowRect(hDlg, &dlgRect);
	dlgWidth = dlgRect.right - dlgRect.left;
	dlgHeight = dlgRect.bottom - dlgRect.top;

	//Store current position of screen
	RECT		rect;
	GetClientRect(hDlg, &rect);
	cxOld = rect.right;
	cyOld = rect.bottom;

	//Get x & y start points based on screen size
	yoffset = GetSystemMetrics(SM_CYFULLSCREEN) / 2 - (dlgHeight / 2);
	xoffset = GetSystemMetrics(SM_CXFULLSCREEN) / 2 - (dlgWidth / 2);

	SetWindowPos(hDlg, HWND_TOP, xoffset, yoffset, dlgWidth, dlgHeight, 0);

	/*if (!SetProfileList(hDlg))
		return (NOK);*/

	InitDialogControls(hDlg);

	//Create the broswer 
	CreateHTMLControl(hDlg);

	wchar_t header[100] = L"<p style=\"font-family:Lucida Console;font-size:70%;\">";
	htmlWindow->WriteToHtmlDoc(header);

	if (wcslen((wchar_t*)lpCmdLine))
	{
		if (lpCmdLine[0] == '\"')
			wcsncpy_s(gFileName, &lpCmdLine[1], wcslen(lpCmdLine) - 2);
		else
			wcscpy_s(gFileName, lpCmdLine);

		if (!PathFileExists(gFileName))
			gFileName[0] = '\0';
	}

	if (gFileName[0] == '\0') //--image=C:\Volatility Workbench\mem.bin
	{
		if (!SearchImgFile(L"\\*.bin", gFileName))
			if (!SearchImgFile(L"\\*.raw", gFileName))
				if (!SearchImgFile(L"\\*.dmp", gFileName))
					SearchImgFile(L"\\*.mem", gFileName);
	}

	if (gFileName[0] != '\0')
	{
		SetDlgItemText(hDlg, IDC_FILE, gFileName);

		wcscpy_s(CfgFileName, gFileName);
		wchar_t *pExt = wcsrchr(CfgFileName, '.');
		if (pExt != NULL)
			wcscpy(pExt, L".cfg");

		// search to see if profile is in the list of supported profiles
		if (ReadConfigFile(CfgFileName))
		{ 
			int i;
			for (i = 0; i < NUM_PLATFORMS; i++)
			{
				if (wcsstr(gPlatform, gPlatforms[i])) // profile matched
				{
					wcscpy(gPlatform, gPlatforms[i]);

					SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_SETCURSEL, i, 0);

					UpdateCommandList(hDlg, gPlatforms[i]);

					break;
				}
			}
		}
		else
		{
			wcscpy(gPlatform, gPlatforms[0]);
			UpdateCommandList(hDlg, L"Windows");
			SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_SETCURSEL, 0, 0);
		}

		EnableControls(hDlg);
	}

	SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETMARQUEE, TRUE, 50);

	InitLog();

	return (OK);
}


void ChangeCurser(CURSOR_TYPE cursor)
{

	if (cursor == WAIT)
		Cursor = LoadCursor(NULL, IDC_WAIT);
	else
		Cursor = LoadCursor(NULL, IDC_ARROW);

	SetClassLong(ghMainWnd, GCL_HCURSOR, (LONG)Cursor);

	PostMessage(ghMainWnd, WM_SETCURSOR, (WPARAM)0, (LPARAM)0);
}

/********************************/
//OnSize
//
//	Function to move controls on window resize
/********************************/
void OnSize(HWND hwnd, int x, int y)
{

	int	cxDelta = 0;
	int	cyDelta = 0;

	// Get the x and y deltas
	cxDelta = x - cxOld;
	cyDelta = y - cyOld;

	// Resize or move the controls
	UpdateControl(hwnd, IDC_CLEARLOG, cxDelta, cyDelta, RS_MOVE);
	UpdateControl(hwnd, IDC_SAVE, cxDelta, cyDelta, RS_MOVE);
	UpdateControl(hwnd, IDC_COPYTOCLIPBOARD, cxDelta, cyDelta, RS_MOVE);
	UpdateControl(hwnd, IDC_ABOUT, cxDelta, cyDelta, RS_MOVE);
	UpdateControl(hwnd, IDOK, cxDelta, cyDelta, RS_MOVE);
	UpdateControl(hwnd, IDB_LOGO, cxDelta, 0, RS_MOVE);
	UpdateControl(hwnd, IDC_STATUS, 0, cyDelta, RS_MOVE);
	UpdateControl(hwnd, IDC_PROGRESS, 0, cyDelta, RS_MOVE);

	

	//backup coordinates for next move
	cxOld = x;
	cyOld = y;

	//Redraw
	InvalidateRect(hwnd, NULL, FALSE);
}

/********************************/
//OnMaxMinInfo
//
//Stop dialog being too small
/********************************/
int OnMaxMinInfo (HWND hwnd, LPMINMAXINFO WinInfo)
 {

	WinInfo->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
	WinInfo->ptMinTrackSize.y = MIN_WINDOW_HEIGHT;
	return 0;
}

//********************************
//OnBrowseDump
//
//Extract process list from memory dump file
//********************************
void OnBrowseDump(HWND hDlg)
{
	wchar_t	CfgFileName[MAX_PATH];

	if (!PopFileLoadDlg(hDlg, gFileName, L"Memory Dump Files (*.bin;*.raw;*.dmp;*.mem)\0*.bin;*.raw;*.dmp;*.mem\0Any File\0*.*\0", L"Select memory dump file"))
		return;

	InitDialogControls(hDlg);

	gProcessList.NumItems = 0;
	gPlatform[0] = '\0';
	gKDBG_Addr[0] = '\0';

	SetDlgItemText(hDlg, IDC_FILE, gFileName);

	wcscpy(CfgFileName, gFileName);
	wchar_t *pExt = wcsrchr(CfgFileName, '.');
	if (pExt != NULL)
		wcscpy(pExt, L".cfg");

	// search to see if profile is in the list of supported profiles
	if (ReadConfigFile(CfgFileName))
	{
		int i;
		for (i = 0; i < NUM_PLATFORMS; i++)
		{
			if (wcsstr(gPlatform, gPlatforms[i])) // profile matched
			{
				wcscpy(gPlatform, gPlatforms[i]);

				SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_SETCURSEL, i, 0);

				UpdateCommandList(hDlg, gPlatforms[i]);

				break;
			}
		}
	}
	else
	{
		wcscpy(gPlatform, gPlatforms[0]);
		UpdateCommandList(hDlg, L"Windows");
		SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_SETCURSEL, 0, 0);
	}

	EnableControls(hDlg);
}

//********************************
//OnGetProcessList
//
//Extract process list from memory dump file
//********************************
void OnGetProcessList(HWND hDlg)
{

	_beginthread(GetProcessesThread, 0, hDlg);
}

void OnRunCommand(HWND hDlg)
{
	_beginthread(RunCommandThread, 0, hDlg);
}

void OnCommandInfo(HWND hDlg)
{
	_beginthread(CommandInfoThread, 0, hDlg);
}

BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;

	GetWindowThreadProcessId(hwnd, &dwID);

	if (dwID == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}

void OnStopCommand(HWND hDlg)
{
	EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)gProcessInfo.dwProcessId);

	// Wait on the handle. If it signals, great. If it times out,
	// then you kill it.
	if (WaitForSingleObject(gProcessInfo.hProcess, 10000) != WAIT_OBJECT_0)
		TerminateProcess(gProcessInfo.hProcess, 0);
	CloseHandle(gProcessInfo.hThread);
	CloseHandle(gProcessInfo.hProcess);

	EnableControls(hDlg);
}


void OnCopyToClipboard(HWND hwnd, wchar_t *szData)
{
	OpenClipboard(hwnd);
	EmptyClipboard();
	size_t size = (wcslen(szData) + 1)*sizeof(wchar_t);
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hg){
		CloseClipboard();
		return;
	}
	memcpy((wchar_t*)GlobalLock(hg), szData, size);
	GlobalUnlock(hg);
	SetClipboardData(CF_UNICODETEXT, hg);
	CloseClipboard();
	GlobalFree(hg);
}

void OnClearLog(HWND hwnd)
{
	wchar_t header[100] = L"<p style=\"font-family:Lucida Console;font-size:70%;\">";
	htmlWindow->WriteToHtmlDoc(header);
	htmlWindow->WriteToHtmlDoc(header);
}

void OnSaveToFile(HWND hwnd)
{

	wchar_t	szTime[50] = { L'\0' };
	FILE	*fp;
	int		ret = 0;

	GetPersonalFolder(fullpath);
	wcscat(fullpath, szDefaultLogName);
	PopFileInitialize(hwnd);
	ret = PopFileSaveDlg(hwnd, fullpath, &szDefaultLogName[1], L"TXT Files (*.TXT)\0*.txt\0\0", L"txt", L"Select a log file name");
	if (ret == 0)
		return;

	//Open file
	fp = _wfopen(fullpath, L"wt, ccs=UTF-8");
	if (fp == NULL)
		return;

	//Write out log
	fwprintf(fp, L"4xWays Log file  -  http://www.4xWays.com\r\n");
	fwprintf(fp, L"==================================================================\r\n");
	fwprintf(fp, L"4xWays Workbench Version: %s Build %s\r\n", PROGRAM_VERSION, PROGRAM_BUILD);
	fwprintf(fp, L"4xWays Version: %s\r\n", 4xWays_VER);

	time_t		lCurrentTime;
	time(&lCurrentTime);											
	swprintf_s(szTime, _T("Log Date: %s\r\n"), _wctime(&lCurrentTime));
	fwprintf(fp, L"%s", szTime);
	
	fwprintf(fp, L"%s\r\n", ScriptResults);
	fflush(fp);
	fclose(fp);
}


//********************************
//CreateHTMLControl
//
//Create a HTML control in the dialog window
//********************************
void CreateHTMLControl(HWND hDlg)
{
	RECT dlgRect;
	GetClientRect(hDlg, &dlgRect);

	htmlWindow = new CWebCtrl(hDlg, ghInstance, SW_SHOW, WEBCTRL_OFFSET_X, WEBCTRL_OFFSET_Y, dlgRect.right - dlgRect.left - WEBCTRL_WIDTHOFFSET_X, dlgRect.bottom - dlgRect.top - WEBCTRL_HEIGHTOFFSET_X);
}

void DisplayHTMLResult()
{
	htmlWindow->AppendToHtmlDoc(HTMLResults);
	SetEvent(hLogUpdated);
}

bool ReadConfigFile(wchar_t *szCfgFileName)
{
	wifstream in;
	wstring str;
	int ProcessCount = 0;
	bool bPlatformFound = false;

	InitConfig();

	in.open(szCfgFileName);
	if (!in)
		return false;

	//locale utf8{ "en_us.UTF-8" };
	in.imbue(locale(std::locale::empty(), new codecvt_utf8<wchar_t>));

	while (!in.eof()) {
		while (getline(in, str)) {
			std::wstring ParamName;
			std::wstring Value;
			ParamName = str.substr(0, str.find(L"="));

			if (ParamName.find(L"PLATFORM") != std::string::npos)
			{
				Value = str.substr(str.find(L"=") + 1, str.find(L"\r") - str.find(L"=") - 1);
				wcscpy(gPlatform, Value.c_str());
				bPlatformFound = true;
			}
			else if (!ParamName.compare(L"PROCESS"))
			{
				Value = str.substr(str.find(L"=") + 1, str.find(L",") - str.find(L"=") - 1);
				wcscpy(gProcessList.Name[ProcessCount], Value.c_str());
				Value = str.substr(str.find(L",") + 1, str.length() - str.find(L",") - 1);
				wcscpy(gProcessList.Value[ProcessCount], Value.c_str());
				ProcessCount++;

			}

		}
	}

	in.close();

	gProcessList.NumItems = ProcessCount;
	return bPlatformFound;
}

void SaveConfigFile(wchar_t *szCfgFileName)
{
	FILE	*fp;
	fp = _wfopen(szCfgFileName, L"wt, ccs=UTF-8");
	if (fp == NULL)
		return;

	fwprintf(fp, L"PLATFORM=%s\n", gPlatform);
	//fwprintf(fp, L"KDBG=%s\n", gKDBG_Addr);
	for (int i = 0; i < gProcessList.NumItems; i++)
	{
		fwprintf(fp, L"PROCESS=%s,%s\n", gProcessList.Name[i], gProcessList.Value[i]);
	}
	fclose(fp);
}

void InitConfig(void)
{
	gPlatform[0] = '\0';
	gKDBG_Addr[0] = '\0';
	InitProcessList();
}

void InitProcessList(void)
{
	gProcessList.NumItems = 0;
	for (int i = 0; i < MAX_NUM_PROCESS; i++)
	{
		gProcessList.Name[i][0] = '\0';
		gProcessList.Value[i][0] = '\0';
	}
}

bool IsValidNumber(const wchar_t* pszText, bool allowMinus)
{
	if (allowMinus && ('-' == *pszText))
		++pszText;
	while (*pszText != '\0')
	{
		if (!isdigit(*pszText))
			return false;
		++pszText;
	}
	return true;
}

bool IsValidHex(const wchar_t* pszText, bool allowMinus)
{
	if (allowMinus && ('-' == *pszText))
		++pszText;
	while (*pszText != '\0')
	{
		if (!isxdigit(*pszText))
			return false;
		++pszText;
	}
	return true;
}

#define READ_SIZE	64

void ReadStandardOutputThread(LPVOID lpParameter)   // thread data
{
	HANDLE	StdOutFromSearch;

	char	buffer[READ_SIZE + 1];
	char	line[2048];
	wchar_t	*pwcsLine;
	DWORD	nBytesRead;
	int		LastErr;
	bool	bLog = 1;

	char* pLineWr = line;
	char* pCR = NULL;
	char* pLastCR = NULL;


	StdOutFromSearch = *(HANDLE*)lpParameter;

	

	//Read the input from the pipe.
	while (StdOutFromSearch)
	{
		if (!ReadFile(StdOutFromSearch, buffer, READ_SIZE, &nBytesRead, NULL))
		{
			if (!ReadFile(StdOutFromSearch, buffer, READ_SIZE, &nBytesRead, NULL))
			{
				LastErr = GetLastError();
				if (LastErr == ERROR_BROKEN_PIPE || LastErr == ERROR_NOACCESS)
					break; // pipe done - normal exit path.
				else {
					MessageBox(ghMainWnd, L"Read error", ErrorCaption, MB_OK);
					break;
				}
			}
		}
		buffer[nBytesRead] = '\0';

		pCR = buffer;
		pLastCR = NULL;

		if (nBytesRead > 0)
		{
			while (pCR < buffer + nBytesRead)
			{
				pCR = strstr(pCR, "\r");
				if (pCR != NULL && (pCR + 1) < (buffer + nBytesRead) && *(pCR + 1) == '\n')
					pCR++;
				if (pCR)
				{
					if (pLastCR == NULL)
					{
						memcpy(pLineWr, buffer, pCR - buffer + 1);
						pLineWr += pCR - buffer;
					}
					else
					{
						memcpy(pLineWr, pLastCR, pCR - pLastCR + 1);
						pLineWr += pCR - pLastCR;
					}
					if (*pLineWr  == '\r')
					{
						pLineWr++;
						*pLineWr = '\n';
					}
					pLineWr++;
					*pLineWr = '\0';
					pLastCR = pCR + 1;
					pCR++;
					pLineWr = line;

					int nChars = MultiByteToWideChar(CP_ACP, 0, line, -1, NULL, 0);
					// allocate it
					pwcsLine = new WCHAR[nChars];
					MultiByteToWideChar(CP_ACP, 0, line, -1, (LPWSTR)pwcsLine, nChars);

					if (strstr(line, "Progress") != NULL)
					{
						EnterCriticalSection(&LogCritSection);
						if (wcslen(pwcsLine) > (_countof(gStrProgress) - 1))
							pwcsLine[_countof(gStrProgress) - 1] = 0;
						wcscpy_s(gStrProgress, _countof(gStrProgress), pwcsLine);
						LeaveCriticalSection(&LogCritSection);
					}
					else if (strlen(line) > 2)
					{
						EnterCriticalSection(&LogCritSection);

						wchar_t *pStr1 = (wchar_t*)realloc(LastScriptResult, (wcslen(LastScriptResult) + wcslen(pwcsLine) + 1) * sizeof(wchar_t));
						if (!pStr1)
							break;
						LastScriptResult = pStr1;
						wcscat(LastScriptResult, pwcsLine);
						AddLogEntry(pwcsLine, false, bLog ? true : false);
						LeaveCriticalSection(&LogCritSection);
					}

					delete[] pwcsLine;
				}
				else
				{
					if (pLastCR == NULL)
					{
						memcpy(pLineWr, buffer, nBytesRead);
						pLineWr += nBytesRead;
					}
					else
					{	
						memcpy(pLineWr, pLastCR, buffer + nBytesRead - pLastCR);
						pLineWr += buffer + nBytesRead - pLastCR;
					}
					break;
				}
			}
			
		}

	}

	_endthread();

}


void ReadErrorOutputThread(LPVOID lpParameter)   // thread data
{
	HANDLE	StdOutFromSearch;

	char	buffer[READ_SIZE + 1];
	char	line[2048];
	wchar_t* pwcsLine;
	DWORD	nBytesRead;
	int		LastErr;
	bool bLog = 1;

	char* pLineWr = line;
	char* pCR = NULL;
	char* pLastCR = NULL;


	StdOutFromSearch = *(HANDLE*)lpParameter;


	//Read the input from the pipe.
	while (StdOutFromSearch)
	{
		if (!ReadFile(StdOutFromSearch, buffer, READ_SIZE, &nBytesRead, NULL))
		{
			if (!ReadFile(StdOutFromSearch, buffer, READ_SIZE, &nBytesRead, NULL))
			{
				LastErr = GetLastError();
				if (LastErr == ERROR_BROKEN_PIPE || LastErr == ERROR_NOACCESS)
					break; // pipe done - normal exit path.
				else {
					MessageBox(ghMainWnd, L"Read error", ErrorCaption, MB_OK);
					break;
				}
			}
		}
		buffer[nBytesRead] = '\0';

		pCR = buffer;
		pLastCR = NULL;
		

		if (nBytesRead > 0)
		{
			while (pCR < buffer + nBytesRead)
			{
				pCR = strstr(pCR, "\r");
				if (pCR != NULL && (pCR + 1) < (buffer + nBytesRead) && *(pCR + 1) == '\n')
					pCR++;
				if (pCR)
				{
					if (pLastCR == NULL)
					{
						memcpy(pLineWr, buffer, pCR - buffer + 1);
						pLineWr += pCR - buffer;
					}
					else
					{
						memcpy(pLineWr, pLastCR, pCR - pLastCR + 1);
						pLineWr += pCR - pLastCR;
					}
					if (*pLineWr == '\r')
					{
						pLineWr++;
						*pLineWr = '\n';
					}
					pLineWr++;
					*pLineWr = '\0';
					pLastCR = pCR + 1;
					pCR++;
					pLineWr = line;

					int nChars = MultiByteToWideChar(CP_ACP, 0, line, -1, NULL, 0);
					// allocate it
					pwcsLine = new WCHAR[nChars];
					MultiByteToWideChar(CP_ACP, 0, line, -1, (LPWSTR)pwcsLine, nChars);

					if (strstr(line, "Progress") != NULL)
					{
						EnterCriticalSection(&LogCritSection);
						if (wcslen(pwcsLine) > (_countof(gStrProgress) - 1) )
							pwcsLine[_countof(gStrProgress) - 1] = 0;
						wcscpy_s(gStrProgress, _countof(gStrProgress), pwcsLine);

						// Change \t after percentage number to a percent sign.
						// more efficient than spliting string, appending new character and concating strings
						wchar_t* percentLocation = wcsstr(gStrProgress, L"\t");
						if(percentLocation != NULL)
							*percentLocation = '%';

						LeaveCriticalSection(&LogCritSection);
					}
					else if (strlen(line) > 2)
					{
						EnterCriticalSection(&LogCritSection);
						
						wchar_t* pStr1 = (wchar_t*)realloc(LastScriptResult, (wcslen(LastScriptResult) + wcslen(pwcsLine) + 1) * sizeof(wchar_t));
						if (!pStr1)
							break;
						LastScriptResult = pStr1;
						wcscat(LastScriptResult, pwcsLine);

						AddLogEntry(pwcsLine, false, bLog ? true : false);
						LeaveCriticalSection(&LogCritSection);
					}

					delete[] pwcsLine;
				}
				else
				{
					if (pLastCR == NULL)
					{
						memcpy(pLineWr, buffer, nBytesRead);
						pLineWr += nBytesRead;
					}
					else
					{
						memcpy(pLineWr, pLastCR, buffer + nBytesRead - pLastCR);
						pLineWr += buffer + nBytesRead - pLastCR;
					}
					break;
				}
			}
		}

	}

	_endthread();

}




bool RunVolatilityScript(wchar_t *CmdParameters)
{
	TCHAR*	skipOffset;
	

	HANDLE	StdOutFromSearch;
	HANDLE	ErrOutFromSearch;
	wchar_t	TmpPath[MAXFILENAME + MAXDIRNAME];
	DWORD	nBytesRead;
	DWORD	nBytesResult;
	DWORD	CurrBufOffset = 0;
	int		LastErr;
	FILE	*file;

	swprintf_s(TmpPath, L"%s\\%s", RunDirName, VOLATILITY_EXE);
	
	if (!PathFileExists(TmpPath))
	{
		MessageBox(ghMainWnd, L"Failed to find Volatility executable in the application directory", ErrorCaption, MB_OK);
		return false;
	}

	//Build file name for temp file in current directory
	swprintf_s(TmpPath, L"\"%s\\%s\" %s", RunDirName, VOLATILITY_EXE, CmdParameters);
	AddLogEntry(TmpPath, true, true);

	wcscpy(LastScriptResult, L"\r\nPlease wait, this may take a few minutes.\r\n");
	AddLogEntry(LastScriptResult, false, false);

	AddLogEntry(L"\r\n", false, true);

	// Launch the Volatility executable with it output (stdout)
	// redirected into a pipe that can be read by this process
	SpawnWithRedirect(TmpPath, &gProcessInfo, &StdOutFromSearch, &ErrOutFromSearch);

	HANDLE hThreadSO;
	HANDLE hThreadSE;

	hThreadSO = (HANDLE)_beginthread(ReadStandardOutputThread, 0, &StdOutFromSearch);
	hThreadSE = (HANDLE)_beginthread(ReadErrorOutputThread, 0, &ErrOutFromSearch);

	WaitForSingleObject(hThreadSO, INFINITE);
	WaitForSingleObject(hThreadSE, INFINITE);

	AddLogEntry(L"\r\n******* End of command output ******\r\n", true, true);

	if (StdOutFromSearch)
		CloseHandle(StdOutFromSearch);

	if (ErrOutFromSearch)
		CloseHandle(ErrOutFromSearch);

	return true;
}

void InitDialogControls(HWND hDlg)
{
	EnableWindow(GetDlgItem(hDlg, IDC_GETPROCESSLIST), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_COMMAND), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_CMDINFO), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_RUN), FALSE);

	for (int i = 0; i < sizeof(gControlsList) / sizeof(gControlsList[0]); i++)
	{
		EnableWindow(GetDlgItem(hDlg, gControlsList[i]), FALSE);
		ShowWindow(GetDlgItem(hDlg, gControlsList[i]), SW_HIDE);
	}

	SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_ADDSTRING, 0, (LPARAM)L"Windows");
	SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_ADDSTRING, 0, (LPARAM)L"Linux");
	SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_ADDSTRING, 0, (LPARAM)L"Mac");
	SendDlgItemMessage(hDlg, IDC_PLATFORM, CB_SETCURSEL, 0, 0);

	SetDlgItemText(hDlg, IDC_DESCRIPTION, L"In order to run a command:\r\n1- Browse an image file\r\n2- Get/Refresh process list\r\n3- Select a command from the list\r\n4- Enter command parameters\r\n5- Run command");

}

void ShowCmdCheckControls(HWND hDlg, int cmdIndex)
{
	//int cmdIndex;
	//bool bCommand = false;
	//cmdIndex = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND)); //
	//if (cmdIndex == 0)
	//	return;

	//if (cmdIndex == gNumScript + 1)
	//	return;

	//if (cmdIndex > gNumScript)
	//{
	//	bCommand = true;
	//	cmdIndex -= (gNumScript + 2);
	//}
	//else
	//	cmdIndex -= 1;

	for (int i = 0; i < sizeof(gControlsList) / sizeof(gControlsList[0]); i++)
	{
		EnableWindow(GetDlgItem(hDlg, gControlsList[i]), FALSE);
		ShowWindow(GetDlgItem(hDlg, gControlsList[i]), SW_HIDE);
	}

	RECT dlgRect;
	GetClientRect(hDlg, &dlgRect);

	//htmlWindow->Resize(WEBCTRL_OFFSET_X, WEBCTRL_OFFSET_Y, dlgRect.right - dlgRect.left - WEBCTRL_WIDTHOFFSET_X, dlgRect.bottom - dlgRect.top - WEBCTRL_HEIGHTOFFSET_X);

	switch (gCommands[cmdIndex].nParams)
	{
	case 0:
		html_resize_offset = 0;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
		
	case 1:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP1), SW_SHOW);
		html_resize_offset = 0;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 2:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP2), SW_SHOW);
		html_resize_offset = 10;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 3:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP3), SW_SHOW);
		html_resize_offset = 20;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 4:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP4), SW_SHOW);
		html_resize_offset = 40;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 5:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP5), SW_SHOW);
		html_resize_offset = 65;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 6:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP6), SW_SHOW);
		html_resize_offset = 90;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 7:
	case 8:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP7), SW_SHOW);
		html_resize_offset = 120;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	case 9:
	case 10:
		ShowWindow(GetDlgItem(hDlg, IDC_GROUP8), SW_SHOW);
		html_resize_offset = 140;
		PostMessage(ghMainWnd, WM_SIZE, (WPARAM)0, (LPARAM)(((dlgRect.bottom - dlgRect.top) << 16) | (WPARAM)(dlgRect.right - dlgRect.left)));
		break;
	}

	

	for (int i = 0; i < gCommands[cmdIndex].nParams; i++)
	{
		SetDlgItemText(hDlg, gCommands[cmdIndex].params[i].nCheckDlgID, gCommands[cmdIndex].params[i].pszLabelText);
		
		SendMessage(GetDlgItem(hDlg, gCommands[cmdIndex].params[i].nCheckDlgID), BM_SETCHECK, gCommands[cmdIndex].params[i].bChecked, 0);
		if (gCommands[cmdIndex].params[i].bChecked)
			ShowCmdValueControl(hDlg, cmdIndex, i);
		EnableWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[i].nCheckDlgID), TRUE);
		ShowWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[i].nCheckDlgID), SW_SHOW);
	}

	SetDlgItemText(hDlg, IDC_DESCRIPTION, gCommands[cmdIndex].pszDescription);
}

void EnableCmdCheckControls(HWND hDlg, int cmdIndex)
{
	for (int i = 0; i < gCommands[cmdIndex].nParams; i++)
	{
		if (IsDlgButtonChecked(hDlg, gCommands[cmdIndex].params[i].nCheckDlgID) == BST_CHECKED)
			EnableCmdValueControl(hDlg, cmdIndex, i);
		EnableWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[i].nCheckDlgID), TRUE);
	}
}

void EnableControls(HWND hDlg)
{
	int index;
	wchar_t command[MAXFILENAME];

	ComboBox_GetText(GetDlgItem(hDlg, IDC_COMMAND), command, MAXFILENAME);

	EnableWindow(GetDlgItem(hDlg, IDC_BROWSEDUMP), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_GETPROCESSLIST), TRUE);
	SendMessage(GetDlgItem(hDlg, IDC_GETPROCESSLIST), WM_SETTEXT, 0, (gProcessList.NumItems > 0) ? (LPARAM)_T("Refresh Process List") : (LPARAM)_T("Get Process List"));
	EnableWindow(GetDlgItem(hDlg, IDC_COMMAND), (gProcessList.NumItems > 0) ? TRUE : FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_PROCESSLIST), (gProcessList.NumItems > 0) ? TRUE : FALSE);
	
	index = SendMessage(GetDlgItem(hDlg, IDC_COMMAND), CB_GETCURSEL, 0, 0);
	index = GetCommandIndex(index, gPlatform);

	if ((gNumScript == 0 || (gNumScript > 0 && index > 0)))
		EnableWindow(GetDlgItem(hDlg, IDC_CMDINFO), (command[0] != '-' && gProcessList.NumItems > 0) ? TRUE : FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_RUN), (command[0] != '-' && gProcessList.NumItems > 0) ? TRUE : FALSE);

	EnableWindow(GetDlgItem(hDlg, IDC_CLEARLOG), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_SAVE), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_COPYTOCLIPBOARD), TRUE);

	EnableCmdCheckControls(hDlg, index);
	
}

void DisableControls(HWND hDlg)
{
	EnableWindow(GetDlgItem(hDlg, IDC_BROWSEDUMP), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_GETPROCESSLIST), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_CMDINFO), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_COMMAND), FALSE);

	for (int i = 0; i < sizeof(gControlsList) / sizeof(gControlsList[0]); i++)
	{
		EnableWindow(GetDlgItem(hDlg, gControlsList[i]), FALSE);
	}

	EnableWindow(GetDlgItem(hDlg, IDC_CLEARLOG), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_SAVE), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_COPYTOCLIPBOARD), FALSE);
}

void ShowCmdValueControl(HWND hDlg, int cmdIndex, int paramIndex)
{
	wchar_t szTemp[MAX_VALUE_LEN] = { '\0' };

	switch (gCommands[cmdIndex].params[paramIndex].ValueType)
	{
	case VALUETYPE_NOVALUE:
		break;
	case VALUETYPE_EDITBOX:
		SetDlgItemText(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID, L"");
		break;
	case VALUETYPE_COMBOBOX:

		SendDlgItemMessage(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID, CB_RESETCONTENT, 0, 0);
		for (int i = 0; i < gCommands[cmdIndex].params[paramIndex].ValueList->NumItems; i++) {
			swprintf(szTemp, L"%s (%s)", gCommands[cmdIndex].params[paramIndex].ValueList->Name[i], gCommands[cmdIndex].params[paramIndex].ValueList->Value[i]);
			SendDlgItemMessage(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID, CB_ADDSTRING, 0, (LPARAM)szTemp);
		}
		SendDlgItemMessage(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID, CB_SETCURSEL, 0, 0);
		ShowWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), SW_SHOW);
		break;
	}

	if (gCommands[cmdIndex].params[paramIndex].ValueType != VALUETYPE_NOVALUE)
	{
		EnableWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), TRUE);
		ShowWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), SW_SHOW);
	}

	if (gCommands[cmdIndex].params[paramIndex].CheckType == CHECKTYPE_RADIOBUTTON)
	{
		for (int i = 0; i < gCommands[cmdIndex].nParams; i++)
		{
			if (i == paramIndex)
				continue;

			if (gCommands[cmdIndex].params[i].ValueType == VALUETYPE_NOVALUE)
				continue;

			if (gCommands[cmdIndex].params[i].CheckType != CHECKTYPE_RADIOBUTTON)
				continue;

			EnableWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[i].nValueDlgID), FALSE);
			ShowWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[i].nValueDlgID), SW_HIDE);
		}
	}
}

void EnableCmdValueControl(HWND hDlg, int cmdIndex, int paramIndex)
{
	switch (gCommands[cmdIndex].params[paramIndex].ValueType)
	{
	case VALUETYPE_NOVALUE:
		break;
	default:
		EnableWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), TRUE);
		ShowWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), SW_SHOW);
		break;
	}
}

void HideCmdValueControl(HWND hDlg, int cmdIndex, int paramIndex)
{
	if (gCommands[cmdIndex].params[paramIndex].ValueType != VALUETYPE_NOVALUE)
	{
		EnableWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), FALSE);
		ShowWindow(GetDlgItem(hDlg, gCommands[cmdIndex].params[paramIndex].nValueDlgID), SW_HIDE);
	}
}

int AddScriptsToCommandList(HWND hDlg);

void UpdateCommandList(HWND hDlg, wchar_t* platform)
{
	SendDlgItemMessage(hDlg, IDC_COMMAND, CB_RESETCONTENT, 0, 0);

	AddScriptsToCommandList(hDlg);

	SendDlgItemMessage(hDlg, IDC_COMMAND, CB_ADDSTRING, 0, (LPARAM)L"----- Volatility Commands -----");

	for (int i = 0; i < TOT_COMMANDS; i++)
	{
		if (wcsstr(gCommands[i].pszPlatform, platform))
		{
			SendDlgItemMessage(hDlg, IDC_COMMAND, CB_ADDSTRING, 0, (LPARAM)gCommands[i].pszCommand);
		}
	}
	SendDlgItemMessage(hDlg, IDC_COMMAND, CB_SETCURSEL, 0, 0);

}


int GetCommandIndex(int index, wchar_t* platform)
{
	int i = 0;

	if (index == 0)
		return -1;

	if (gNumScript > 0)
	{
		if (index == gNumScript + 1)
			return -1;

		if (index > gNumScript)
		{
			index -= (gNumScript + 2);
			for (i = 0; i < TOT_COMMANDS; i++)
				if (wcsstr(gCommands[i].pszPlatform, platform))
					break;
			return i + index;
		}
		else
			return 0;
	}
	else
		return index;

}


//********************************
//Cleanup
//
//
//********************************
void Cleanup(HWND hDlg)
{
	free(LastScriptResult);
	free(ScriptResults);
	
	CloseHandle(hLogUpdated);

	if (htmlWindow != NULL)
		delete(htmlWindow);
}


/********************************/
//UpdateControl
//
//	Move or resize a control (without any relative reference to any other controls on the window)
//		PT_MOVE:		Move the control down and across, keeping the  same size
//		PT_MOVE_DOWN:	Move the control down only (not across), keeping the  same size
//		PT_MOVE_ACROSS:	Move the control across only (not down), keeping the  same size
//		PT_RESIZE:		Move the controls bottom right co-ordinate, keeping the top left the same
//		PT_RESIZE_HALFY:Move the controls bottom right co-ordinate (x and 0.5y), keeping the top left the same
/********************************/
void UpdateControl(HWND hDlg, int iControl, int cxDelta, int cyDelta, int iType)
{
	HWND	hImageFrame;
	RECT	rcImageFrame;
	POINT	ptImageFrameTL;
	POINT	ptImageFrameBR;
	int		cxFrameWidth	= 0;
	int		cyFrameHeight	= 0;

	hImageFrame = GetDlgItem(hDlg, iControl);

	GetWindowRect(hImageFrame, &rcImageFrame);

	// Get current top/left coordinates
	ptImageFrameTL.x = rcImageFrame.left;
	ptImageFrameTL.y = rcImageFrame.top;
	ScreenToClient(hDlg, &ptImageFrameTL);
	// Get current bottom/right coordinates
	ptImageFrameBR.x = rcImageFrame.right;
	ptImageFrameBR.y = rcImageFrame.bottom;
	ScreenToClient(hDlg, &ptImageFrameBR);
	// Work out width/height
	cxFrameWidth  = ptImageFrameBR.x - ptImageFrameTL.x;
	cyFrameHeight = ptImageFrameBR.y - ptImageFrameTL.y;	

	if (iType == RS_MOVE)							//Move the control down and across, keeping the  same size
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x + cxDelta, 
					ptImageFrameTL.y + cyDelta, 
					cxFrameWidth,  
					cyFrameHeight, 
					FALSE);
	else if (iType == RS_RESIZE)					//Move the controls bottom right co-ordinate, keeping the top left the same
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x, 
					ptImageFrameTL.y, 
					cxFrameWidth  + cxDelta,  
					cyFrameHeight + cyDelta, 
					FALSE);
	/*
	else if (iType == RS_RESIZE_HALFY)					//Move the controls bottom right co-ordinate, keeping the top left the same
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x, 
					(long) ((float)ptImageFrameTL.y/2.0), 
					cxFrameWidth  + cxDelta,  
					cyFrameHeight + cyDelta, 
					FALSE);
					*/
	else if (iType == RS_MOVE_DOWN)					//Move the control down only (not across), keeping the  same size
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x, 
					ptImageFrameTL.y + cyDelta, 
					cxFrameWidth,  
					cyFrameHeight, 
					FALSE);
	else if (iType == RS_MOVE_ACROSS)				//Move the control across only (not down), keeping the  same size
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x + cxDelta, 
					ptImageFrameTL.y, 
					cxFrameWidth,  
					cyFrameHeight, 
					FALSE);
	else if (iType == RS_MOVE_ACROSS_SIZE_DOWN) //Move across and resize down
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x + cxDelta, 
					ptImageFrameTL.y, 
					cxFrameWidth,  
					cyFrameHeight + cyDelta, 
					FALSE);
	else if (iType == RS_MOVE_DOWN_SIZE_ACROSS) //Move across and resize down
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x, 
					ptImageFrameTL.y + cyDelta, 
					cxFrameWidth + cxDelta,  
					cyFrameHeight, 
					FALSE);
	else if (iType == RS_MOVE_DOWN_AND_ACROSS) //Move across and resize down
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x + cxDelta, 
					ptImageFrameTL.y + cyDelta, 
					cxFrameWidth,  
					cyFrameHeight, 
					FALSE);
	else if (iType == RS_SIZE_ACROSS) //Move across and resize down
		MoveWindow( hImageFrame, 
					ptImageFrameTL.x, 
					ptImageFrameTL.y, 
					cxFrameWidth + cxDelta,  
					cyFrameHeight, 
					FALSE);

}

bool SearchImgFile(wchar_t *pszExt, wchar_t *pszFileName)
{
	WIN32_FIND_DATA		stFindData;
	HWND imgFileList = GetDlgItem(ghMainWnd, IDC_FILE);
	HANDLE		hRes;

	wcscpy_s(gFileName, RunDirName);
	wcscat_s(gFileName, pszExt); //L"\\*.bin"

	hRes = FindFirstFile(gFileName, &stFindData);

	// Find how many files are in the directory
	if (INVALID_HANDLE_VALUE != hRes)
	{

		swprintf_s(pszFileName, MAXFILENAME + MAXDIRNAME, L"%s\\%s", RunDirName, stFindData.cFileName);
		SetDlgItemText(ghMainWnd, IDC_FILE, gFileName);
		// Cleanup
		FindClose(hRes);
		return true;
	}

	gFileName[0] = '\0';
	return false;
}

int AddScriptsToCommandList(HWND hDlg)
{
	WIN32_FIND_DATA		stFindData;
	HWND				imgFileList = GetDlgItem(ghMainWnd, IDC_FILE);
	HANDLE				hRes;
	wchar_t				pszFileName[MAXFILENAME + MAXDIRNAME] = L"";

	wcscpy_s(pszFileName, RunDirName);
	wcscat_s(pszFileName, L"\\*.vws");

	gNumScript = 0;

	hRes = FindFirstFile(pszFileName, &stFindData);

	// Find how many files are in the directory
	if (INVALID_HANDLE_VALUE != hRes)
	{
		
		SendDlgItemMessage(hDlg, IDC_COMMAND, CB_ADDSTRING, 0, (LPARAM)L"----- User Scripts -----");
		SendDlgItemMessage(hDlg, IDC_COMMAND, CB_ADDSTRING, 0, (LPARAM)stFindData.cFileName);
		swprintf_s(pszFileName, MAXFILENAME + MAXDIRNAME, L"%s\\%s", RunDirName, stFindData.cFileName);
		wcscpy_s(gScriptFileNames[gNumScript], pszFileName);
		gNumScript++;
		while (FindNextFile(hRes, &stFindData) != 0) 
		{
			swprintf_s(pszFileName, MAXFILENAME + MAXDIRNAME, L"%s\\%s", RunDirName, stFindData.cFileName);
			wcscpy_s(gScriptFileNames[gNumScript], pszFileName);
			gNumScript++;
			SendDlgItemMessage(hDlg, IDC_COMMAND, CB_ADDSTRING, 0, (LPARAM)stFindData.cFileName);
		}
		// Cleanup
		FindClose(hRes);
		return 1;
	}

	return 0;
}

// You must free the result if result is non-NULL.
wchar_t *str_replace(wchar_t *orig, wchar_t *rep, wchar_t *with, UINT64 *len)
{
	wchar_t *result; // the return string
	wchar_t *ins;    // the next insert point
	wchar_t *tmp;    // varies
	UINT64 len_rep;  // length of rep (the string to remove)
	UINT64 len_with; // length of with (the string to replace rep with)
	UINT64 len_front; // distance between rep and end of last rep
	UINT64 count;    // number of replacements

	// sanity checks and initialization
	if (!orig || !rep)
		return NULL;
	len_rep = wcslen(rep);
	if (len_rep == 0)
		return NULL; // empty rep causes infinite loop during count
	if (!with)
		with = L"";
	len_with = wcslen(with);

	// count the number of replacements needed
	ins = orig;
	for (count = 0; tmp = wcsstr(ins, rep); ++count) {
		ins = tmp + len_rep;
	}

	*len = wcslen(orig) + (UINT32)(len_with - len_rep) * count + 1;
	tmp = result = (wchar_t *)malloc((*len)*sizeof(wchar_t));

	if (!result)
		return NULL;

	// first time through the loop, all the variable are set correctly
	// from here on,
	//    tmp points to the end of the result string
	//    ins points to the next occurrence of rep in orig
	//    orig points to the remainder of orig after "end of rep"
	while (count--) {
		ins = wcsstr(orig, rep);
		len_front = ins - orig;
		tmp = wcsncpy(tmp, orig, len_front) + len_front;
		tmp = wcscpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	wcscpy(tmp, orig);
	return result;
}

bool PopFileLoadDlg(HWND hwnd,
	wchar_t *pstrFileName,
	wchar_t *filter,
	wchar_t *dialog_title)
{
	OPENFILENAME	ofn;

	ZeroMemory(&gFileName, sizeof(gFileName));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = pstrFileName;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = dialog_title; 
	ofn.lpstrFilter = filter;
	ofn.nMaxFile = MAX_PATH;

	if (GetOpenFileName(&ofn))
		return true;

	return false;
}

//Local vars
static OPENFILENAME		ofn;

//********************************
//PopFileSaveDlg
//
//
//********************************
BOOL PopFileSaveDlg(HWND hwnd,
	wchar_t *pstrFileName,
	wchar_t *pstrTitleName,
	wchar_t *filter,
	wchar_t *def_extension,
	wchar_t *dialog_title)
{

	ofn.lpstrFilter = filter;
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = pstrFileName;
	ofn.lpstrFileTitle = pstrTitleName;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = dialog_title;
	ofn.lpstrDefExt = def_extension;

	return GetSaveFileName(&ofn);
}

//--------------------------------------------------------------------
// GetPersonalFolder
// 
// Returns the User local application data folder
//		eg. 
//
// If the directory does not exist, create it
//
//--------------------------------------------------------------------
void GetPersonalFolder(wchar_t * szPath)
{
	wchar_t	szPersonalDir[MAXDIRNAME + MAXFILENAME] = { L'\0' };

	// User Personal folder
	SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szPersonalDir);
	wcscpy(szPath, szPersonalDir);

	wcscat(szPath, L"\\");
	wcscat(szPath, COMPANY_NAME_SHORT);
	_wmkdir(szPath);

	wcscat(szPath, L"\\");
	wcscat(szPath, PROGRAM_NAME);
	_wmkdir(szPath);
}

//********************************
//PopFileInitialize
//
//
//********************************
void PopFileInitialize(HWND hwnd)
{

	int				StructSize;

	StructSize = OPENFILENAME_SIZE_VERSION_400;

	ofn.lStructSize = StructSize;
	ofn.hwndOwner = hwnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = NULL;          // File name + path, Set in Open and Close functions
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrFileTitle = NULL;          // File name, Set in Open and Close functions
	ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = 0;             // Set in Open and Close functions
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
}



//**********************************************************
// GetProcessesThread
//
//
// 
//**********************************************************
void GetProcessesThread(LPVOID lpParameter)   // thread data
{
	HWND	hDlg = (HWND)lpParameter;
	wchar_t	str[200];
	wchar_t	*pOffset;
	wchar_t	KDBG_Offset[20];
	wchar_t	CmdParameters[512];
	bool	bPIDFound = false;
	int		numProcesses = 0;
	int	index;

	DisableControls(hDlg);
	EnableWindow(GetDlgItem(hDlg, IDC_RUN), TRUE);
	
	ChangeCurser(WAIT);
	SetWindowText(hDlg, L"4xWays (Command is running ...)");

	index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM));

	if(index == 0) // Windows
		swprintf_s(CmdParameters, L"-f \"%s\" windows.pslist.PsList", gFileName);
	else if (index == 1) // Linux
		swprintf_s(CmdParameters, L"-f \"%s\" linux.pslist.PsList ", gFileName);
	else // Mac
		swprintf_s(CmdParameters, L"-f \"%s\" mac.pslist.PsList ", gFileName);
	
	if (!RunVolatilityScript(CmdParameters))
		goto clean_up;
	
	InitProcessList();

	if (wcsstr(gPlatform, L"Windows"))
		pOffset = wcsstr(LastScriptResult, L"PID");
	else
		pOffset = wcsstr(LastScriptResult, L"Pid");

	if (!pOffset)
	{
		MessageBox(hDlg, L"Failed to obtain process list\r\nThis could be due to selecting a wrong platform", ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
		goto clean_up;
	}

	pOffset = wcsstr(pOffset, L"\n");
	pOffset = wcsstr((wchar_t *)(pOffset + 1), L"\n");

	gProcessList.Name[numProcesses][0] = '\0';

	while (1)
	{
		swscanf(pOffset, L"%99s", str);
		pOffset = wcsstr(pOffset, str) + wcslen(str) + 1;
		if (isdigit(str[0]))
		{
			bPIDFound = true;
			wcscpy_s(gProcessList.Value[numProcesses], str);
			swscanf(pOffset, L"%s", str); // Skip PPID
			pOffset = wcsstr(pOffset, str) + wcslen(str) + 1;
			swscanf(pOffset, L"%s", str); // Read ImageFileName
			wcscat_s(gProcessList.Name[numProcesses], str);

			numProcesses++;
			gProcessList.Name[numProcesses][0] = '\0';
		}
		//else
		//	MessageBox(hDlg, L"Failed to obtain process list\r\nThis could be due to selecting a wrong platform", ErrorCaption, MB_ICONEXCLAMATION | MB_OK);

		
		pOffset = wcsstr(pOffset, L"\n");
		if (pOffset == NULL)
			break;
		if (*(pOffset + 1) == '\0')
			break;

	}

	if(numProcesses == 0)
	{
		MessageBox(hDlg, L"Failed to obtain process list\r\nThis could be due to selecting a wrong platform", ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
		goto clean_up;
	}

	gProcessList.NumItems = numProcesses;

	wchar_t	CfgFileName[MAX_PATH];

	wcscpy_s(CfgFileName, gFileName);

	wchar_t *pExt = wcsrchr(CfgFileName, '.');
	if (pExt != NULL)
		wcscpy(pExt, L".cfg");

	SaveConfigFile(CfgFileName);


clean_up:

	EnableControls(hDlg);

	ChangeCurser(ARROW);
	ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), FALSE);
	SetWindowText(hDlg, L"4xWays");
	wcscpy_s(gStrProgress, L"");

	gCommandRunning = false;
	SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Run"));

	_endthread();
}

//**********************************************************
// CommandInfoThread
//
//
// 
//**********************************************************
void CommandInfoThread(LPVOID lpParameter)   // thread data
{
	HWND	hDlg = (HWND)lpParameter;
	wchar_t	str[50];
	wchar_t	*pOffset;
	wchar_t	KDBG_Offset[20];
	wchar_t	CmdParameters[256];
	bool	bPIDFound = false;
	int		numProcesses = 0;
	int		cmd_index;
	int		profile_index;


	DisableControls(hDlg);
	EnableWindow(GetDlgItem(hDlg, IDC_RUN), TRUE);

	ChangeCurser(WAIT);
	ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), TRUE);
	SetWindowText(hDlg, L"4xWays (Command is running ...)");
	wcscpy_s(gStrProgress, L"Command is running ...");

	profile_index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM));
	cmd_index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND));
	cmd_index = GetCommandIndex(cmd_index, gPlatforms[profile_index]);
	swprintf_s(CmdParameters, L"%s -h", gCommands[cmd_index].pszCommand);
	RunVolatilityScript(CmdParameters);

clean_up:

	EnableControls(hDlg);

	ChangeCurser(ARROW);
	SetWindowText(hDlg, L"4xWays");
	wcscpy_s(gStrProgress, L"");

	gCommandRunning = false;
	SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Run"));

	_endthread();
}

// trim from end of string (right)
inline std::wstring& rtrim(std::wstring& s)
{
	const wchar_t *ws = L" \t\n\r\f\v";
	s.erase(s.find_last_not_of(ws) + 1);
	return s;
}

// trim from beginning of string (left)
inline std::wstring& ltrim(std::wstring& s)
{
	const wchar_t *ws = L" \t\n\r\f\v";
	s.erase(0, s.find_first_not_of(ws));
	return s;
}

// trim from both ends of string (left & right)
inline std::wstring& trim(std::wstring& s)
{
	return ltrim(rtrim(s));
}

//**********************************************************
// RunCommandThread
//
//
// 
//**********************************************************
void RunCommandThread(LPVOID lpParameter)   // thread data
{
	HWND	hDlg = (HWND)lpParameter;
	int			cmd_index;
	int			profile_index;


	ChangeCurser(WAIT);
	ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), TRUE);
	SetWindowText(hDlg, L"4xWays (Command is running ...)");
	wcscpy_s(gStrProgress, L"Command is running ...");

	profile_index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PLATFORM));
	cmd_index =  ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND));
	cmd_index = GetCommandIndex(cmd_index, gPlatforms[profile_index]);

	if (cmd_index) 
	{

		int			process_index;
		int			value_index;
		wchar_t		CmdParameters[MAXDIRNAME + MAXFILENAME + MAXPARAMETERSLEN];
		wchar_t		Param[MAXPARAMETERSLEN];
		wchar_t		Value[MAXFILENAME];
		wchar_t		SavePath[MAXFILENAME + MAXDIRNAME] = L"";
		wchar_t		TmpPath[MAXDIRNAME];
		wchar_t		msg[100];

		process_index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PROCESSLIST));

		DisableControls(hDlg);

		swprintf_s(CmdParameters, L"-f \"%s\" ", gFileName);

		for (int i = 0; i < gCommands[cmd_index].nParams; i++)
		{
			if (!gCommands[cmd_index].params[i].bPluginParam && IsDlgButtonChecked(hDlg, gCommands[cmd_index].params[i].nCheckDlgID) == BST_CHECKED)
			{
				switch (gCommands[cmd_index].params[i].ValueType)
				{
				case VALUETYPE_NOVALUE:
					swprintf_s(Param, L"%s ", gCommands[cmd_index].params[i].pszParam);
					wcscat_s(CmdParameters, Param);
					break;

				case VALUETYPE_EDITBOX:
					ComboBox_GetText(GetDlgItem(hDlg, gCommands[cmd_index].params[i].nValueDlgID), Value, sizeof(Value));
					if (trim(wstring(Value)).length() == 0)
					{
						swprintf_s(msg, L"\"%s\" is empty", gCommands[cmd_index].params[i].pszLabelText);
						MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
						goto clean_up;
					}

					switch (gCommands[cmd_index].params[i].ValidationType)
					{
					case VALIDATION_DISABLED:
						break;
					case VALIDATION_DIGIT:
						if (!IsValidNumber(Value, false))
						{
							swprintf_s(msg, L"Invalid data entry in \"%s\", enter a number", gCommands[cmd_index].params[i].pszLabelText);
							MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
							goto clean_up;
						}
						break;
					case VALIDATION_HEXADDRESS:
						if (Value[0] != '0' || Value[1] != 'x' || !IsValidHex(&Value[2], false))
						{
							swprintf_s(msg, L"Invalid data entry in \"%s\", enter a HEX address", gCommands[cmd_index].params[i].pszLabelText);
							MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
							goto clean_up;
						}
						break;
					case VALIDATION_REGEX:
						break;
					case VALIDATION_DIR:
						swprintf_s(TmpPath, L"\"%s", Value);
						wcscpy_s(SavePath, Value);
						wcscpy_s(SavePath, TmpPath);

						int dirCreated = CreateDirectory(&TmpPath[1], NULL);
						if (dirCreated == ERROR_PATH_NOT_FOUND)
						{
							swprintf_s(msg, L"Invalid data entered in \"%s\", enter a directory name", gCommands[cmd_index].params[i].pszLabelText);
							MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
							goto clean_up;
						}
						else if (dirCreated == ERROR_ALREADY_EXISTS)
						{
							int a = 1;
						}
						wcscat_s(SavePath, L"\"");
						break;
					}

					swprintf_s(Param, L"%s %s ", gCommands[cmd_index].params[i].pszParam, SavePath);
					wcscat_s(CmdParameters, Param);
					break;

				case VALUETYPE_COMBOBOX:
					value_index = ComboBox_GetCurSel(GetDlgItem(hDlg, gCommands[cmd_index].params[i].nValueDlgID));
					swprintf_s(Param, L"%s %s ", gCommands[cmd_index].params[i].pszParam, gCommands[cmd_index].params[i].ValueList->Value[value_index]);
					wcscat_s(CmdParameters, Param);
					break;
				}
			}
		}

		swprintf_s(Param, L"%s ", gCommands[cmd_index].pszCommand);
		wcscat_s(CmdParameters, Param);

		for (int i = 0; i < gCommands[cmd_index].nParams; i++)
		{
			if (gCommands[cmd_index].params[i].bPluginParam && IsDlgButtonChecked(hDlg, gCommands[cmd_index].params[i].nCheckDlgID) == BST_CHECKED)
			{
				switch (gCommands[cmd_index].params[i].ValueType)
				{
				case VALUETYPE_NOVALUE:
					swprintf_s(Param, L"%s ", gCommands[cmd_index].params[i].pszParam);
					wcscat_s(CmdParameters, Param);
					break;

				case VALUETYPE_EDITBOX:
					ComboBox_GetText(GetDlgItem(hDlg, gCommands[cmd_index].params[i].nValueDlgID), Value, sizeof(Value));
					if (trim(wstring(Value)).length() == 0)
					{
						swprintf_s(msg, L"\"%s\" is empty", gCommands[cmd_index].params[i].pszLabelText);
						MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
						goto clean_up;
					}

					switch (gCommands[cmd_index].params[i].ValidationType)
					{
					case VALIDATION_DISABLED:
						break;
					case VALIDATION_DIGIT:
						if (!IsValidNumber(Value, false))
						{
							swprintf_s(msg, L"Invalid data entry in \"%s\", enter a number", gCommands[cmd_index].params[i].pszLabelText);
							MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
							goto clean_up;
						}
						break;
					case VALIDATION_HEXADDRESS:
						if (Value[0] != '0' || Value[1] != 'x' || !IsValidHex(&Value[2], false))
						{
							swprintf_s(msg, L"Invalid data entry in \"%s\", enter a HEX address", gCommands[cmd_index].params[i].pszLabelText);
							MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
							goto clean_up;
						}
						break;
					case VALIDATION_REGEX:
						break;
					case VALIDATION_DIR:
						swprintf_s(TmpPath, L"\"%s\\%s", RunDirName, Value);
						wcscpy_s(SavePath, Value);
						wcscpy_s(SavePath, TmpPath);

						int dirCreated = CreateDirectory(&TmpPath[1], NULL);
						if (dirCreated == ERROR_PATH_NOT_FOUND)
						{
							swprintf_s(msg, L"Invalid data entered in \"%s\", enter a directory name", gCommands[cmd_index].params[i].pszLabelText);
							MessageBox(hDlg, msg, ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
							goto clean_up;
						}
						else if (dirCreated == ERROR_ALREADY_EXISTS)
						{
							int a = 1;
						}
						wcscat_s(SavePath, L"\"");
						break;
					}

					swprintf_s(Param, L"%s %s ", gCommands[cmd_index].params[i].pszParam, SavePath);
					wcscat_s(CmdParameters, Param);
					break;

				case VALUETYPE_COMBOBOX:
					value_index = ComboBox_GetCurSel(GetDlgItem(hDlg, gCommands[cmd_index].params[i].nValueDlgID));
					swprintf_s(Param, L"%s %s ", gCommands[cmd_index].params[i].pszParam, gCommands[cmd_index].params[i].ValueList->Value[value_index]);
					wcscat_s(CmdParameters, Param);
					break;
				}
			}
		}

		RunVolatilityScript(CmdParameters);
	}
	else
	{
		wifstream	in;
		wstring		str;
		bool		bScriptHasPID = false;
		int			pid_index;
		UINT64		len;
		wchar_t		*wstr1, *wstr2, *wstr3;
		wchar_t		ImgFileName[MAXFILENAME + MAXDIRNAME];

		swprintf_s(ImgFileName, L"\"%s\" ", gFileName);
	
		bScriptHasPID = IsDlgButtonChecked(hDlg, gCommands[cmd_index].params[0].nCheckDlgID);
		int script_index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMMAND));
		pid_index = ComboBox_GetCurSel(GetDlgItem(hDlg, gCommands[cmd_index].params[0].nValueDlgID));

		in.open(gScriptFileNames[script_index - 1]);
		if (!in)
			goto clean_up;

		//locale utf8{ "en_us.UTF-8" };
		in.imbue(locale(std::locale::empty(), new codecvt_utf8<wchar_t>));

		while (!in.eof())
		{
			while (getline(in, str))
			{
				if (str.find(L"%PID") != std::string::npos)
				{
					bScriptHasPID = true;
					if (IsDlgButtonChecked(hDlg, gCommands[cmd_index].params[0].nCheckDlgID) == BST_UNCHECKED)
					{
						MessageBox(hDlg, L"Please select Process ID", ErrorCaption, MB_ICONEXCLAMATION | MB_OK);
						goto clean_up;
					}
					break;
					
				}
			}
		}

		in.close();
		in.open(gScriptFileNames[script_index - 1]);
		if (!in)
			goto clean_up;

		while (!in.eof())
		{
			while (getline(in, str))
			{
				if (gCommandRunning == false)
					goto clean_up;
				wstr1 = _wcsdup(str.c_str());
				wstr2 = str_replace(wstr1, L"%IMG", ImgFileName, &len);
				wstr3 = str_replace(wstr2, L"%PID", gProcessList.Value[pid_index], &len);
				RunVolatilityScript(wstr3);
				free(wstr1);
				free(wstr2);
				free(wstr3);
			}
		}

		in.close();
				
	}


clean_up:

	EnableControls(hDlg);

	ChangeCurser(ARROW);
	ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), FALSE);
	SetWindowText(hDlg, L"4xWays");

	wcscpy_s(gStrProgress, L"");

	gCommandRunning = false;
	SendMessage(GetDlgItem(hDlg, IDC_RUN), WM_SETTEXT, 0, (LPARAM)_T("Run"));

	_endthread();
}

void ReplaceAll(std::wstring *str, const std::wstring& from, const std::wstring& to) {
	size_t start_pos = 0;
	while ((start_pos = str->find(from, start_pos)) != std::wstring::npos) {
		str->replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
}

void AddLogEntry(wchar_t *pszLog, bool bDisplayTime, bool bAddToLog)
{
	time_t lCurrentTime;
	struct tm* tm_info;
	wchar_t timeStr[50];
	UINT64	len;
	wchar_t *szLogLine;
	wchar_t* pStr1;
	wchar_t* pStr2;

	UINT64 strlen = wcslen(pszLog);
	szLogLine = (wchar_t*)malloc((strlen * sizeof(wchar_t)) + sizeof(timeStr) + sizeof(wchar_t));
	if (!szLogLine)
		return;

	time(&lCurrentTime);

	if (bDisplayTime)
	{
		time(&lCurrentTime);
		swprintf_s(timeStr, _T("\r\nTime Stamp: %s"), _wctime(&lCurrentTime));
		wcscpy(szLogLine, timeStr);
		wcscat(szLogLine, L"\r\n");
		wcscat(szLogLine, pszLog);
	}
	else
		wcscpy(szLogLine, pszLog);

	if (bAddToLog)
	{
		pStr1 = (wchar_t*)realloc(ScriptResults, (wcslen(ScriptResults) + wcslen(szLogLine) + 1)*sizeof(wchar_t));
		if (!pStr1)
			return;
		ScriptResults = pStr1;
		wcscat(ScriptResults, szLogLine);
	}


	pStr1 = str_replace(szLogLine, L"\r\n", L"\r\n<br>", &len);
	if (!pStr1)
		return;

	pStr2 = str_replace(pStr1, L" ", L"&nbsp;", &len);
	if (!pStr2)
		return;

	free(pStr1);

	HTMLResults = str_replace(pStr2, L"	", L"&emsp;", &len);
	if (!HTMLResults)
		return;


	ResetEvent(hLogUpdated);
	PostMessage(ghMainWnd, WM_APP, (WPARAM)0, (LPARAM)0);

	
	WaitForMultipleObjects(1, &hLogUpdated, FALSE, 2000);

	free(HTMLResults);
}

void InitLog(void)
{
	time_t lCurrentTime;
	struct tm* tm_info;

	wchar_t str[512];

	HTMLResults = str;
	swprintf(HTMLResults, L"%s %s Build %s<br>%s<br>4xWays Version %s<br>%s<br>------------------------------------<br>", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD, WEBSITE, 4xWays_VER, 4xWays_WEBSITE);

	//ResetEvent(hLogUpdated);
	//PostMessage(ghMainWnd, WM_APP, (WPARAM)0, (LPARAM)0);
		
	//WaitForMultipleObjects(1, &hLogUpdated, FALSE, 10000);
	DisplayHTMLResult();
}