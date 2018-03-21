// stdafx.cpp : source file that includes just the standard includes
// rtsdr.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#if RELEASE_DEBUG_OUT
	void AfxMsgDbgOut( LPCTSTR lpszFormat, ...)
	{
		CString Format( lpszFormat );
		TCHAR buf [4096];
		va_list va;
		DWORD lastError;

		lastError = GetLastError();						// save current error
		_tcsncpy_s( buf, 4096, _T( "rtlsdr: " ), 8 );

		va_start( va, lpszFormat );
		int pt = _vstprintf_s( &buf[ 8 ], 4096 - 8, Format, va ) + 8;
		va_end( va );

		buf[ pt ] = 0;
		OutputDebugString( buf );
//		AfxMessageBox(buf);
		SetLastError (lastError);						// restore current error
	}
#endif
