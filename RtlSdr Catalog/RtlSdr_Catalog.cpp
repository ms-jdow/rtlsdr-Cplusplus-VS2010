
// RtlSdr_Catalog.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "RtlSdr_Catalog.h"
#include "RtlSdr_CatalogDlg.h"
#include "MyCommandLineInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRtlSdr_CatalogApp

BEGIN_MESSAGE_MAP( CRtlSdr_CatalogApp, CWinApp )
	ON_COMMAND( ID_HELP, &CWinApp::OnHelp )
END_MESSAGE_MAP()


// CRtlSdr_CatalogApp construction

CRtlSdr_CatalogApp::CRtlSdr_CatalogApp( void )
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CRtlSdr_CatalogApp object

CRtlSdr_CatalogApp theApp;


// CRtlSdr_CatalogApp initialization

BOOL CRtlSdr_CatalogApp::InitInstance( void )
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Parse command line for standard shell commands, DDE, file open
	MyCommandLineInfo cmdInfo;
	ParseCommandLine( cmdInfo );

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	SetRegistryKey( _T( "RtlSdr Catalog" ));

	CRtlSdr_CatalogDlg dlg;
	m_pMainWnd = &dlg;
	if ( cmdInfo.quiet == true )
		dlg.quiet = true;
	INT_PTR nResponse = dlg.DoModal();

	_AFX_THREAD_STATE* pState = AfxGetThreadState();
	ASSERT( pState );
	if ( pState->m_msgCur.lParam == ID_TRUE_EXIT )
		pState->m_msgCur.lParam = 0;

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

