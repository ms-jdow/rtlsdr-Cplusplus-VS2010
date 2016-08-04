// TrayIconManager.cpp : implementation file
//

#include "stdafx.h"
#include "RtlSdr_Catalog.h"
#include "TrayIconManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrayIconManager

IMPLEMENT_DYNCREATE( CTrayIconManager, CWinThread )

CTrayIconManager::CTrayIconManager( void )
{
	
}

CTrayIconManager::~CTrayIconManager( void )
{
}

BOOL CTrayIconManager::InitInstance( void )
{
	HICON hIcon = ::LoadIcon( AfxGetResourceHandle()
							, MAKEINTRESOURCE( IDR_MAINFRAME )  // Icon to use
							);

	TRACE ( "I: Creating Tray Notification icon.\n" );
	// Create the tray icon

	if ( !m_TrayIcon.Create( NULL							// Let icon deal with its own messages
						   , WM_ICON_NOTIFY					// Icon notify message to use
						   , _T( "RtlSdr Catalog Control" )	// tooltip
						   , hIcon
						   , IDR_TRAY_POPUP					// ID of tray icon
						   , false
						   , NULL  //_T( "RtlSdr Catalog is starting" ), // balloon tip
						   , _T( "RtlSdr Catalog" )			// balloon title
						   , NIIF_INFO						// balloon icon
						   , 10								// balloon timeout
						   ))
    {
		TRACE ( "W: Failed to create tray notification icon!\n" );
    }
	else
	{
		TRACE ( "I: Tray notification icon created.\n" );

		m_TrayIcon.ShowBalloon( _T( "Initializing\r\nThis may take a minute..." )
							  , _T( "RtlSdr Catalog" )
							  , NIIF_INFO
							  , 10			// display time
							  );

		m_TrayIcon.SetMenuDefaultItem( 2, TRUE );			// default is minimize or show
	}
	return TRUE;
}

int CTrayIconManager::ExitInstance( void )
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP( CTrayIconManager, CWinThread )
	//{{AFX_MSG_MAP(CTrayIconManager)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrayIconManager message handlers

bool CTrayIconManager::ShowBalloon( const char *body
								  , const char *title
								  , DWORD flags
								  , int seconds
								  )
{
	tray_message msg;
	msg.message = body;
	msg.header	= title;
	msg.flags	= flags;
	msg.time	= seconds;

	message_queue.AddTail( msg );

	return true;							// it always works!
}

BOOL CTrayIconManager::OnIdle( LONG lCount )
{
	if ( m_TrayIcon.m_hWnd != NULL && message_queue.GetCount() > 0 )
	{
		tray_message msg = message_queue.GetHead();
		message_queue.RemoveHead();

		m_TrayIcon.ShowBalloon( msg.message
							  , msg.header
							  , msg.flags
							  , msg.time
							  );
	}
	
	return CWinThread::OnIdle( lCount );
}
