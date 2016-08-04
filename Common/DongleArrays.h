//	DongleArrays.h

#pragma once

#define	MAX_USB_PATH	7
#define MAX_STR_SIZE	256

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
		duplicated    = r.duplicated;
		busy          = r.busy;
		found         = r.found;
		Spare         = r.Spare;
		tunerType	  = r.tunerType;
	}

	void operator = ( const Dongle& d );

	//	Make sure these are all null terminated.
	BYTE    manfIdStr[ MAX_STR_SIZE ];	//	Null terminated Manufacturer ID string
	BYTE	prodIdStr[ MAX_STR_SIZE ];	//	Null terminated Product ID string
	BYTE	sernIdStr[ MAX_STR_SIZE ];	//	Null terminated Serial Number ID string
	WORD	vid;						//	Vendor ID word
	WORD	pid;						//	Product ID word.
	BYTE	usbpath[ MAX_USB_PATH ];	//	Stores USB path through hubs to device
	bool	duplicated;					//	LibUsb index for this device.
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
		busy       = false;
		vid        = 0;
		pid        = 0;
		found      = false;
		duplicated = false;
	}

	void Clear( void )
	{
		busy	      = 0;
		vid		      = 0;
		pid		      = 0;
		tunerType     = 0;
		found         = -1;
		duplicated    = false;
		memset( manfIdStr, 0, MAX_STR_SIZE );
		memset( prodIdStr, 0, MAX_STR_SIZE );
		memset( sernIdStr, 0, MAX_STR_SIZE );
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
		duplicated    = r.duplicated;
		memcpy( &manfIdStr, r.manfIdStr, MAX_STR_SIZE );
		memcpy( &prodIdStr, r.prodIdStr, MAX_STR_SIZE );
		memcpy( &sernIdStr, r.sernIdStr, MAX_STR_SIZE );
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
			return ( strcmp( manfIdStr, d.manfIdStr ) == 0 )
				&& ( strcmp( prodIdStr, d.prodIdStr ) == 0 )
				&& ( strcmp( sernIdStr, d.sernIdStr ) == 0 )
				&& ( vid         == d.vid )
				&& ( pid         == d.pid )
				;
		}
	}

	bool operator != ( const Dongle& d ) const
	{
		return !( *this == d );
	}

	char	manfIdStr[ MAX_STR_SIZE ];	//	Null terminated Manufacturer ID string
	char	prodIdStr[ MAX_STR_SIZE ];	//	Null terminated Product ID string
	char	sernIdStr[ MAX_STR_SIZE ];	//	Null terminated Serial Number ID string
	WORD	vid;						//	Manufacturer ID word
	WORD	pid;						//	Product ID word.
	BYTE	usbpath[ 7 ];				//	Stores USB path through hubs to device
	bool	busy;						//	Device is busy if true.
	BYTE	tunerType;					//	Type of tuner per enum rtlsdr_tuner
	char	found;						//	RTLSDR index for merging data
	bool	duplicated;					//	Is this entry a near duplicate?
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
	duplicated    = d.duplicated;

	memcpy( manfIdStr, d.manfIdStr, MAX_STR_SIZE );
	memcpy( prodIdStr, d.prodIdStr, MAX_STR_SIZE );
	memcpy( sernIdStr, d.sernIdStr, MAX_STR_SIZE );
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
