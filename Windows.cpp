
#include <windows.h>
#include <stdio.h>
#include "4xWays.h"

void DisplayError(char *pszAPI);
void PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdErr, wchar_t *ProgName, PROCESS_INFORMATION *ProcessInfo);


HANDLE SpawnWithRedirect(wchar_t *ProgName, PROCESS_INFORMATION *ProcessInfo, HANDLE *hChildStdOut,
	HANDLE *hChildStdErr)
{
	HANDLE hOutputReadTmp1, hOutputReadTmp2;
	HANDLE hOutputRead, hErrorRead;
	HANDLE hErrorWrite, hOutputWrite;
	SECURITY_ATTRIBUTES sa;
	
	
	// Set up the security attributes struct.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	
	
	// Create the child output pipe.
	if (!CreatePipe(&hOutputReadTmp1,&hOutputWrite,&sa,0))
		DisplayError("CreatePipe");
	
	// Create new output read handle and the input write handles. Set
	// the Properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp1,
		GetCurrentProcess(),
		&hOutputRead, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		DisplayError("DupliateHandle");
	
	// Added by Hamid
	// Create the child output pipe.
	if (!CreatePipe(&hOutputReadTmp2, &hErrorWrite, &sa, 0))
		DisplayError("CreatePipe");

	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp2,
		GetCurrentProcess(),
		&hErrorRead, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		DisplayError("DupliateHandle");
	
	// Close inheritable copies of the handles you do not want to be
	// inherited.
	if (!CloseHandle(hOutputReadTmp1)) DisplayError("CloseHandle");
	if (!CloseHandle(hOutputReadTmp2)) DisplayError("CloseHandle");
	
	PrepAndLaunchRedirectedChild(hOutputWrite, hErrorWrite, ProgName, ProcessInfo);
		
	// Close pipe handles (do not continue to modify the parent).
	// You need to make sure that no handles to the write end of the
	// output pipe are maintained in this process or else the pipe will
	// not close when the child process exits and the ReadFile will hang.
	if (!CloseHandle(hOutputWrite)) DisplayError("CloseHandle");
	if (!CloseHandle(hErrorWrite)) DisplayError("CloseHandle");
	
	// Redirection is complete
	*hChildStdOut = hOutputRead;
	*hChildStdErr = hErrorRead;
	return (hOutputRead);
	
}


/////////////////////////////////////////////////////////////////////// 
// PrepAndLaunchRedirectedChild
// Sets up STARTUPINFO structure, and launches redirected child.
/////////////////////////////////////////////////////////////////////// 
void PrepAndLaunchRedirectedChild(HANDLE hChildStdOut,
								  HANDLE hChildStdErr,
								  wchar_t *ProgName, PROCESS_INFORMATION *ProcessInfo)
{
	STARTUPINFO			si;
	char				ErrorStr[512];
	
	// Set up the start up info struct.
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	// Hide the child window (must include STARTF_USESHOWWINDOW)
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdOutput = hChildStdOut;
	si.hStdInput  = NULL;
	si.hStdError  = hChildStdErr;
	
	
	if (!CreateProcess(NULL, ProgName, NULL, NULL, TRUE, CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, NULL, NULL, &si, ProcessInfo)) {
		sprintf_s (ErrorStr, 512, "CreateProcess failed\n%s",ProgName);
		DisplayError(ErrorStr);
		
	}
	//else {
	//	// Close any unnecessary handles.
	//	if (!CloseHandle(ProcessInfo->hThread)) DisplayError("CloseHandle");
	//	if (!CloseHandle(ProcessInfo.hProcess)) DisplayError("CloseHandle");
	//}
}



/////////////////////////////////////////////////////////////////////// 
// DisplayError
// Displays the error number and corresponding message.
/////////////////////////////////////////////////////////////////////// 
void DisplayError(char *pszAPI)
{
	LPVOID lpvMessageBuffer;
	wchar_t szPrintBuffer[512];
	
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpvMessageBuffer, 0, NULL);
	
	swprintf_s (szPrintBuffer, 512,
		L"ERROR: API    = %s\nError code = %d.\nMessage   = %s\n",
		pszAPI, GetLastError(), (char *)lpvMessageBuffer);
	
	MessageBox (ghMainWnd, szPrintBuffer, L"Error while starting search", MB_ICONERROR | MB_OK);

	LocalFree(lpvMessageBuffer);
}

