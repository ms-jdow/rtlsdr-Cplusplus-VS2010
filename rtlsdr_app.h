// rtlsdr_app.h : main header file for the RTL Direct DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "SharedMemoryFile.h"

//#define THEAPP	//	If MFC dialogs are ever used anywhere this must be set.
				//	This is also a handy way to get this module version. But
				//	I am not sure I need it.

// CRTLDirectApp
// See RTL Direct.cpp for the implementation of this class
//

#if defined THEAPP
class rtlsdr;
extern CStringA RtlSdrVersionString;

class rtlsdr_app : public CWinApp
{
public:
	rtlsdr_app();
	~rtlsdr_app();

	rtlsdr*		GetSdrDongle			( void );
	void		CloseSdrDongle			( rtlsdr* dongle );
	rtlsdr*		FindDongleByDeviceIndex	( int index );
	LPCSTR		GetRtlSdrVersionString	( void ) { return RtlSdrVersionString; }

// Overrides
public:
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();

	DECLARE_MESSAGE_MAP()

//	static CStringA	RtlSdrVersionString;

protected:
};

extern rtlsdr_app theApp;
#endif
extern RtlSdrAreaDef				TestDongles;
extern RtlSdrAreaDef*				RtlSdrArea;
extern Dongle*						Dongles;