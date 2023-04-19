


#ifndef CNTREVNT_H
#define CNTREVNT_H


//#define _WIN32_IE 0x0501

#include <mshtmhst.h>
#include <mshtmdid.h>
#include <exdispid.h>
#include <stdio.h>
#include "VolatilityWorkbench.h"

static unsigned char AppUrlBlank[] = {'a', 0, 'b', 0, 'o', 0, 'u', 0, 't', 0, ':', 0, 'b', 0, 'l', 0, 'a', 0, 'n', 0, 'k', 0 ,
's', 0, 'e', 0, 'a', 0, 'r', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'g', 0, 'i', 0, '?', 0};
static unsigned char AppUrl[] = {'a', 0, 'b', 0, 'o', 0, 'u', 0, 't', 0, ':', 0,
's', 0, 'e', 0, 'a', 0, 'r', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'g', 0, 'i', 0, '?', 0};


class CWebEventSink : public IDispatch
{
public:

	CWebEventSink()
	{
		m_cRef = 0;
	}

	// *** IUnknown ***
	STDMETHOD(QueryInterface)( REFIID riid, PVOID *ppv )
	{
		if ( IsEqualIID( riid, IID_IDispatch ) )
			*ppv = (IDispatch *) this;
		else if ( IsEqualIID( riid, IID_IUnknown ) )
			*ppv = this;
		else
		{
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();

		return S_OK;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		return InterlockedIncrement( &m_cRef );
	}

	STDMETHOD_(ULONG, Release)(void)
	{
		ULONG cRef = InterlockedDecrement( &m_cRef );

		if ( cRef == 0 )
			delete this;

		return cRef;
	}

	// *** IDispatch ***
	STDMETHOD (GetIDsOfNames)( REFIID, OLECHAR **, unsigned int, LCID, DISPID *pdispid )
	{
		*pdispid = DISPID_UNKNOWN;
		return DISP_E_UNKNOWNNAME;
	}

	STDMETHOD (GetTypeInfo)( unsigned int, LCID, ITypeInfo ** )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (GetTypeInfoCount)( unsigned int * )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (Invoke)( DISPID dispid, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int * )
	{
		return S_OK;
	}

private:

	LONG m_cRef;
};

class CWebContainer : public IOleClientSite, 
	public IOleInPlaceSite,
	public IOleInPlaceFrame,
	public IOleControlSite,
	public IDispatch,
	public IDocHostUIHandler
{
public:

	CWebContainer( HWND );
	~CWebContainer();


	// *** IDocHostUIHandler ***
	//added this bit to allow for inheriting from IDocHostUIHandler so we can stop the right click menu
	//we only really want to change ShowContextMenu, the rest can return E_NOTIMPL
	//return E_NOTIMPL to get a right click menu in the html window

	HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID, POINT *ppt, 
		IUnknown *pcmdtReserved, IDispatch *pdispReserved)
	{
		//return S_OK;
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE ShowUI(DWORD dwID, 
		IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
		IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE HideUI()
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE UpdateUI()
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow __RPC_FAR *pUIWindow, BOOL fRameWindow)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpMsg,	const GUID *pguidCmdGroup, DWORD nCmdID)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch)
	{
		return E_NOTIMPL;
	}



	//This has  been modified to scan a URL when it is clicked to see if it is an external link
	//or an internal app link that needs to be process by the program
	HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
	{
		WCHAR	*src; 
		int		len=0;

		// Get length of URL
		src = pchURLIn;
		while ((*(src)++));
		--src;
		len = src - pchURLIn; 

		// See if the URL starts with an internal link like
		//"about:blanksearch.cgi?" or "about:search.cgi?"
		if ( _wcsnicmp(pchURLIn, (wchar_t *)&AppUrlBlank[0], 22) == 0 ||
			_wcsnicmp(pchURLIn, (wchar_t *)&AppUrl[0], 17) == 0)
		{
			//convert URL to char*
			WideCharToMultiByte(CP_ACP, 0, pchURLIn, -1,lastURL, MAX_URL_LEN, NULL, NULL);

			// Post a message to this window using WM_APP, and pass the required URL string.
			PostMessage(ghMainWnd, WM_APP, 0, 0);

			// Tell browser that we processed the URL
			*ppchURLOut = 0;
			return(S_OK);
		}
		else
		{
			//Allocate memory for the return URL
			*ppchURLOut = (OLECHAR*)CoTaskMemAlloc(4);
			char tmpUrl[MAX_URL_LEN];

			//Convert the clicked URL to char*
			WideCharToMultiByte(CP_ACP, 0, pchURLIn, -1,tmpUrl, MAX_URL_LEN, NULL, NULL);

			//make the return URL empty so the browser doesn't try to open the link
			MultiByteToWideChar(CP_ACP, 0," ", -1, *ppchURLOut, MAX_URL_LEN);

			if(strncmp(tmpUrl, "about:blank", 11) == 0)
			{			
				//Copy the modified URL for use by the main message loop
				//+11 to remove the "about:blank"
				strcpy_s(lastURL, MAX_URL_LEN, tmpUrl+11);			
			}
			else
				if(strcmp(tmpUrl, "about:search.cgi") == 0)
				{			
					// Post a message to this window using WM_APP+1, and pass the required URL string.
					strcpy_s(lastURL, MAX_URL_LEN, tmpUrl+6);
					PostMessage(ghMainWnd, WM_APP+1, 0, 0);

					return(S_OK);		
				}
				else
					if(strncmp(tmpUrl, "about:", 6) == 0)
					{			
						//Copy the modified URL for use by the main message loop
						//+6 to remove the "about:"
						strcpy_s(lastURL, MAX_URL_LEN, tmpUrl+6);			
					}
					else
					{
						//Copy the unmodified URL
						strcpy_s(lastURL, MAX_URL_LEN, tmpUrl);
					}

					//Post WM_APP message so we can handle the link
					PostMessage(ghMainWnd, WM_APP, 0, 0);
					return(S_OK);

		}

		// We don't need to modify the URL. Note: We need to set ppchURLOut to 0 if we don't
		// return an OLECHAR (buffer) containing a modified version of pchURLIn.
		*ppchURLOut = 0;
		return(S_FALSE);
	}

	HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject *pDO, IDataObject  **ppDORet)
	{
		return E_NOTIMPL;
	}

	HRESULT ModifyContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved) 
	{
		return S_OK;
	}

	HRESULT CustomContextMenu(POINT *ppt, IUnknown *pcmdtReserved) 
	{


		return S_OK;
	}



	// *** IUnknown ***
	STDMETHOD(QueryInterface)( REFIID riid, PVOID *ppv )
	{
		if ( IsEqualIID( riid, IID_IOleClientSite ) )
			*ppv = (IOleClientSite *) this;
		else if ( IsEqualIID( riid, IID_IOleInPlaceSite ) )
			*ppv = (IOleInPlaceSite *) this;
		else if ( IsEqualIID( riid, IID_IOleInPlaceFrame ) )
			*ppv = (IOleInPlaceFrame *) this;
		else if ( IsEqualIID( riid, IID_IOleInPlaceUIWindow ) )
			*ppv = ( IOleInPlaceUIWindow *) this;
		else if ( IsEqualIID( riid, IID_IOleControlSite ) )
			*ppv = (IOleControlSite *)this;
		else if ( IsEqualIID( riid, IID_IOleWindow ) )
			*ppv = this;
		else if ( IsEqualIID( riid, IID_IDispatch ) )
			*ppv = (IDispatch *) this;
		else if ( IsEqualIID( riid, IID_IUnknown ) )
			*ppv = this;
		else if ( IsEqualIID( riid, IID_IDocHostUIHandler ) ) //added to allow ID-cHostUIHandler, remove rtight click menu
			*ppv = (IDocHostUIHandler *) this;
		else
		{
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();

		return S_OK;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		return InterlockedIncrement( &m_cRef );
	}

	STDMETHOD_(ULONG, Release)(void)
	{
		ULONG cRef = InterlockedDecrement( &m_cRef );

		if ( cRef == 0 )
			delete this;

		return cRef;
	}

	// *** IOleClientSite ***
	STDMETHOD (SaveObject)()
	{
		return E_NOTIMPL;
	}

	STDMETHOD (GetMoniker)( DWORD, DWORD, LPMONIKER * )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (GetContainer)( LPOLECONTAINER * )
	{
		return E_NOINTERFACE;
	}

	STDMETHOD (ShowObject)()
	{
		return S_OK;
	}

	STDMETHOD (OnShowWindow)( BOOL )
	{
		return S_OK;
	}

	STDMETHOD (RequestNewObjectLayout)()
	{
		return E_NOTIMPL;
	}

	// *** IOleWindow ***
	STDMETHOD (GetWindow)( HWND *phwnd )
	{
		*phwnd = m_hwnd;
		return S_OK;
	}

	STDMETHOD (ContextSensitiveHelp)( BOOL )
	{
		return E_NOTIMPL;
	}

	// *** IOleInPlaceSite ***
	STDMETHOD (CanInPlaceActivate)(void)
	{
		return S_OK;
	}

	STDMETHOD (OnInPlaceActivate) (void)
	{
		return S_OK;
	}

	STDMETHOD (OnUIActivate) (void)
	{
		return S_OK;
	}

	STDMETHOD (GetWindowContext)(
		IOleInPlaceFrame    **ppFrame, 
		IOleInPlaceUIWindow **ppIIPUIWin,
		LPRECT                prcPosRect,
		LPRECT                prcClipRect,
		LPOLEINPLACEFRAMEINFO pFrameInfo )
	{
		*ppFrame = (IOleInPlaceFrame *) this;
		*ppIIPUIWin = NULL;

		RECT rc;

		GetClientRect( m_hwnd, &rc );

		prcPosRect->left   = 0;
		prcPosRect->top    = 0;
		prcPosRect->right  = rc.right;
		prcPosRect->bottom = rc.bottom;

		CopyRect( prcClipRect, prcPosRect );

		pFrameInfo->cb             = sizeof(OLEINPLACEFRAMEINFO);
		pFrameInfo->fMDIApp        = FALSE;
		pFrameInfo->hwndFrame      = m_hwnd;
		pFrameInfo->haccel         = NULL;
		pFrameInfo->cAccelEntries  = 0;

		(*ppFrame)->AddRef();

		return S_OK;
	}

	STDMETHOD (Scroll)( SIZE )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (OnUIDeactivate)( BOOL )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (OnInPlaceDeactivate)(void)
	{
		return S_OK;
	}

	STDMETHOD (DiscardUndoState)(void)
	{
		return E_NOTIMPL;
	}

	STDMETHOD (DeactivateAndUndo)(void)
	{
		return E_NOTIMPL;
	}

	STDMETHOD (OnPosRectChange)( LPCRECT )
	{
		return S_OK;
	}

	// *** IOleInPlaceUIWindow ***
	STDMETHOD (GetBorder)( LPRECT )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (RequestBorderSpace)( LPCBORDERWIDTHS )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (SetBorderSpace)( LPCBORDERWIDTHS )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (SetActiveObject)( IOleInPlaceActiveObject *, LPCOLESTR )
	{
		return E_NOTIMPL;
	}

	// *** IOleInPlaceFrame ***
	STDMETHOD (InsertMenus)( HMENU, LPOLEMENUGROUPWIDTHS )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (SetMenu)( HMENU, HOLEMENU, HWND )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (RemoveMenus)( HMENU )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (SetStatusText)( LPCOLESTR )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (EnableModeless)( BOOL )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (TranslateAccelerator)( LPMSG, WORD )
	{
		return S_OK;
	}

	// *** IOleControlSite ***
	STDMETHOD (OnControlInfoChanged)(void)
	{
		return E_NOTIMPL;
	}

	STDMETHOD (LockInPlaceActive)( BOOL )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (GetExtendedControl)( IDispatch ** )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (TransformCoords)( POINTL *, POINTF *, DWORD )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (TranslateAccelerator)( LPMSG, DWORD )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (OnFocus)( BOOL )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (ShowPropertyFrame)(void)
	{
		return E_NOTIMPL;
	}

	// *** IDispatch ***
	STDMETHOD (GetIDsOfNames)( REFIID, OLECHAR **, unsigned int, LCID, DISPID *pdispid )
	{
		*pdispid = DISPID_UNKNOWN;
		return DISP_E_UNKNOWNNAME;
	}

	STDMETHOD (GetTypeInfo)( unsigned int, LCID, ITypeInfo ** )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (GetTypeInfoCount)( unsigned int * )
	{
		return E_NOTIMPL;
	}

	STDMETHOD (Invoke)( DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int * )
	{
		return DISP_E_MEMBERNOTFOUND;
	}

	void setLocation( int, int, int, int );
	void setVisible( bool );
	void setFocus( bool );
	void add();
	void remove();
	HRESULT getWeb( IWebBrowser2 ** );

private:

	void ConnectEvents();
	void DisconnectEvents();

	IConnectionPoint *GetConnectionPoint( REFIID );

	LONG           m_cRef;
	HWND           m_hwnd;
	IUnknown      *m_punk;
	RECT           m_rect;
	CWebEventSink *m_pEvent;
	DWORD          m_eventCookie;
};

#endif