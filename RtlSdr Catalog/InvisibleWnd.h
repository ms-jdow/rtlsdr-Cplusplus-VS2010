#pragma once

// InvisibleWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInvisibleWnd window

class CInvisibleWnd : public CWnd
{
// Construction
public:
	CInvisibleWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInvisibleWnd)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CInvisibleWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CInvisibleWnd)
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG
	afx_msg LRESULT OnNewLogMessage (WPARAM wParam, LPARAM lParam);
	BOOL OnQueryEndSession();
	void OnEndSession(BOOL bEnding);
	DECLARE_MESSAGE_MAP()

	HWND prev_child;
};

