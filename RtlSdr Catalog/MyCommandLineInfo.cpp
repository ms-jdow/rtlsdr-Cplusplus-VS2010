// MyCommandLineInfo.cpp: implementation of the MyCommandLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyCommandLineInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


extern const CString strings[];
extern const UINT commands[];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MyCommandLineInfo::MyCommandLineInfo()
	: quiet( false )
{
}

MyCommandLineInfo::~MyCommandLineInfo()
{

}
void MyCommandLineInfo::ParseParam( const TCHAR* pszParam
								  , BOOL bFlag
								  , BOOL bLast
								  )
{
	TRACE( _T( "Param: \"%s\", flag %d, last %d" ), pszParam, bFlag, bLast );

	if ( bFlag )
	{
		TRACE( "cmp = %d\n", _tcsicmp( pszParam, _T( "quiet" )));
		if ( _tcsicmp( pszParam, _T( "quiet" )) == 0 )
			quiet = true;
	}

	CCommandLineInfo::ParseParam( pszParam, bFlag, bLast );
}