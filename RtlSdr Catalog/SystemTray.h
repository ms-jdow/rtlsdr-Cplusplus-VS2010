/////////////////////////////////////////////////////////////////////////////
// SystemTray.h : header file
//
// Written by Chris Maunder (cmaunder@mail.com)
// Copyright (c) 1998.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. If 
// the source code in  this file is used in any commercial application 
// then acknowledgement must be made to the author of this file 
// (in whatever form you wish).
//
// This file is provided "as is" with no expressed or implied warranty.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 

#pragma once

#include <afxtempl.h>
#include <afxdisp.h>    // COleDateTime
#include "InvisibleWnd.h"

/////////////////////////////////////////////////////////////////////////////
// CSystemTray window

class CSystemTray : public CWnd
{
// Construction/destruction
public:
				CSystemTray				( void );
				CSystemTray				( CWnd* pWnd
										, UINT uCallbackMessage
										, LPCTSTR szTip
										, HICON icon
										, UINT uID
										, BOOL bhidden = FALSE
										, LPCTSTR szBalloonTip = NULL
										, LPCTSTR szBalloonTitle = NULL
										, DWORD dwBalloonIcon = NIIF_NONE
										, UINT uBalloonTimeout = 10
										);
    virtual ~CSystemTray();

    DECLARE_DYNAMIC( CSystemTray )

// Operations
public:
    BOOL		Enabled					( void ) { return m_bEnabled; }
    BOOL		Visible					( void ) { return !m_bHidden; }	// icon state, not window state!
	BOOL		WindowHidden			( void ) { return m_bWindowHidden; }

    // Create the tray icon
    BOOL		Create					( CWnd* pParent
										, UINT uCallbackMessage
										, LPCTSTR szTip
										, HICON icon
										, UINT uID
										, BOOL bHidden = FALSE
										, LPCTSTR szBalloonTip = NULL
										, LPCTSTR szBalloonTitle = NULL
										, DWORD dwBalloonIcon = NIIF_NONE
										, UINT uBalloonTimeout = 10
										);

    // Change or retrieve the Tooltip text
    BOOL		SetTooltipText			( LPCTSTR pszTooltipText );
    BOOL		SetTooltipText			( UINT nID );
    CString		GetTooltipText			( void ) const;

    // Change or retrieve the icon displayed
    BOOL		SetIcon					( HICON hIcon );
    BOOL		SetIcon					( LPCTSTR lpszIconName );
    BOOL		SetIcon					( UINT nIDResource );
    BOOL		SetStandardIcon			( LPCTSTR lpIconName );
    BOOL		SetStandardIcon			( UINT nIDResource );
    HICON		GetIcon					( void ) const;

    void		SetFocus				( void );
    BOOL		HideIcon				( void );
    BOOL		ShowIcon				( void );
    BOOL		AddIcon					( void );
    BOOL		RemoveIcon				( void );
    BOOL		MoveToRight				( void );

    BOOL		ShowBalloon				( LPCTSTR szText
										, LPCTSTR szTitle = NULL
										, DWORD dwIcon = NIIF_NONE
										, UINT uTimeout = 10
										);

    // For icon animation
    BOOL		SetIconList				( UINT uFirstIconID
										, UINT uLastIconID
										);
    BOOL		SetIconList				( HICON* pHIconList
										, UINT nNumIcons
										);
    BOOL		Animate					( UINT nDelayMilliSeconds
										, int nNumSeconds = -1
										);
    BOOL		StepAnimation			( void );
    BOOL		StopAnimation			( void );

    // Change menu default item
    void		GetMenuDefaultItem		( UINT& uItem, BOOL& bByPos );
    BOOL		SetMenuDefaultItem		( UINT uItem, BOOL bByPos );

    // Change or retrieve the window to send notification messages to
    BOOL		SetNotificationWnd		( CWnd* pNotifyWnd );
    CWnd*		GetNotificationWnd		( void ) const;

    // Change or retrieve the window to send menu commands to
    BOOL		SetTargetWnd			( CWnd* pTargetWnd );
    CWnd*		GetTargetWnd			( void ) const;

    // Change or retrieve  notification messages sent to the window
    BOOL		SetCallbackMessage		( UINT uCallbackMessage );
    UINT		GetCallbackMessage		( void ) const;

    UINT_PTR	GetTimerID				( void ) const  { return m_nTimerID; }

// Static functions
public:
    static void MinimiseToTray			( CWnd* pWnd
										, BOOL bForceAnimation = FALSE
										);
    static void MaximiseFromTray		( CWnd* pWnd
										, BOOL bForceAnimation = FALSE
										);

public:
    // Default handler for tray notification message
    virtual LRESULT OnTrayNotification	( WPARAM uID
										, LPARAM lEvent
										);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSystemTray)
	protected:
	virtual LRESULT WindowProc			( UINT message
										, WPARAM wParam
										, LPARAM lParam
										);
	//}}AFX_VIRTUAL

// Implementation
protected:
    void		Initialise				( void );
    void		InstallIconPending		( void );

	virtual void CustomizeMenu			( CMenu* ) {}	// Used for customizing the menu

// Implementation
protected:
    NOTIFYICONDATA	m_tnd;
    BOOL		m_bEnabled;				// does O/S support tray icon?
    BOOL		m_bHidden;				// Has the icon been hidden?
    BOOL		m_bRemoved;				// Has the icon been removed?
    BOOL		m_bShowIconPending;		// Show the icon once tha taskbar has been created
    BOOL		m_bWin2K;				// Use new W2K features?
	CWnd*		m_pTargetWnd;			// Window that menu commands are sent

    CArray< HICON, HICON > m_IconList;
    UINT_PTR     m_uIDTimer;
    INT_PTR      m_nCurrentIcon;
    COleDateTime m_StartTime;
    UINT         m_nAnimationPeriod;
    HICON        m_hSavedIcon;
    UINT         m_DefaultMenuItemID;
    BOOL         m_DefaultMenuItemByPos;
	UINT         m_uCreationFlags;

// Static data
protected:
    static BOOL	RemoveTaskbarIcon		( CWnd* pWnd );

    static const UINT		m_nTimerID;
    static UINT				m_nMaxTooltipLength;
    static const UINT		m_nTaskbarCreatedMsg;
    static CInvisibleWnd	m_wndInvisible;
	static BOOL				m_bWindowHidden;	// window is hidden

    static BOOL	GetW2K					( void );
#ifndef _WIN32_WCE
    static void GetTrayWndRect			( LPRECT lprect );
    static BOOL GetDoWndAnimation();
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CSystemTray)
	afx_msg void OnTimer				( UINT_PTR nIDEvent );
	//}}AFX_MSG
#ifndef _WIN32_WCE
	afx_msg void OnSettingChange		( UINT uFlags
										, LPCTSTR lpszSection
										);
#endif
    LRESULT		OnTaskbarCreated		( WPARAM wParam
										, LPARAM lParam
										);
	LRESULT		OnDumpRequest			( WPARAM wParam
										, LPARAM lParam
										);

	afx_msg void OnShowMainWnd			( void );
	afx_msg void OnUpdateShowMainWnd	( CCmdUI* pCmdUI );
	afx_msg void OnHideMainWnd			( void );
	afx_msg void OnUpdateHideMainWnd	( CCmdUI* pCmdUI );
	afx_msg void OnAppExit				( void );
	afx_msg	void OnAppAbout				( void );

    DECLARE_MESSAGE_MAP()
};

