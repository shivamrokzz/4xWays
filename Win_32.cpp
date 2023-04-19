
#define _WIN32_IE 0x0501
#define _CRT_SECURE_NO_WARNINGS


#include <mshtml.h>
#include <mshtmcid.h> 
#include <stdio.h> 
#include <atlbase.h>
#include <atlconv.h>

#include "WebControl.h"


LPTSTR html_tag = (L"about:<html></html>");
char HtmlTag[][50] = {"zoom_sort","zoom_xml","zoom_query","zoom_per_page","zoom_and","zoom_metaform", "zoom_date"};
//********************************
//CWebContainer constructor
//
//********************************
CWebContainer::CWebContainer( HWND hwnd )
{
	m_cRef = 0;
	m_hwnd = hwnd;
	m_punk = NULL;
	
	SetRectEmpty( &m_rect );
	
	m_pEvent = new CWebEventSink();
	
	
	m_pEvent->AddRef();
}



CWebContainer::~CWebContainer()
{
	m_pEvent->Release();
	
	m_punk->Release();
}



void CWebContainer::add()
{
	CLSID clsid;
	
	long ret = CLSIDFromString( L"Shell.Explorer", &clsid );
	
	ret = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IUnknown, (PVOID *) &m_punk );
	
	IOleObject *pioo;
	
	ret =  m_punk->QueryInterface( IID_IOleObject, (PVOID *) &pioo );
	
	ret =  pioo->SetClientSite( this );
	
	pioo->Release();
	
	IPersistStreamInit *ppsi;
	
	ret = m_punk->QueryInterface( IID_IPersistStreamInit, (PVOID *) &ppsi );
	
	
	ppsi->InitNew();
	
	ppsi->Release();
	
	ConnectEvents();
	setVisible( true );
	
}



void CWebContainer::remove()
{
	IOleObject *pioo;
	
	m_punk->QueryInterface( IID_IOleObject, (PVOID *) &pioo );
	
	pioo->Close( OLECLOSE_NOSAVE );
	
	pioo->SetClientSite( NULL );
	
	pioo->Release();
	
	IOleInPlaceObject *pipo;
	
	m_punk->QueryInterface( IID_IOleInPlaceObject, (PVOID *) &pipo );
	
	pipo->UIDeactivate();
	
	pipo->InPlaceDeactivate();
	
	pipo->Release();
	
	DisconnectEvents();
}




void CWebContainer::setLocation( int x, int y, int cx, int cy )
{
	m_rect.left   = x;
	m_rect.top    = y;
	m_rect.right  = cx;
	m_rect.bottom = cy;
	
	IOleInPlaceObject *pipo;
	
	m_punk->QueryInterface( IID_IOleInPlaceObject, (PVOID *) &pipo );
	
	pipo->SetObjectRects( &m_rect, &m_rect );
	pipo->Release();
}



void CWebContainer::setVisible( bool bVisible )
{
	IOleObject *pioo;
	
	m_punk->QueryInterface( IID_IOleObject, (PVOID *) &pioo );
	
	if ( bVisible )
	{
		pioo->DoVerb( OLEIVERB_INPLACEACTIVATE, NULL, this, 0, m_hwnd, &m_rect );
		pioo->DoVerb( OLEIVERB_SHOW, NULL, this, 0, m_hwnd, &m_rect );
	}
	else
		pioo->DoVerb( OLEIVERB_HIDE, NULL, this, 0, m_hwnd, NULL );
	
	
	pioo->Release();
}



void CWebContainer::setFocus( bool bFocus )
{
	IOleObject *pioo;
	
	if ( bFocus )
	{
		m_punk->QueryInterface( IID_IOleObject, (PVOID *) &pioo );

		pioo->DoVerb( OLEIVERB_UIACTIVATE, NULL, this, 0, m_hwnd, &m_rect );
		pioo->Release();
	}
}


void CWebContainer::ConnectEvents()
{
	IConnectionPoint *pcp = GetConnectionPoint( DIID_DWebBrowserEvents2 );
	
	HRESULT  ret = pcp->Advise( m_pEvent, &m_eventCookie );
	pcp->Release();
}

void CWebContainer::DisconnectEvents()
{
	IConnectionPoint *pcp;
	
	pcp = GetConnectionPoint( DIID_DWebBrowserEvents2 );
	
	pcp->Unadvise( m_eventCookie );
	
	pcp->Release();
}
/**************************************************************************

  CWebContainer::GetConnectionPoint()
  
**************************************************************************/
IConnectionPoint* CWebContainer::GetConnectionPoint( REFIID riid )
{
	IConnectionPointContainer *pcpc;
	
	m_punk->QueryInterface( IID_IConnectionPointContainer, (PVOID *) &pcpc );
	
	IConnectionPoint *pcp;
	
	HRESULT ret = pcpc->FindConnectionPoint( riid, &pcp );
	
	pcpc->Release();
	
	return pcp;
}
/**************************************************************************

  CWebContainer: Retrives Web browser object
  
**************************************************************************/
HRESULT CWebContainer::getWeb( IWebBrowser2 **ppweb )
{
	return m_punk->QueryInterface( IID_IWebBrowser2, (PVOID *) ppweb );
}


/*********************
WebCtrl functions 

Constructor function
Initialise web control and creates the new window

  HWND hParent - web control is created as a child window to this parent
  int nCmdShow - whether to create the window as visible or hidden, use SW_SHOW or SW_HIDE
  int startx - starting x coord
  int starty - starting y coord 
  int width - starting width
  int height - starting height
*********************/
HFONT font;
CWebCtrl::CWebCtrl(HWND hParent, HINSTANCE m_hinst, int nCmdShow, int startx, int starty, int width, int height)
{
	m_cxScroll = GetSystemMetrics( SM_CXHSCROLL ) + 2;
	m_cyScroll = GetSystemMetrics( SM_CYVSCROLL ) + 2;
	
	InitWebCtrl();
	
	//to create a web page dynamically use about: eg _T("about: <html><h1>blah") 
	//or about: blank to be ready to write to it
	wchar_t m_szHTML[MAX_PATH]; 
	swprintf_s ( m_szHTML, MAX_PATH, _T("about: <html></html>"));
	
	m_hwndWebCtrl = CreateWindow(
		WC_WEBCTRL,
		m_szHTML,
		WS_CHILD,
		startx,		// initial x position
		starty,		// initial y position
		width,		// initial x size
		height,		// initial y size
		hParent,	// Parent window
		NULL,		// Menu
		m_hinst,	
		(LPVOID) this );		//Pass an instance of itself so we can use the callback function and access data memebers properly

	//font = CreateFont(12, 0, 0, 0, 300, false, false, false,
	//	DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	//SendMessage(m_hwndWebCtrl, WM_SETFONT, (WPARAM)font, TRUE);

	ShowWindow( m_hwndWebCtrl, nCmdShow );

	//font = CreateFont(12, 0, 0, 0, 300, false, false, false,
	//	DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	//SendMessage(m_hwndWebCtrl, WM_SETFONT, (WPARAM)font, TRUE);
	

}

/*********************
WebCtrl functions 

Destructor function
*********************/
CWebCtrl::~CWebCtrl()
{
	DestroyWindow(m_hwndWebCtrl);
	UninitWebCtrl();
}

//********************************
//GetHtml
//
//Get the HTML string and fill its value
//return number of bytes to move for next tag or 0 to exit
//********************************
int  CWebCtrl::GetHtml(char *SelectOption,char *value, bool metaform) {
	char *start = NULL,*end = NULL;
	int len, num;
	start = strstr(SelectOption,"selected");

	if (start != NULL) {
		if (!metaform) {
			start = start + strlen("selected>");
			end = strchr(start,'<');
			
			*(end ) = NULL;
			strcpy_s(value, sizeof(value),start);
		}
		else {
			start = strchr(start,'=');
			end = strchr(start,'>');
			len = end - SelectOption;
			num = abs(end - start - 1);
		
			
			strncpy(value, start + 1, num);
			value[num] = NULL;
			return abs(len);
		}
	}
	else 
		value = NULL;
	return 0;
	
}
//********************************
//ParseHTML
//
//Parse user input for the search.cgi executable from the document
//********************************
void CWebCtrl::ParseHTML (char* HiddenStr, char * SearchString, char * NumResults, int *AllWord, char * MetaString) {

	IHTMLDocument2 *pNewDoc = NULL;
	IDispatch *document_dispatch = NULL;
	IDispatch *disp;
	IHTMLElement  *item, *td;
	IHTMLElementCollection *all_element_collection, *tds;
	IHTMLElement2 *tbl2;
	long alen, tdlen;
	char name[50], value[50] = "", CommandName[50];
	BSTR  TdInput = SysAllocString( L"INPUT"), TdSelect = SysAllocString( L"SELECT"), itext, ClassBSTR;
	VARIANT dummy, AttributeValue, Name;
	HRESULT hr = m_pweb->get_Document(&document_dispatch);
	char NameC[MAX_SEARCH_STRING_LEN], ClassName[MAX_SEARCH_STRING_LEN], SelectOption[MAX_SEARCH_STRING_LEN];
	int InputAttribute = 0, SelectionFound = 0;
	
	
	if (SUCCEEDED(hr) && (document_dispatch != NULL)) 
	{
		// get the actual document interface
		hr = document_dispatch->QueryInterface(IID_IHTMLDocument,(LPVOID *) &pNewDoc);
		if (SUCCEEDED(hr) && pNewDoc != NULL) 
		{
			//Retrive the forms from document
			pNewDoc->get_forms(&all_element_collection);
			if (SUCCEEDED(hr) && (all_element_collection != NULL)) 
			{
				//Retrive the number of object in collection
				all_element_collection->get_length( &alen);
				//4 byte signed int
				dummy.vt = VT_I4;
				dummy.intVal = 0;

				//Get dispatch from collection
				all_element_collection->item( dummy, dummy, (IDispatch**)&disp);
				if (disp)
				{
					//Query interface for ihtmlelement				
					disp->QueryInterface( IID_IHTMLElement, (void**)&item);
					if (item){
						disp->Release();
						disp = 0L;
						item->QueryInterface( IID_IHTMLElement2, (void**)&tbl2);
						item->Release();
						item = 0L;

						if(tbl2){
							//Retrieves a collection of objects based on the specified element name
							tbl2->getElementsByTagName( TdInput, &tds);
							if (tds){
								tds->get_length( &tdlen);
								//Read input value
								for (int tri = 0; (tri < tdlen) && (InputAttribute < MAX_INPUT_TAG); tri++){
									dummy.intVal = tri;
									tds->item( dummy, dummy, &disp);
									if(disp){
										disp->QueryInterface(IID_IHTMLElement, (void**)&td);
										if(td){
											//Get the class name
											td->get_className(&ClassBSTR);
											if (ClassBSTR != NULL) {
												WideCharToMultiByte(CP_ACP, 0, (ClassBSTR), -1, ClassName, MAX_SEARCH_STRING_LEN, NULL, NULL);
											}
											//Get the attibute name
											td->getAttribute(L"name", 0, &Name);											
											if (Name.bstrVal != NULL) {
												WideCharToMultiByte(CP_ACP, 0, (Name.bstrVal), -1, NameC, MAX_SEARCH_STRING_LEN, NULL, NULL);
												//Read zoom sort and zoom_xml string
												if (!strcmp(NameC,HtmlTag[0])  || !strcmp(NameC,HtmlTag[1])){
													td->getAttribute(L"value", 0, &AttributeValue);
													if (AttributeValue.bstrVal) 
														WideCharToMultiByte(CP_ACP, 0, (AttributeValue.bstrVal), -1, value, MAX_SEARCH_STRING_LEN, NULL, NULL);
													else
														*value = NULL;
													if (!strcmp(NameC,HtmlTag[0]))//if its zoom_sort
														sprintf_s (CommandName, 50, "%s=%s",NameC,value);
													else
														sprintf_s (CommandName, 50, "&%s=%s",NameC,value);

													strcat_s(HiddenStr, MAX_SEARCH_STRING_LEN, CommandName);
												}
												//Read zoom query string
												else if (!strcmp(NameC,HtmlTag[2])){
													td->getAttribute(L"value", 0, &AttributeValue);
													if (AttributeValue.bstrVal) 
														WideCharToMultiByte(CP_ACP, 0, (AttributeValue.bstrVal), -1, SearchString, MAX_SEARCH_STRING_LEN, NULL, NULL);
													else
														SearchString = NULL;
												}
												//Read And button option
												else if (!strcmp(NameC,HtmlTag[4])){
													td->getAttribute(L"checked", 0, &AttributeValue);
													if (AttributeValue.boolVal )
														*AllWord = 1;
													else
														*AllWord = 0;
												}
												//read values in zoom_metaforms
												else if (!strncmp( ClassName, HtmlTag[5], 13)){

													td->getAttribute(L"name", 0, &Name);
													td->getAttribute(L"value", 0, &AttributeValue);
													if (AttributeValue.bstrVal) 
														WideCharToMultiByte(CP_ACP, 0, (AttributeValue.bstrVal), -1, value, sizeof(value), NULL, NULL);
													else
														strcpy_s(value,sizeof(value),"-1");

													WideCharToMultiByte(CP_ACP, 0, (Name.bstrVal), -1, name, sizeof(name), NULL, NULL);
													sprintf_s (CommandName, 50, "&%s=%s",name,value);
													strcat_s(MetaString,MAX_SEARCH_STRING_LEN, CommandName);
												}
												//read in  zoom_date*
												else if (!strncmp( NameC, HtmlTag[6], strlen(HtmlTag[6])))
												{

													td->getAttribute(L"name", 0, &Name);
													td->getAttribute(L"value", 0, &AttributeValue);
													if (AttributeValue.bstrVal) 
														WideCharToMultiByte(CP_ACP, 0, (AttributeValue.bstrVal), -1, value, sizeof(value), NULL, NULL);
													else
														strcpy_s(value,sizeof(value),"");

													WideCharToMultiByte(CP_ACP, 0, (Name.bstrVal), -1, name, sizeof(name), NULL, NULL);
													sprintf_s (CommandName, 50, "&%s=%s",name,value);
													strcat_s(MetaString,MAX_SEARCH_STRING_LEN, CommandName);
												}
												
											}
											td->Release();
											td = 0L;
										}
										disp->Release();
										disp = 0L;
									}
								}
								tds->Release();
								tds = 0L;
							}//End search for 'input' tag


							//Get value from 'select' tags
							tbl2->getElementsByTagName( TdSelect, &tds);
							if (tds){
								tds->get_length( &tdlen);
								for (int tri = 0; (tri < tdlen) && (!SelectionFound); tri++){
									dummy.intVal = tri;
									tds->item( dummy, dummy, &disp);
									if( disp){
										disp->QueryInterface( IID_IHTMLElement, (void**)&td);
										if(td){
											//Get the name
											td->getAttribute(L"name", 0, &Name);
											if (Name.bstrVal != NULL) {
												WideCharToMultiByte(CP_ACP, 0, (Name.bstrVal), -1, NameC, MAX_SEARCH_STRING_LEN, NULL, NULL);
											}
											//Get the class name
											td->get_className(&ClassBSTR);
											if (ClassBSTR != NULL) {
												WideCharToMultiByte(CP_ACP, 0, (ClassBSTR), -1, ClassName, MAX_SEARCH_STRING_LEN, NULL, NULL);
											}
											//Get the Html string
											td->get_innerHTML(&itext);
											if (itext != NULL) {
												WideCharToMultiByte(CP_ACP, 0, (itext), -1, SelectOption, MAX_SEARCH_STRING_LEN, NULL, NULL);
											}

											if (!strcmp(NameC,HtmlTag[3])) {
												GetHtml(SelectOption,NumResults,false);
											}
											else if (!strncmp(ClassName,HtmlTag[5],13)){
												char value[50], CommandName[50];
												unsigned int n = 0, len = 0;
												do {
													n = GetHtml(&SelectOption[len],value,true);
													if (n){
														sprintf_s (CommandName, 50, "&%s=%s",NameC,value);
														strcat_s(MetaString,MAX_SEARCH_STRING_LEN, CommandName);
														len = len + n;
													}
												}
												while (n && (len <strlen(SelectOption)));

											}
											td->Release();
											td = 0L;
										}
										disp->Release();
										disp = 0L;
									}
								}
								tds->Release();
								tds = 0L;
							}//End search for OPTION

							tbl2->Release();
							tbl2 = 0L;
						}
					}//if (item) ends
				}//if (disp) ends
			}//if all_element_collection ends
			pNewDoc->Release();
		}// if pNewDoc ends
		document_dispatch->Release();
	}
}



//using about: to create the page in one go is limited to 508 chars 
//(buffer size of that particualr variable in the navigate function i'm assuming)
//so use write to add a line of html to the created page
void CWebCtrl::WriteToHtmlDoc(wchar_t* buffer)
{
#define maxbuffer 250000	
	
	IHTMLDocument2 *document = NULL;
	IDispatch *document_dispatch = NULL;
	
	
	HRESULT hr = m_pweb->get_Document(&document_dispatch);
	if(document_dispatch == NULL)
		return;

	if (SUCCEEDED(hr) && (document_dispatch != NULL)) 
	{
		// get the actual document interface
		hr = document_dispatch->QueryInterface(IID_IHTMLDocument2,(void **)&document);

		if (document != NULL) 
		{
			//Close document to CLEAR the window before adding new text
			document->close();

			// construct text to be written to browser as SAFEARRAY
			SAFEARRAY *safe_array = SafeArrayCreateVector(VT_VARIANT,0,1);
			
			VARIANT	*variant;
			SafeArrayAccessData(safe_array,(LPVOID *)&variant);
			
			/*OLECHAR achW[maxbuffer]; 
			MultiByteToWideChar(CP_ACP, 0, buffer, -1, achW, maxbuffer);*/ 
			
			variant->vt      = VT_BSTR;
			variant->bstrVal = SysAllocString(buffer);	
			
			SafeArrayUnaccessData(safe_array);
			// write SAFEARRAY to browser document
			int ret = document->write(safe_array);
			if(ret != S_OK)
			{// if (->write != S_OK MessageBox(hwnd_main, GetLastErrorString(), "WebCtrl:Write error", MB_OK);
				//char msg[256];
				//sprintf( msg, "%s - %d", GetLastErrorString(), ret);
				//MessageBox(hwnd_main, msg, "WebCtrl:Write error", MB_OK);
			}


			// release document 
			document->Release();
			// release dispatch interface	
			document_dispatch->Release();
			ShowWindow(m_hwndWebCtrl, SW_SHOW);
			HRESULT hr = VariantClear(variant);
			hr = SafeArrayDestroy(safe_array);
		}
	}
	
	ShowWindow(m_hwndWebCtrl, SW_SHOW);
	
}

void CWebCtrl::AppendToHtmlDoc(wchar_t* buffer)
{
#define maxbuffer 250000
	
	
	IHTMLDocument2 *document = NULL;
	IDispatch *document_dispatch = NULL;
	
	HRESULT hr = m_pweb->get_Document(&document_dispatch);
	if(document_dispatch == NULL)
		return;
	
	if (SUCCEEDED(hr) && (document_dispatch != NULL)) 
	{
		// get the actual document interface
		hr = document_dispatch->QueryInterface(IID_IHTMLDocument2,(void **)&document);
		
		if (document != NULL) 
		{
			// construct text to be written to browser as SAFEARRAY
			SAFEARRAY *safe_array = SafeArrayCreateVector(VT_VARIANT,0,1);
			
			VARIANT	*variant;
			SafeArrayAccessData(safe_array,(LPVOID *)&variant);
			
			//OLECHAR achW[maxbuffer]; 
			//MultiByteToWideChar(CP_ACP, 0, buffer, -1, achW, maxbuffer); 
			
			variant->vt      = VT_BSTR;
			variant->bstrVal = SysAllocString(buffer);	
			
			SafeArrayUnaccessData(safe_array);
			
			// write SAFEARRAY to browser document
			int ret = document->write(safe_array);
			if(ret != S_OK)
			{// if (->write != S_OK MessageBox(hwnd_main, GetLastErrorString(), "WebCtrl:Write error", MB_OK);
				//char msg[256];
				//sprintf( msg, "%s - %d", GetLastErrorString(), ret);
				//MessageBox(hwnd_main, msg, "WebCtrl:Write error", MB_OK);
			}

			// this is the trick!
			// take the body element from document...
			//
			IHTMLElement *pBody = NULL;
			hr = document->get_body(&pBody);
			// from body we can get element2 interface,
			// which allows us to do scrolling
			IHTMLElement2 *pElement = NULL;
			hr = pBody->QueryInterface(IID_IHTMLElement2, (void**)&pElement);
			// now we are ready to scroll
			long real_scroll_height;
			//pElement->put_scrollTop(1000000);
			pElement->get_scrollHeight(&real_scroll_height);
			pElement->put_scrollTop(real_scroll_height + 1000);

			//pElement->Release();
			//pBody->Release();
			// release document 
			document->Release();
			// release dispatch interface	
			document_dispatch->Release();
			ShowWindow(m_hwndWebCtrl, SW_SHOW);
			HRESULT hr = VariantClear(variant);
			hr = SafeArrayDestroy(safe_array);
		}
	}
	
	ShowWindow(m_hwndWebCtrl, SW_SHOW);
	//SendMessage(m_hwndWebCtrl, WM_VSCROLL, (WPARAM)SB_PAGEDOWN, NULL);
}

//Navigate to the specified document
void CWebCtrl::Navigate( LPTSTR psz )
{
   if ( !psz )
      return;

   if ( !*psz )
      return;
   
   int len = lstrlen( psz ) + 1;
   
   //WCHAR *pszW;   
   //pszW = new WCHAR[len];
   //MultiByteToWideChar( CP_ACP, 0, psz, -1, pszW, len );
   
   VARIANT v;
   VariantInit( &v );
   
   v.vt = VT_BSTR;
   
   v.bstrVal = SysAllocString( psz );
   
   m_pweb->Navigate2( &v, NULL, NULL, NULL, NULL );
   
   //delete []pszW;
}

void CWebCtrl::SetVisible(bool visible)
{
	m_pContainer->setVisible(visible);

}


//Copy selected text in web browser window to the clipboard
void CWebCtrl::CopySelection()
{
m_pweb->ExecWB(OLECMDID_COPY ,OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL); //omfg this works better than all the other code i wrote dammit

}

//Resize the IE window
void CWebCtrl::Resize(int x, int y, int width, int height)
{
	MoveWindow(m_hwndWebCtrl, x, y, width, height, TRUE);
}

//Print document
//include a custom header and footer so we dont use the ie  "page x of x" header 

//Microsoft Knowledge Base Article - 267240 HOWTO: Print Custom Headers and Footers for a WebBrowser Control
void CWebCtrl::Print()
{
	   SAFEARRAYBOUND psabBounds[1];
	   
	   SAFEARRAY *psaHeadFoot;
	   
	   HRESULT hr = S_OK;
	   LPSTR sMem = NULL;
	   
	   VARIANT vHeadStr, vFootStr, vArg, vHeadTxtStream;
	   
	   VariantInit(&vHeadStr);
	   VariantInit(&vFootStr);
	   VariantInit(&vHeadTxtStream);

	   // Variables needed to send IStream header to print operation.
	   HGLOBAL hG = 0;
	   IStream *pStream= NULL;
	   IUnknown *pUnk = NULL;
	   ULONG lWrote = 0;
	   
	   long rgIndices;
	   
	   // Initialize header and footer parameters to send to ExecWB().
	   psabBounds[0].lLbound = 0;
	   psabBounds[0].cElements = 3;
	   psaHeadFoot = SafeArrayCreate(VT_VARIANT, 1, psabBounds);
	   if (NULL == psaHeadFoot) 
	   {
		   // Error handling goes here.
	   }

	   
	   // Argument 1: Header
	   vHeadStr.vt = VT_BSTR;
	   vHeadStr.bstrVal = SysAllocString(L" "); //can change these to whatever, maybe nothing for the header
	   if (vHeadStr.bstrVal == NULL) 
	   {
		   // Error handling goes here.
	   }
	   
	   //SysStringLen(BSTR bs);
	   
	   // Argument 2: Footer
	   vFootStr.vt = VT_BSTR;
	   vFootStr.bstrVal = SysAllocString(L" "); //and maybe have "Testlog hh:mm dd/mm/yyyy" or something customisable from a preferences menu
	   if (vFootStr.bstrVal == NULL) 
	   {
		   // Error handling goes here.
	   }
	   
	   // Argument 3: IStream containing header text. Outlook and Outlook 
	   // Express use this to print out the mail header. 	
	   if ((sMem = (LPSTR)CoTaskMemAlloc(MAX_HEADER_TEXT_LEN)) == NULL) 
	   {
		   // Error handling goes here.
	   }
	   // We must pass in a full HTML file here, otherwise this 
	   // gets turned into garbage corrupted when we print.
	   sprintf_s (sMem, MAX_HEADER_TEXT_LEN, "<html></html>");
	   
	   
	   // Allocate an IStream for the LPSTR that we just created.
	   hG = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, strlen(sMem));
	   if (hG == NULL) 
	   {
		   // Error handling goes here.
	   }
	   hr = CreateStreamOnHGlobal(hG, TRUE, &pStream);
	   if (FAILED(hr)) 
	   {
		   // Error handling goes here.
	   }
	   hr = pStream->Write(sMem, strlen(sMem), &lWrote);
	   if (SUCCEEDED(hr))
	   {
		   // Set the stream back to its starting position.
		   LARGE_INTEGER pos;
		   pos.QuadPart = 0;
		   pStream->Seek((LARGE_INTEGER)pos, STREAM_SEEK_SET, NULL);
		   hr = pStream->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk));
		   vHeadTxtStream.vt = VT_UNKNOWN;
		   vHeadTxtStream.punkVal = pUnk;
	   }
	   
	   rgIndices = 0;
	   SafeArrayPutElement(psaHeadFoot, &rgIndices, static_cast<void *>(&vHeadStr));
	   rgIndices = 1;
	   SafeArrayPutElement(psaHeadFoot, &rgIndices, static_cast<void *>(&vFootStr));
	   rgIndices = 2;
	   SafeArrayPutElement(psaHeadFoot, &rgIndices, static_cast<void *>(&vHeadTxtStream));
	   
	   // SAFEARRAY must be passed ByRef or else MSHTML transforms it into NULL.
	   VariantInit(&vArg);
	   vArg.vt = VT_ARRAY | VT_BYREF;
	   vArg.parray = psaHeadFoot;
	   
	   //call the print function with the new headers 
	   m_pweb->ExecWB(OLECMDID_PRINT ,OLECMDEXECOPT_PROMPTUSER, &vArg, NULL);
}

typedef struct CustomData_tag CustomData;
struct CustomData_tag {
	HFONT hFont;
	// ...

};

//Callback proc for webctrl window
LRESULT CWebCtrl::WebCtrlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{	
	LPCREATESTRUCT	pCreSt;
	CWebCtrl  *pThis = (CWebCtrl *) GetWindowLong(hwnd, GWL_USERDATA);
	CustomData	*pData = (CustomData*)GetWindowLongPtr(hwnd, 0);

	switch ( uMsg )
	{
	
	case WM_SETFONT:
		/*pData->hFont = (HFONT)wParam;
		InvalidateRect(hwnd, NULL, TRUE);*/
		break;
	case WM_NCCREATE :
		//get the object 'this' instance from lParam and store it with the Window info
		pCreSt = (LPCREATESTRUCT) lParam;
		pThis = (CWebCtrl *)(pCreSt->lpCreateParams);
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)pThis);
		break;
	case WM_CREATE:
		//Create the helper objects for the browser

		pThis->m_pContainer = new CWebContainer(hwnd );

		pThis->m_pContainer->AddRef();

		pThis->m_pContainer->add();

		pThis->m_pContainer->getWeb( &pThis->m_pweb );

		pThis->m_pweb->put_RegisterAsDropTarget(VARIANT_FALSE); //make sure you cant drop a html page in the window to navigate too (otherwise it crashes)

		CREATESTRUCT *pcs;

		pcs = (CREATESTRUCT *) lParam;

		pThis->Navigate((html_tag));
		break;
	
      case WM_SIZE:
		  {
			  //removed + m_cxScroll from loword(lparam) to give vertical scrollbar and  + m_cyScroll from hiword(lparam) to give horzontal scrollbar
			  //Add them back to hide the scroll bars
			  pThis->m_pContainer->setLocation( 0, 0, LOWORD(lParam), HIWORD(lParam));
			  
		  }
         break;

      case WM_DESTROY:

         pThis->m_pweb->Release();

         pThis->m_pContainer->remove();

         pThis->m_pContainer->Release();

         break;
		 
   }
	return  DefWindowProc(hwnd, uMsg, wParam, lParam );
}

bool WINAPI CWebCtrl::InitWebCtrl()
{

	int ret = OleInitialize(NULL);
	if( ret != S_OK && ret != S_FALSE  ) //call this instead of CoInitialize so the clipboard can be used with the browser
	{
		MessageBox(NULL, L"Error initialising OLE", L"Error", MB_OK);

	}

	WNDCLASS wc;
	
	HINSTANCE hinst = GetModuleHandle( NULL );
	
	if ( !GetClassInfo( hinst, WC_WEBCTRL, &wc ) )
	{
		memset( &wc, 0, sizeof(wc) );
		
		wc.style         = CS_DBLCLKS | CS_GLOBALCLASS | CS_NOCLOSE;
		wc.lpfnWndProc   = WebCtrlProc;
		wc.hInstance     = hinst;
		wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
		wc.lpszClassName = WC_WEBCTRL;
		
		return RegisterClass( &wc ) ? true : false;
	}
	

	return true;
}

bool WINAPI CWebCtrl::UninitWebCtrl()
{
   WNDCLASS wc;

   bool bResult = false;

   if ( GetClassInfo( GetModuleHandle( NULL ), WC_WEBCTRL, &wc ) )
      bResult = UnregisterClass( WC_WEBCTRL, wc.hInstance ) ? true : false;

   OleUninitialize( ); //instead of CoUninitialize();

   return bResult;
}
