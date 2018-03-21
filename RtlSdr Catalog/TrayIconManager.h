#pragma once

// TrayIconManager.h : header file
//

#include "SystemTray.h"


/////////////////////////////////////////////////////////////////////////////
// CTrayIconManager thread

class CTrayIconManager : public CWinThread
{
	DECLARE_DYNCREATE( CTrayIconManager )
protected:
					CTrayIconManager	( void );	// protected constructor used by dynamic creation

// Attributes
public:
	CSystemTray		m_TrayIcon;

// Operations
public:
	virtual bool	ShowBalloon			( const char *body
										, const char* title
										, DWORD flags
										, int seconds = 10
										);

// Overrides
public:
	virtual BOOL	InitInstance		( void  );
	virtual int		ExitInstance		( void  );
	virtual BOOL	OnIdle				( LONG lCount );

// Implementation
protected:
	virtual			~CTrayIconManager	( void );

	DECLARE_MESSAGE_MAP()

	class tray_message
	{
	public:
		CString		message;
		CString		header;
		DWORD		flags;
		DWORD		time;
	};

	CList < tray_message, tray_message& > message_queue;
};
