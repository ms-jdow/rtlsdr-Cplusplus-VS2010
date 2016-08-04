
// RtlSdr_CatalogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RtlSdr_Catalog.h"
#include "RtlSdr_CatalogDlg.h"
#include "afxdialogex.h"
#include "TrayIconManager.h"
#include <Dbt.h>
#include <winerror.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//	Thread Utility function

UINT flagthreadstarter( LPVOID pParam )
{
	CRtlSdr_CatalogDlg* data = (CRtlSdr_CatalogDlg*) pParam;
	data->FlagThreadFunction();
	return 0;
}

UINT threadstarter( LPVOID pParam )
{
	CRtlSdr_CatalogDlg* data = (CRtlSdr_CatalogDlg*) pParam;
	data->ThreadFunction();
	return 0;
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRtlSdr_CatalogDlg dialog


CRtlSdr_CatalogDlg::CRtlSdr_CatalogDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRtlSdr_CatalogDlg::IDD, pParent)
	, hDevNotify( NULL )
	, thread( NULL )
	, threadPending( NULL )
	, flagThread( NULL )
	, flagThreadRunning( false )
	, quiet( false )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRtlSdr_CatalogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_FIND, m_ctlFind );
	DDX_Control( pDX, IDC_RTLSDRLIST, m_ctl_MasterList );
}

BEGIN_MESSAGE_MAP(CRtlSdr_CatalogDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_FIND, OnFind )
	ON_WM_DEVICECHANGE()
	ON_CBN_DROPDOWN( IDC_DONGLE_LIST, OnCbnDropdownDongleList )
	ON_COMMAND( ID_SHOW_MAIN_WND, OnShowWindow )
	ON_COMMAND( ID_HIDE_MAIN_WND, OnHideWindow )
	ON_WM_CLOSE()
	ON_COMMAND( ID_TRUE_EXIT, OnTrueExit )
	ON_COMMAND(ID_EXIT, OnMenuExit)
	ON_BN_CLICKED(ID_REMOVE, OnBnClickedRemove )
	ON_COMMAND( ID_UPDATELIST, OnUpdateStrings )
END_MESSAGE_MAP()


// CRtlSdr_CatalogDlg message handlers

BOOL CRtlSdr_CatalogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	DWORD style = GetStyle();
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT(( IDM_ABOUTBOX & 0xFFF0 ) == IDM_ABOUTBOX );
	ASSERT( IDM_ABOUTBOX < 0xF000 );

	CMenu* pSysMenu = GetSystemMenu( FALSE );
	if ( pSysMenu != NULL )
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString( IDS_ABOUTBOX );
		ASSERT( bNameValid );
		if ( !strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu( MF_SEPARATOR );
			pSysMenu->AppendMenu( MF_STRING, IDM_ABOUTBOX, strAboutMenu );
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon( m_hIcon, TRUE );			// Set big icon
	SetIcon( m_hIcon, FALSE );		// Set small icon

	// TODO: Add extra initialization here
	TRACE("I: Starting thread to create tray icon.\n");

	TrayIcon = (CTrayIconManager*)
				AfxBeginThread( RUNTIME_CLASS( CTrayIconManager ));
	
	if ( !TrayIcon )
	{
		TRACE( "W: FAILED TO CREATE TRAY THREAD!\n" );
		PostQuitMessage( 0 );
	}
	m_TrayIcon = &TrayIcon->m_TrayIcon;

	//	The CWinThread will be automatically deleted on thread termination.

	if ( !donglelist.IsSharedMemoryActive())
	{
		exit( 1 );
	}
	RegisterForNotifications();

	flagThreadRunning = true;
	flagThread = AfxBeginThread( flagthreadstarter, this );
	if ( !quiet )
		PostMessage( WM_COMMAND, ID_SHOW_MAIN_WND );
	else
		PostMessage( WM_COMMAND, ID_HIDE_MAIN_WND );

	PostMessage( WM_COMMAND, IDC_FIND );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRtlSdr_CatalogDlg::OnSysCommand( UINT nID, LPARAM lParam )
{
	if (( nID & 0xFFF0 ) == IDM_ABOUTBOX )
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand( nID, lParam );
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRtlSdr_CatalogDlg::OnPaint( void )
{
	if ( IsIconic())
	{
		CPaintDC dc( this ); // device context for painting

		SendMessage( WM_ICONERASEBKGND
				   , reinterpret_cast<WPARAM> ( dc.GetSafeHdc())
				   , 0
				   );

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics( SM_CXICON );
		int cyIcon = GetSystemMetrics( SM_CYICON );
		CRect rect;
		GetClientRect(&rect);
		int x = ( rect.Width() - cxIcon + 1 ) / 2;
		int y = ( rect.Height() - cyIcon + 1 ) / 2;

		// Draw the icon
		dc.DrawIcon( x, y, m_hIcon );
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRtlSdr_CatalogDlg::OnQueryDragIcon( void )
{
	return static_cast<HCURSOR>( m_hIcon );
}


void CRtlSdr_CatalogDlg::OnDestroy( void )
{
	flagThreadRunning = false;
	int i = 0;
	while(( flagThread != NULL ) && ( i++ < 50 ))	// One second to go away....
		Sleep( 20 );

	CDialogEx::OnDestroy();

	if ( hDevNotify )
		UnregisterDeviceNotification( hDevNotify );
}


void CRtlSdr_CatalogDlg::OnFind( void )
{
	m_ctlFind.EnableWindow( false );
	m_ctl_MasterList.EnableWindow( false );
	thread = AfxBeginThread( threadstarter, this );
}


void CRtlSdr_CatalogDlg::OnShowWindow( void )
{
	TRACE ("Tray maximizing\n");
	if ( m_TrayIcon->WindowHidden())
		CSystemTray::MaximiseFromTray( this );
	else
		ShowWindow ( SW_NORMAL );
    m_TrayIcon->SetMenuDefaultItem( ID_HIDE_MAIN_WND, FALSE );
}


void CRtlSdr_CatalogDlg::OnHideWindow( void )
{
	TRACE ( "Tray minimizing\n" );
    CSystemTray::MinimiseToTray( this );
    m_TrayIcon->SetMenuDefaultItem( ID_SHOW_MAIN_WND, FALSE );
}


void CRtlSdr_CatalogDlg::OnClose( void )
{
	OnHideWindow();
}


void CRtlSdr_CatalogDlg::OnTrueExit( void )
{
	TrayIcon->PostThreadMessage( WM_QUIT, 0, 0 );
	CDialog::OnCancel();
}


void CRtlSdr_CatalogDlg::OnMenuExit( void )
{
	m_TrayIcon->PostMessage( WM_COMMAND, ID_TRUE_EXIT );
}


void CRtlSdr_CatalogDlg::OnBnClickedRemove( void )
{
	//	Remove entry from master list, dongle array, and registry
	//	But do that only if the dongle is not found.
	int sel = m_ctl_MasterList.GetCurSel();
	if ( sel < 0 )
		return;
	//	So it is an orphan entry. Remove it.
	//	But ask first
	Dongle* dongle = &Dongles[ sel ];
	if (( dongle->found < 0 )
//	&&	( dongle->vid != 0 )			// JD 20160336	let spurious entries
//	&&	( dongle->pid != 0 )			//				be deleted.
	   )
	{
		if ( dongle->busy )
		{
			CString msg;
			msg = _T( "Dongle is in use." );
			AfxMessageBox( msg, MB_ICONEXCLAMATION );
		}
		if ( AfxMessageBox( _T( "Remove highlighted entry?" ), MB_OKCANCEL )
				!= IDOK )
			return;
		//	Dongles array - move entry + 1 to entry, until entry is empty.
		//	And write updated registry.
		donglelist.RemoveDongle( sel );
		m_ctl_MasterList.DeleteString( sel );

		PostMessage( WM_COMMAND, IDC_FIND );

	}
}


//#include <C:\WinDDK\7600.16385.1\inc\api\usbiodef.h>
//{A5DCBF10-6530-11D2-901F-00C04FB951ED}
#define MY_DEFINE_GUID( name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8 ) \
    EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

MY_DEFINE_GUID( GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, \
				0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED );

void CRtlSdr_CatalogDlg::RegisterForNotifications( void )
{
	if ( hDevNotify )
	{
		return;
	}
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory( &NotificationFilter, sizeof( NotificationFilter ));
	NotificationFilter.dbcc_size = sizeof( DEV_BROADCAST_DEVICEINTERFACE );
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	//	assume we want to be notified with GUID_DEVINTERFACE_USB_DEVICE
	//	to get notified with all interface on XP or above
	//	ORed 3rd param with DEVICE_NOTIFY_ALL_INTERFACE_CLASSES and
	//	dbcc_classguid will be ignored
	NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
	hDevNotify = RegisterDeviceNotification( this->GetSafeHwnd()
										   , &NotificationFilter
										   , DEVICE_NOTIFY_WINDOW_HANDLE
										   );
	if ( !hDevNotify )
	{
	    // error handling...
		TRACE( "RegisterDeviceNotification failed %d\n", GetLastError());
	    return;
	}
	TRACE( "\n" );
}

BOOL CRtlSdr_CatalogDlg::OnDeviceChange( UINT nEvent
											, DWORD_PTR dwp
											)
{
	if ( !dwp )
		return TRUE;	//	Do nothing.

	TRACE( "nEvent = %d (0x%x), dwp = 0x%x\n", nEvent, nEvent, dwp );
	DEV_BROADCAST_HDR* hdr = (DEV_BROADCAST_HDR*) dwp;
	DEV_BROADCAST_DEVICEINTERFACE* di = (DEV_BROADCAST_DEVICEINTERFACE*) dwp;
	if ( dwp && ( hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE ))
	{
		TRACE( "Device type %d (0x%x)\n", hdr->dbch_devicetype, hdr->dbch_devicetype );
		switch( nEvent )
		{
		case DBT_DEVICEARRIVAL:
			TRACE( "Info %s just appeared thread = 0x%x\n", di->dbcc_name, thread );
			//	Start a thread to run GetCatalog()
			if ( thread == NULL )
			{
				m_ctl_MasterList.EnableWindow( false );
				Sleep( 5000 );
				thread = AfxBeginThread( threadstarter, this );
			}
			else
			if ( threadPending == NULL )
				threadPending = AfxBeginThread( threadstarter, this );
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			TRACE( "Info %s just vanished thread = 0x%x\n", di->dbcc_name, thread );
			//	Start a thread to run GetCatalog()
			if ( thread == NULL )
				thread = AfxBeginThread( threadstarter, this );
			else
			if ( threadPending == NULL )
				threadPending = AfxBeginThread( threadstarter, this );
			break;
		default:
			TRACE( "UNKNOWN EVENT TYPE\n" );
			break;
		}
	}
	TRACE( "Thread = 0x%x this = 0x%x\n", thread, this );
	return TRUE;
}


void CRtlSdr_CatalogDlg::ThreadFunction( void )
{
	TRACE( "Thread = 0x%x this = 0x%x threadPending 0x%x\n"
		 , thread, this, threadPending );
	m_ctlFind.EnableWindow( false );
	while( threadPending )
		Sleep( 100 );
	TRACE( "Begin thread 0x%x\n", thread );
	m_ctl_MasterList.EnableWindow( false );
	m_ctl_MasterList.ResetContent();
	donglelist.GetCatalog();
	for( int i = 0; i < donglelist.GetCount(); i++ )
	{
		CString manf;
		CString prod;
		CString sern;
		donglelist.GetStrings( i, manf, prod, sern );
		char modifier = ' ';
		if ( !donglelist.IsFound( i ))
			modifier = '-';
		else
		if ( donglelist.IsBusy( i ))
			modifier = 'B';
		CString name;
		name.Format( _T( "%c%s" )
				   , modifier
				   , _T( " " ) + manf + _T( "; " ) + prod + _T( "; " ) + sern
				   );
		m_ctl_MasterList.AddString( name );
	}
	TRACE( "End Thread 0x%x\n", thread );
	if ( threadPending )
	{
		TRACE( "Thread Pending\n" );
		Sleep( 5000 );
		if ( threadPending )
			thread = threadPending;
		else
			m_ctl_MasterList.EnableWindow( true );
		threadPending = NULL;
		return;
	}
	m_ctl_MasterList.EnableWindow( true );
	thread = NULL;
	m_ctlFind.EnableWindow( true );
}


void CRtlSdr_CatalogDlg::OnCbnDropdownDongleList( void )
{
	while( thread )
		Sleep( 100 );
}


void CRtlSdr_CatalogDlg::FlagThreadFunction( void )
{
	while( flagThreadRunning )
	{
		donglelist.SetMasterPresent();
		if ( donglelist.GetMasterUpdate())
		{
			donglelist.SetMasterPresent();
			PostMessage( WM_COMMAND, ID_UPDATELIST );
		}
		Sleep( 10 );
	}
	donglelist.SetMasterPresent( false );
	flagThread = NULL;
}


void CRtlSdr_CatalogDlg::OnUpdateStrings( void )
{
	TRACE( "Update strings\n" );
	Sleep( 300 );
	m_ctl_MasterList.ResetContent();
	for( int i = 0; i < donglelist.GetCount(); i++ )
	{
		CString manf;
		CString prod;
		CString sern;
		donglelist.GetStrings( i, manf, prod, sern );
		char modifier = ' ';
		if ( !donglelist.IsFound( i ))
			modifier = '-';
		else
		if ( donglelist.IsBusy( i ))
			modifier = 'B';
		CString name;
		name.Format( _T( "%c%s" )
				   , modifier
				   , _T( " " ) + manf + _T( "; " ) + prod + _T( "; " ) + sern
				   );
		m_ctl_MasterList.AddString( name );
	}
}

