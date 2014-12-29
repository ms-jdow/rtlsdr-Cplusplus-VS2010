// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#ifndef MODE_STATIC
#define MODE_STATIC		1
#define MODE_EXPORT		2
#define MODE_IMPORT		3
#endif


//	Select from the above three definitions
#ifndef MODE
#define MODE	MODE_EXPORT
#endif


#if MODE == MODE_STATIC
	#define SDRDAPI
#elif MODE == MODE_EXPORT
	#define SDRDAPI __declspec(dllexport)
#elif MODE == MODE_IMPORT
	#define SDRDAPI __declspec(dllimport)
#else
	#pragma message( "Please define MODE above as one of the three values above it." )
	THIS GUARANTEES AN ERROR WHEN COMPILING WITH "MODE" IMPROPERLY DEFINED!
#endif

#if defined( _M_X64 )
	#ifndef rtlsdr_STATIC
		#pragma comment( lib, "libusb\\MS64\\dll\\libusb-1.0.lib" )
	#else
		#pragma comment( lib, "libusb\\MS64\\static\\libusb-1.0.lib" )
	#endif
#else
	#ifndef rtlsdr_STATIC
		#pragma comment( lib, "libusb\\MS32\\dll\\libusb-1.0.lib" )
	#else
		#pragma comment( lib, "libusb\\MS32\\static\\libusb-1.0.lib" )
	#endif
#endif

typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MHZ( x )	(( x ) * 1000 * 1000)
#define KHZ( x )	(( x ) * 1000 )

