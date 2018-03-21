
// rtlsdr_service_test.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Crtlsdr_service_testApp:
// See rtlsdr_service_test.cpp for the implementation of this class
//

class CRtlSdr_CatalogApp : public CWinApp
{
public:
	CRtlSdr_CatalogApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CRtlSdr_CatalogApp theApp;