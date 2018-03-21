// InvisibleWnd.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "InvisibleWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInvisibleWnd

CInvisibleWnd::CInvisibleWnd()
{
	prev_child = NULL;
}

CInvisibleWnd::~CInvisibleWnd()
{
//	TRACE ("%.8X Deleting CInvisibleWnd\n", this);
}


UINT	RegisteredConfigMessageId = 0;

BEGIN_MESSAGE_MAP(CInvisibleWnd, CWnd)
	//{{AFX_MSG_MAP(CInvisibleWnd)
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE (RegisteredConfigMessageId, OnNewLogMessage)
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CInvisibleWnd message handlers

LRESULT CInvisibleWnd::OnNewLogMessage(WPARAM wParam, LPARAM lParam)
{
	//	Pass the log message to our child.  When the main window is hidden and
	//	we only have a tray icon, the main window is our child.  When the main
	//	window is showing we do not have a child.  However we still have the
	//	same name as the main window, and it is even bets whether we will get
	//	a log message or it will go directly to the child.  So save off the
	//	handle of the window we find and send to it.

	TRACE ("Invisible window got a registered message!\n");
	CWnd *child = GetWindow (GW_CHILD);
	if (child)
	{
		//	Do not save the CWnd pointer, it is temporary and will vanish.
		//	The actual window handle should remain viable as long as we are
		//	around.

		prev_child = child->m_hWnd;
	}
	if (prev_child)
	{
		//	Since we start out minimized we are pretty much guaranteed of
		//	having a child window at near startup, so it should be almost 
		//	impossible to have a null window handle.  However, there is a
		//	thin slice of time where we can maybe get a message before the
		//	child is first set...

		TRACE ("Invisible window sending message to child\n");
		return ::SendMessage (prev_child, RegisteredConfigMessageId, wParam, lParam);
	}
	else
	{
		TRACE ("Invisble window doesn't have a child!\n");
	}
	return 0;
}

void CInvisibleWnd::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	CWnd::OnWindowPosChanging(lpwndpos);

#ifdef IS_SMA
	CWnd *child = GetWindow (GW_CHILD);
	if (child)
		prev_child = child->m_hWnd;
	if (prev_child && (lpwndpos->flags == 0x47))//& SWP_SHOWWINDOW))
	{
		TRACE ("Got windowpos changing with %X on invisible window, trying to pass it on.\n",
				lpwndpos->flags);
		::SendMessage (prev_child, REQUEST_DISPLAY, 0, 0);
	}
#endif
}

BOOL CInvisibleWnd::OnQueryEndSession() 
{
	CWnd *child = GetWindow (GW_CHILD);
	if (child)
	{
		//	Do not save the CWnd pointer, it is temporary and will vanish.
		//	The actual window handle should remain viable as long as we are
		//	around.

		prev_child = child->m_hWnd;
	}
	if (prev_child)
	{
		//	Since we start out minimized we are pretty much guaranteed of
		//	having a child window at near startup, so it should be almost 
		//	impossible to have a null window handle.  However, there is a
		//	thin slice of time where we can maybe get a message before the
		//	child is first set...

		return ::SendMessage (prev_child, WM_QUERYENDSESSION, NULL, NULL ) != 0;
	}
	return TRUE;
}

void CInvisibleWnd::OnEndSession(BOOL bEnding) 
{
	CWnd *child = GetWindow (GW_CHILD);
	if (child)
	{
		//	Do not save the CWnd pointer, it is temporary and will vanish.
		//	The actual window handle should remain viable as long as we are
		//	around.

		prev_child = child->m_hWnd;
	}
	if (prev_child)
	{
		//	Since we start out minimized we are pretty much guaranteed of
		//	having a child window at near startup, so it should be almost 
		//	impossible to have a null window handle.  However, there is a
		//	thin slice of time where we can maybe get a message before the
		//	child is first set...

		::SendMessage (prev_child, WM_ENDSESSION, bEnding, NULL );
	}
	return;
}
