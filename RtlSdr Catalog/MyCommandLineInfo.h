// MyCommandLineInfo.h: interface for the MyCommandLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class MyCommandLineInfo : public CCommandLineInfo  
{
public:
	MyCommandLineInfo();
	virtual ~MyCommandLineInfo();

	void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);

public:
	bool	quiet;
};
