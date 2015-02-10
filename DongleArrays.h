//	DongleArrays.h

#pragma once

#define	MAX_USB_PATH	7

class Dongle;

class Return
{
public:
	Return() {}

	Return( const Return& r )
	{
		*this = r;
	}

	void operator = ( const Return& r )
	{
		memcpy( manfIdStr, r.manfIdStr, sizeof( Return ));
	}

	//	As = except Does not overwrite string4, the usbpath
	void operator += ( const Return& r )
	{
		memcpy( manfIdStr, r.manfIdStr, 3 * sizeof( manfIdStr ));
		vid           = r.vid;
		pid           = r.pid;
		NotNeeded     = r.NotNeeded;
		busy          = r.busy;
		found         = r.found;
		Spare         = r.Spare;
		tunerType	  = r.tunerType;
	}

	void operator = ( const Dongle& d );

	//	Make sure these are all null terminated.
	BYTE    manfIdStr[ 256 ];			//	Null terminated Manufacturer ID string
	BYTE	prodIdStr[ 256 ];			//	Null terminated Product ID string
	BYTE	sernIdStr[ 256 ];			//	Null terminated Serial Number ID string
	WORD	vid;						//	Vendor ID word
	WORD	pid;						//	Product ID word.
	BYTE	usbpath[ MAX_USB_PATH ];	//	Stores USB path through hubs to device
	BYTE	NotNeeded;					//	LibUsb index for this device.
	BYTE	busy;						//	Maintain file format.
	BYTE	found;
	BYTE	Spare;
	BYTE	tunerType;					//	Type of tuner per enum rtlsdr_tuner
};

class Dongle
{
public:
	Dongle()
	{
		busy     = false;
		vid      = 0;
		pid      = 0;
		found    = false;
	}

	void Clear( void )
	{
		busy	      = 0;
		vid		      = 0;
		pid		      = 0;
		tunerType     = 0;
		found         = -1;
		manfIdCStr.Empty();
		prodIdCStr.Empty();
		sernIdCStr.Empty();
		memset( usbpath, 0, sizeof( usbpath ));
	}

	Dongle( const Dongle& d )
	{
		*this = d;
	}

	Dongle( const Return& r )
	{
		*this = r;
	}

	void operator = ( const Return& r )
	{
		busy	      = r.busy != 0;	// Determined other ways.
		found         = r.found;
		vid		      = r.vid;
		pid		      = r.pid;
		tunerType     = r.tunerType;
		found         = -1;
		manfIdCStr    = r.manfIdStr;
		prodIdCStr    = r.prodIdStr;
		sernIdCStr    = r.sernIdStr;
		memcpy( usbpath, r.usbpath, sizeof( usbpath ));
	}

	bool operator == ( const Dongle& d ) const
	{
		if ( busy || d.busy )
		{
			//	Make a best guess. If it's in the same slot it's the same thing.
			//	This is the best guess I can make.
			return ( memcmp( &usbpath, &d.usbpath, sizeof( usbpath )) == 0 )
				&& ( vid      == d.vid )
				&& ( pid      == d.pid )
				;
		}
		else
		{
			return ( manfIdCStr  == d.manfIdCStr )
				&& ( prodIdCStr  == d.prodIdCStr )
				&& ( sernIdCStr  == d.sernIdCStr )
				&& ( vid         == d.vid )
				&& ( pid         == d.pid )
				;
		}
	}

	bool operator != ( const Dongle& d ) const
	{
		return !( *this == d );
	}

	CString manfIdCStr;					//	Null terminated Manufacturer ID string
	CString prodIdCStr;					//	Null terminated Product ID string
	CString sernIdCStr;					//	Null terminated Serial Number ID string
	WORD	vid;						//	Manufacturer ID word
	WORD	pid;						//	Product ID word.
	BYTE	usbpath[ 7 ];				//	Stores USB path through hubs to device
	bool	busy;						//	Device is busy if true.
	BYTE	tunerType;					//	Type of tuner per enum rtlsdr_tuner
	char	found;						//	RTLSDR index for merging data
};

__inline void Return::operator = ( const Dongle& d )
{
	memset( &manfIdStr, 0, sizeof( Return ));
	vid           = d.vid;
	pid           = d.pid;
	busy          = d.busy;
	found	      = d.found;
	Spare         = 0;
	tunerType     = d.tunerType;
	NotNeeded     = 0;

	memcpy( manfIdStr, CStringA( d.manfIdCStr ), d.manfIdCStr.GetLength());
	memcpy( prodIdStr, CStringA( d.prodIdCStr ), d.prodIdCStr.GetLength());
	memcpy( sernIdStr, CStringA( d.sernIdCStr ), d.sernIdCStr.GetLength());
	memcpy( usbpath, d.usbpath, MAX_USB_PATH );
}

class CDongleArray : public CArray< Dongle, Dongle >
{
public:
	CDongleArray() {}

	void operator = ( const CDongleArray& da )
	{
		RemoveAll();
		for( INT_PTR i = 0; i < da.GetSize(); i++ )
			Add( da.GetAt( i ));
	}

	void operator += (const CDongleArray& da )
	{
		for( INT_PTR i = 0; i < da.GetSize(); i++ )
		{
			INT_PTR j = 0;
			for ( ; j < GetSize(); j++ )
			{
				if ( GetAt( j ) == da.GetAt( i ))
					break;
			}
			if ( j >= GetSize())
				Add( da.GetAt( i ));
		}
	}

	INT_PTR Add( const Dongle& dongle )
	{
		//	Look for a duplicate already present.
		for ( INT_PTR entry = 0; entry < GetSize(); entry++ )
		{
			if ( dongle == GetAt( entry ))
				return entry;
		}
		return __super::Add( dongle );
	}

	void SetAllNotFound( void )
	{
		for ( INT_PTR i = 0; i < GetSize(); i++ )
		{
			GetAt( i ).found = -1;
		}
	}

	// For this one we do not sort or any of the other nifty stuff.
};
