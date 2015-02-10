#include "StdAfx.h"
#include "librtlsdr.h"

//	This file coordinates the dongle database and its Windows Registry
//	use for the rtlsdr class in the rtlsdr.dll project.

CDongleArray	rtlsdr::Dongles;
time_t			rtlsdr::lastCatalog = 0;
bool			rtlsdr::noCatalog = false;
bool			rtlsdr::goodCatalog = false;

#define REGISTRYBASE	_T( "Software\\rtlsdr" )

#define STR_OFFSET	9

CMyMutex	rtlsdr::registry_mutex( _T( "RtlSdr++ Mutex" ));
CMyMutex	rtlsdr::dongle_mutex;


//	STATIC	//
void rtlsdr::FillDongleArraysAndComboBox( CDongleArray * da
										, CComboBox * combo
										, LPCTSTR selected
										)
{
	rtlsdr::GetCatalog();
	if ( goodCatalog )
	{
		if ( combo )
			combo->ResetContent();
		if ( da )
			da->RemoveAll();

		CMutexLock cml( dongle_mutex );

		INT_PTR comboselected = -1;
		for( INT_PTR index = 0; index < Dongles.GetSize(); index++ )
		{
			Dongle* dongle = &Dongles.GetAt( index );
			if ( dongle->found < 0 )
				continue;
			if ( combo )
			{
				CStringA msg;
				if (dongle->busy )
					msg.Format( "* %s,%s,sn %s", dongle->manfIdCStr, dongle->prodIdCStr, dongle->sernIdCStr );
				else
					msg.Format( "%s, %s, sn %s", dongle->manfIdCStr, dongle->prodIdCStr, dongle->sernIdCStr );
				combo->AddString( CString( msg ));
				if ( selected && ( msg == selected ))
					comboselected = index;
			}
			if ( da )
			{
				da->Add( *dongle );
			}
		}

		if ( combo && combo->GetCount() > 0 )
		{
			combo->SetCurSel((int) comboselected );
		}
	}
}

//	STATIC	//
bool rtlsdr::GetDongleIdString( CString& string, int devindex )
{
	CMutexLock cml( dongle_mutex );

	string.Empty();
	if ((DWORD) devindex >= (DWORD) Dongles.GetSize())
		return false;
	rtlsdr::GetCatalog();
	if ( goodCatalog )
	{
		Dongle* dongle = NULL;
		int tindex = 0;
		for( INT_PTR i = 0; i < Dongles.GetSize(); i++ )
		{
			if ( Dongles[ i ].found == devindex )
			{
				dongle = &Dongles[ i ];
				string.Format( _T( "%s, %s, %s" ), dongle->manfIdCStr, dongle->prodIdCStr, dongle->sernIdCStr );
				return true;
			}
		}
	}
	return false;
}

//	STATIC	//		/// NOT USED
int rtlsdr::FindDongleByIdString( LPCTSTR source )
{
	CMutexLock cml( dongle_mutex );

	CString test( source );
	if ( test[ 0 ] == _T( '*' ))
		test = test.Mid( 2 );

	//	Name syntax		"<user tag> ppm <number>"
	//	Ignore variable data in the name;
	int loc = test.Find( _T( "ppm" ));
	if ( loc > 0 )
		test = test.Left( loc );
	int found = 0;
	for ( int i = 0; i < (int) Dongles.GetSize(); i++ )
	{
		CString tstring;
		GetDongleIdString( tstring, i );
		int loc = tstring.Find( _T( "ppm" ));
		if (( loc < 0 ) && ( tstring == test ))
			return Dongles[ i ].found;
		else
		if ( tstring.Left( loc ) == test )
			return Dongles[ i ].found;
	}
	return -1;
}

//	This writes the dongle array values out to the registry.
//	We leave old, not replaced, values around so that we don't lose dongles that
//	are temporarily removed. Note that duplicated entries created chiefly by hand
//	editing the registry or by merging exported registry values have been removed
//	in the DongleArray "Add()" function.)
void rtlsdr::WriteRegistry( void )
{
	TRACE( "WriteRegistry\n" );
	CMutexLock cml( registry_mutex );

	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;
	DWORD	status;
	// Just to be sure - clear everthing here
	resVal = RegCreateKeyEx( HKEY_CURRENT_USER
						   , REGISTRYBASE	//_T( "Software\\rtlsdr" )
						   , 0
						   , NULL
						   , 0
						   , KEY_ALL_ACCESS
						   , NULL
						   , &hRtlsdrKey
						   , &status
						   );
	//
	// If successful, read all the registry entries under this key    
	//

	if ( resVal == ERROR_SUCCESS ) 
	{
		TRACE( L"Got rtlsdr key" );
		// First initiate any global values the various classes have.
		DWORD resVal;
		DWORD keytype = REG_DWORD;
		DWORD Size = sizeof( DWORD );
		DWORD debugfile = true;
		// So now we read values we need.
		CString name;
		for ( INT_PTR i = 0; i < Dongles.GetSize(); i++ )
		{
			name.Format( _T( "Dongle%d" ), i );
			Return returned;
			returned = Dongles[ i ];
			resVal = RegSetValueEx( hRtlsdrKey
								  , name
								  , 0
								  , REG_BINARY
								  , (const BYTE*) &returned
								  , sizeof( Return )
								  );
			if ( resVal != ERROR_SUCCESS )
				TRACE( L" error setting ThreadPriority value!" );
		}
		RegCloseKey( hRtlsdrKey );
	}
}

//	Call only from inside a mutex lock.
//	STATIC //
uint32_t rtlsdr::WriteSingleRegistry( int index )
{
	TRACE( "WriteSingleRegistry( %d)\n", index );
	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;
	DWORD	status;

	// Just to be sure - clear everthing here
	resVal = RegCreateKeyEx( HKEY_CURRENT_USER
						   , REGISTRYBASE	//_T( "Software\\rtlsdr" )
						   , 0
						   , NULL
						   , 0
						   , KEY_ALL_ACCESS
						   , NULL
						   , &hRtlsdrKey
						   , &status
						   );

	//
	// If successful, read all the registry entries under this key    
	//

	if ( resVal == ERROR_SUCCESS ) 
	{
		TRACE( L"Got rtlsdr key\n" );
		// First initiate any global values the various classes have.
		DWORD resVal;
		DWORD keytype = REG_DWORD;
		DWORD Size = sizeof( DWORD );
		DWORD debugfile = true;
		// So now we read values we need.
		CString name;
		name.Format( _T( "Dongle%d" ), index );
		Return returned;
		returned = Dongles[ index ];
		resVal = RegSetValueEx( hRtlsdrKey
							  , name
							  , 0
							  , REG_BINARY
							  , (const BYTE*) &returned
							  , sizeof( Return )
							  );
		if ( resVal != ERROR_SUCCESS )
			TRACE( L" error resetting %s data %d!", name, resVal );
		RegCloseKey( hRtlsdrKey );
		WriteRegLastCatalogTime();
	}
	return resVal;
}

//	STATIC //
void rtlsdr::WriteRegLastCatalogTime( void )
{
	TRACE( "WriteRegLastCatalogTime\n" );
	CMutexLock cml( registry_mutex );
	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;
	DWORD	status;

	// Just to be sure - clear everthing here
	resVal = RegCreateKeyEx( HKEY_CURRENT_USER
						   , REGISTRYBASE	//_T( "Software\\rtlsdr" )
						   , 0
						   , NULL
						   , 0
						   , KEY_ALL_ACCESS
						   , NULL
						   , &hRtlsdrKey
						   , &status
						   );

	//
	// If successful, read all the registry entries under this key    
	//
	if ( resVal == ERROR_SUCCESS ) 
	{
		TRACE( L"Got rtlsdr key\n" );
		DWORD resVal;
		DWORD debugfile = true;
		// So now we read values we need.
		resVal = RegSetValueEx( hRtlsdrKey
							  , _T( "LastCatalog" )
							  , 0
							  , REG_QWORD
							  , (const BYTE*) &lastCatalog
							  , sizeof( __int64 )
							  );
		if ( resVal != ERROR_SUCCESS )
			TRACE( L" error resetting LastCatalog data %d!", resVal );
		RegCloseKey( hRtlsdrKey );
	}
}

void rtlsdr::ReadRegistry( void )
{
	TRACE( "ReadRegistry\n" );
	CMutexLock cml( registry_mutex );

	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;
	// Just to be sure - clear everthing here

	resVal = RegOpenKeyEx( HKEY_CURRENT_USER // HKEY_LOCAL_MACHINE
						 , REGISTRYBASE
						 , 0
						 , KEY_READ
						 , &hRtlsdrKey
						 );

	if ( resVal == ERROR_SUCCESS ) 
	{
		TRACE( L"Got rtlsdr key" );
		// First initiate any global values the various classes have.
		Return returned;
		DWORD resVal;
		DWORD keytype = REG_DWORD;
		DWORD Size = sizeof( DWORD );
		DWORD debugfile = true;
		// So now we read values we need.
		CString name;
		Dongles.RemoveAll();
		for ( INT_PTR i = 0; ; i++ )
		{
			name.Format( _T( "Dongle%d" ), i );
			DWORD Size = sizeof( Return );
			DWORD keytype;

			resVal = RegQueryValueEx( hRtlsdrKey
									, name
									, NULL
									, &keytype
									, (BYTE*) &returned
									, &Size
									);
			if (( resVal == ERROR_SUCCESS )
			&&	( keytype == REG_BINARY ))
			{
				// Fill out a description
				TRACE( L"Got %s, tuner %d\n", name, returned.tunerType );
				Dongle dongle( returned );
				Dongles.Add( dongle );	// Implicit Return = Dongle from Arrays
			}
			else
			{
				break;
			}
		}

		Size = sizeof( __int64 );
		__int64	value;

		keytype = REG_QWORD;
		resVal = RegQueryValueEx( hRtlsdrKey
								, _T( "LastCatalog" )
								, NULL
								, &keytype
								, (BYTE*) &value
								, &Size
								);
		if (( resVal == ERROR_SUCCESS )
		&&	( keytype == REG_QWORD ))
		{
			// Fill out a description
			TRACE( L"Got LastCatalog\n" );
			Dongles.Add( returned );	// Implicit Return = Dongle from Arrays
			lastCatalog = max( value, 1 );
		}
		else
			lastCatalog = 1;

		RegCloseKey( hRtlsdrKey );
	}
}

void rtlsdr::mergeToMaster( Dongle& tempd, int index )
{
	CMutexLock cml( registry_mutex );

	for ( INT_PTR mast = 0; mast < Dongles.GetSize(); mast++ )
	{
		Dongle *md = &Dongles[ mast ];
		if ( *md == tempd )
		{
			md->busy = tempd.busy;
			md->found = tempd.found;
			if ( !tempd.busy )
			{
				md->manfIdCStr   = tempd.manfIdCStr;
				md->prodIdCStr   = tempd.prodIdCStr;
				md->sernIdCStr   = tempd.sernIdCStr;
				rtlsdr dev;
				int ret = dev.rtlsdr_open((uint32_t) md->found );
				if ( ret == 0 )
				{
					md->tunerType = dev.rtlsdr_get_tuner_type();
					dev.rtlsdr_close();
				}
			}
			return;
		}
	}

	if ( !tempd.busy )
	{
		rtlsdr dev;
		int ret = dev.rtlsdr_open( index );
		if ( ret == 0 )
		{
			tempd.tunerType = dev.rtlsdr_get_tuner_type();
			dev.rtlsdr_close();
		}
	}
	Dongles.Add( tempd );
}


const rtlsdr_dongle_t * rtlsdr::find_known_device( uint16_t vid
												 , uint16_t pid
												 )
{
	unsigned int i;
	const rtlsdr_dongle_t *device = NULL;
	CMutexLock cml( dongle_mutex );

	for ( i = 0; i < sizeof( known_devices ) / sizeof( rtlsdr_dongle_t ); i++ )
	{
		if ( known_devices[ i ].vid == vid && known_devices[ i ].pid == pid)
		{
			device = &known_devices[ i ];
			break;
		}
	}

	return device;
}

int rtlsdr::GetEepromStrings( const eepromdata& data
							, CString* manf
							, CString* prod
							, CString* sern
							)
{
	//	Pull the whole set of strings.
	int pos = STR_OFFSET;
	pos = GetEepromString( data, STR_OFFSET, manf );
	if ( pos < 0 )
		return pos;
	pos = GetEepromString( data, pos, prod );
	if ( pos < 0 )
		return pos;
	pos = GetEepromString( data, pos, sern );
	if ( pos < 0 )
		return pos;
	return 0;
}

int rtlsdr::GetEepromStrings( uint8_t *data
							, int datalen
							, char* manf
							, char* prod
							, char* sern
							)
{
	//	Pull the whole set of strings.
	int pos = STR_OFFSET;
	pos = GetEepromString( data, datalen, STR_OFFSET, manf );
	if ( pos < 0 )
		return pos;
	pos = GetEepromString( data, datalen, pos, prod );
	if ( pos < 0 )
		return pos;
	pos = GetEepromString( data, datalen, pos, sern );
	if ( pos < 0 )
		return pos;
	return 0;
}
	
int rtlsdr::GetEepromString( const eepromdata& data
						   , int pos
						   , CString* string
						   )

{
	int size = data[ pos ];
	if ( pos + size >= EEPROM_SIZE )
	{
		TRACE( "Past end of eeprom region\n" );
		return -4;
	}

	if ( data[ pos + 1 ] != 0x03 )
	{
		TRACE( "Invalid string descriptor value\n" );
		return -4;
	}

	// Do this safely if the eeprom area is exactly full.
	if ( string)
	{
		uint8_t tdata[ 257 ];
		memcpy( tdata, data, 256 );
		tdata[ size + pos ] = 0;
		tdata[ size + pos + 1 ] = 0;
		*string = (TCHAR*) &tdata[ pos + 2 ];
	}
	return size + pos;
}

int rtlsdr::GetEepromString( const uint8_t *data
						   , int datalen		// Length to read.
						   , int pos
						   , char* string	// BETTER be 256 bytes long
						   )
{
	int size = data[ pos ];
	if ( pos + size > datalen )
	{
		TRACE( "Past end of eeprom region\n" );
		return -4;
	}

	if ( data[ pos + 1 ] != 0x03 )
	{
		TRACE( "Invalid string descriptor value\n" );
		return -4;
	}

	if ( string != NULL )
	{
		// Do this safely if the eeprom area is exactly full.
		int index = 0;
		size /= 2;
		size -= 1;
		for ( pos += 2; index < size; index++ )
		{
			string[ index ] = data[ pos ];
			pos += 2;
		}
		string[ index ] = 0;	//	Terminate the string.
	}

	return pos;
}

//	Change this to Dongle& tdongle and fill out tdongle?
int rtlsdr::FullEepromParse( const eepromdata& data
						   , Dongle& tdongle
						   )
{
	//	Pick apart some of the 9 lead bytes.
	//  typically 28 32 da 0b 32 28 a5 16 02
	//	The first two bytes the rtl 2832 identifier
	if (( data[ 0 ] != 0x28 ) || ( data[ 1 ] != 0x32 ))
	{
		TRACE( "Error: invalid RTL2832 EEPROM header!\n" );
		return -1;
	}
	//	The next four are the VID and PID LSB first.
	tdongle.vid = data[ 2 ] | data[ 3 ] << 8;
	tdongle.pid = data[ 4 ] | data[ 5 ] << 8;

	//	dat[6] == 0xa5 means have_serial
	//	dat[7] & 0x01 means remote wakeup
	//	dat[7] & 0x02 means enable IR
	//	Simply preserve these bits for writing. We do not use them in the registry.

	//	Get the strings
	int r = GetEepromStrings( data
							, &tdongle.manfIdCStr
							, &tdongle.prodIdCStr
							, &tdongle.sernIdCStr
							);

	//	Get port nums for where the dongle is.
	libusb_device *	device       = libusb_get_device( devh );
	usbportnums portnums = { 0 };

#if 1 || defined( PORT_PATH_WORKS_PORTNUMS_DOESNT )
	//	This works for x64 even though it's deprecated.
	int cnt = libusb_get_port_path( ctx
								  , device
								  , portnums
								  , MAX_USB_PATH
								  );
#else
	//	This fails even though it is the "proper" way.
	cnt = libusb_get_port_numbers( devlist[ i ], portnums, MAX_USB_PATH );
#endif
	if ( cnt > 0 )
	{
		// We have good portnums. Clear libusb messup.
		for( int j = cnt; j < 7; j++ )
			portnums[ j ] = 0;
		memcpy( tdongle.usbpath
			  , portnums
			  , min( sizeof( portnums ), sizeof( tdongle.usbpath ))
			  );
	}
	else
	{
		memset( tdongle.usbpath
			  , 0
  			  , min( sizeof( portnums ), sizeof( tdongle.usbpath ))
			  );

		TRACE( "	Dongle path is empty!\n" );
	}

	return 0;
}

int rtlsdr::SafeFindDongle( const Dongle& tdongle )
{
//	CMutexLock cml( registry_mutex );	// Always done within this lock

	if ( Dongles.GetSize() == 0 )
		ReadRegistry();

	for ( int entry = 0; entry < Dongles.GetSize(); entry++ )
	{
		//	Make sure the dongle we're testing is marked not busy.
		bool donglebusy = Dongles[ entry ].busy;
		Dongles[ entry ].busy = false;
		if ( tdongle == Dongles[ entry ])
		{
			Dongles[ entry ].busy = donglebusy;
			return entry;
		}
		Dongles[ entry ].busy = donglebusy;
	}
	return -1;
}


// STATIC //
int rtlsdr::rtlsdr_get_index_by_serial( const char *serial )
{
	int i;
	int cnt;
	int r;
	char str[ 256 ];

	if ( !serial )
		return -1;

	cnt = rtlsdr_get_device_count();

	if ( !cnt )
		return -2;

	CMutexLock cml( dongle_mutex );

	for ( i = 0; i < cnt; i++ )
	{
		r = rtlsdr_get_device_usb_strings( i, NULL, NULL, str );
		if (( r == 0 ) && (strcmp( serial, str ) == 0 ))
			return i;
	}

	return -3;
}


//	This should be run from inside a dongle critical section lock.
// STATIC //
int rtlsdr::GetDongleIndexFromNames( const char * manufact
								   , const char * product
								   , const char * serial
								   )
{
	CString mfg( manufact );
	CString prd( product );
	CString ser( serial );
	return GetDongleIndexFromNames( mfg
								  , prd
								  , ser
								  );
}

// STATIC //
int rtlsdr::GetDongleIndexFromNames( const CString& manufact
								   , const CString& product
								   , const CString& serial
								   )
{
	for( int i = 0; i < Dongles.GetSize(); i++ )
	{
		Dongle *dingle = &Dongles[ i ];
		if (( dingle->manfIdCStr == manufact )
		&&	( dingle->prodIdCStr == product )
		&&	( dingle->sernIdCStr == serial ))
			return i;
	}
	return -1;
}

// STATIC //	Do this within the dongles_lock mutex
int rtlsdr::GetDongleIndexFromDongle( const Dongle dongle )
{
	CMutexLock cml( dongle_mutex );
	for( int i = 0; i < Dongles.GetSize(); i++ )
	{
		if ( Dongles[ i ] == dongle )
			return i;
	}
	return -1;
}
