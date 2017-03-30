
// RtlSdr_CatalogDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "RtlSdrList.h"

class CTrayIconManager;
class CSystemTray;

// CRtlSdr_CatalogDlg dialog
class CRtlSdr_CatalogDlg : public CDialogEx
{
// Construction
public:
					CRtlSdr_CatalogDlg ( CWnd* pParent = NULL );	// standard constructor

// Dialog Data
	enum { IDD = IDD_RTLSDR_CATALOG_DIALOG };

	protected:
	virtual void	DoDataExchange			( CDataExchange* pDX );	// DDX/DDV support


// Implementation
protected:
	HICON			m_hIcon;
	CButton			m_ctlFind;
	CListBox		m_ctl_MasterList;

	// Generated message map functions
	virtual BOOL	OnInitDialog			( void );
	afx_msg void	OnSysCommand			( UINT nID
											, LPARAM lParam
											);
	afx_msg void	OnPaint					( void );
	afx_msg HCURSOR OnQueryDragIcon			( void );
	afx_msg void	OnDestroy				( void );
	afx_msg void	OnFind					( void );
	afx_msg BOOL	OnDeviceChange			( UINT nEvent
											, DWORD_PTR dwp
											);
	afx_msg void	OnCbnDropdownDongleList	( void );
	afx_msg	void	OnShowWindow			( void );
	afx_msg	void	OnHideWindow			( void );
	afx_msg void	OnTrueExit				( void );
	afx_msg void	OnClose					( void );
	afx_msg void	OnMenuExit				( void );
	afx_msg void	OnBnClickedRemove		( void );
	afx_msg void	OnUpdateStrings			( void );
	DECLARE_MESSAGE_MAP()

	void			RegisterForNotifications( void );

public:
	void			ThreadFunction			( void );
	void			FlagThreadFunction		( void );

protected:
	HDEVNOTIFY		hDevNotify;
	RtlSdrList		donglelist;
	CWinThread*		threadRunning;
	CWinThread*		threadPending;
	CWinThread*		flagThread;
	CTrayIconManager*
					TrayIcon;
	CSystemTray*	m_TrayIcon;
	bool			flagThreadRunning;
public:
	bool			quiet;
};
