#include "StdAfx.h"
#define LIBSDRNAMES
#include "librtlsdr.h"

//	This file coordinates the dongle database and its Windows Registry
//	use for the rtlsdr class in the rtlsdr.dll project.

time_t			rtlsdr::lastCatalog = 0;
bool			rtlsdr::noCatalog = false;
bool			rtlsdr::goodCatalog = false;

#define REGISTRYBASE	_T( "Software\\rtlsdr" )

CMyMutex	rtlsdr::registry_mutex( _T( "RtlSdr++ Mutex" ));
CMyMutex	rtlsdr::dongle_mutex;


//	STATIC	//
void rtlsdr::FillDongleArraysAndComboBox( CDongleArray * da
										, CComboBox * combo
										, LPCTSTR selected
										)
{
//	rtlsdr::
		GetCatalog();
	if ( goodCatalog )
	{
		if ( combo )
			combo->ResetContent();
		if ( da )
			da->RemoveAll();

		CMutexLock cml( dongle_mutex );

		INT_PTR comboselected = -1;
		for( DWORD index = 0; index < RtlSdrArea->activeEntries; index++ )
		{
			Dongle* dongle = &Dongles[ index ];
			if ( dongle->found < 0 )
				continue;
			if ( combo )
			{
				CStringA msg;
				if (dongle->busy )
					msg.Format( "* %s,%s,sn %s", dongle->manfIdStr, dongle->prodIdStr, dongle->sernIdStr );
				else
					msg.Format( "%s, %s, sn %s", dongle->manfIdStr, dongle->prodIdStr, dongle->sernIdStr );
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
	if ((DWORD) devindex >= RtlSdrArea->activeEntries )
		return false;
//	rtlsdr::
		GetCatalog();
	if ( goodCatalog )
	{
		Dongle* dongle = NULL;
		int tindex = 0;
		for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
		{
			if ( Dongles[ i ].found == devindex )
			{
				dongle = &Dongles[ i ];
				string.Format( _T( "%s, %hs, %hs" )
							 , dongle->manfIdStr
							 , dongle->prodIdStr
							 , dongle->sernIdStr
							 );
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
	for ( int i = 0; i < (int) RtlSdrArea->activeEntries; i++ )
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
		// First initiate any global values the various classes have.
		DWORD resVal;
		DWORD keytype = REG_DWORD;
		DWORD Size = sizeof( DWORD );
		DWORD debugfile = true;
		// So now we read values we need.
		CString name;
		for ( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
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
				TRACE( _T( " error setting ThreadPriority value!" ));
		}
		RegCloseKey( hRtlsdrKey );
	}
	RtlSdrArea->MasterUpdate = true;
}

//	Call only from inside a mutex lock.
//	STATIC //
uint32_t rtlsdr::WriteSingleRegistry( int index )
{
	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;
	DWORD	status;

	if ( index < 0 )
		return -1;

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
			TRACE( _T( " error resetting %s data %d!" ), name, resVal );
		RegCloseKey( hRtlsdrKey );
		WriteRegLastCatalogTime();
	}
	RtlSdrArea->MasterUpdate = true;
	return resVal;
}

//	STATIC //
void rtlsdr::WriteRegLastCatalogTime( void )
{
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
			TRACE( _T( "error resetting LastCatalog data %d!" ), resVal );
		RegCloseKey( hRtlsdrKey );
	}
}

void rtlsdr::ReadRegistry( void )
{
	if ( TestMaster())
		return;					//  We have known good data already.

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
		// First initiate any global values the various classes have.
		Return returned;
		DWORD resVal;
		DWORD keytype = REG_DWORD;
		DWORD Size = sizeof( DWORD );
		DWORD debugfile = true;
		// So now we read values we need.
		CString name;
		memset( Dongles, 0, sizeof( Dongle ) * MAX_DONGLES );
		RtlSdrArea->activeEntries = 0;
//		Dongles.RemoveAll();
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
				Dongle dongle( returned );
				//	Some protection for a messed up registry.
				if ( returned.pid != returned.vid )
					Dongles[ RtlSdrArea->activeEntries++ ] = returned;
				Size = 0;
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
			// Implicit Dongle = Return
			lastCatalog = max( value, 1 );
		}
		else
			lastCatalog = 1;

		RegCloseKey( hRtlsdrKey );
	}
}

void rtlsdr::mergeToMaster( Dongle& tempd )
{
	CMutexLock cml( registry_mutex );

	for ( DWORD mast = 0; mast < RtlSdrArea->activeEntries; mast++ )
	{
		Dongle md = Dongles[ mast ];
		if ( tempd == md )
		{
			md.busy = tempd.busy;
			md.found = tempd.found;
			if ( !tempd.duplicated )
				memcpy( md.usbpath, tempd.usbpath, MAX_USB_PATH );
			else
			{
				if ( memcmp( md.usbpath, tempd.usbpath, MAX_USB_PATH ) != 0 )
				{
					continue;
				}
			}
			return;
		}
	}

	Dongles[ RtlSdrArea->activeEntries++ ] = tempd;
}


// STATIC //
const char* rtlsdr::find_requested_dongle_name( libusb_context *ctx
											  , uint32_t index
											  )
{
	libusb_device **list;
	struct libusb_device_descriptor dd;
	const rtlsdr_dongle_t *dongle = NULL;
	const char * donglename;

	uint32_t cnt = (uint32_t) libusb_get_device_list( ctx, &list );

	for ( uint32_t i = 0; i < cnt; i++ )
	{
		libusb_get_device_descriptor( list[ i ], &dd );

		dongle = find_known_device( dd.idVendor, dd.idProduct );

		if ( dongle )
		{
			BYTE portnums[ MAX_USB_PATH ] = { 0 };
			int portcnt = libusb_get_port_numbers( list[ i ]
												 , portnums
												 , MAX_USB_PATH
												 );
			if ( portcnt > 0 )
			{
				if ( memcmp( portnums
						   , &Dongles[ index ].usbpath
						   , portcnt
						   ) == 0 )
					break;
			}
		}
		dongle = NULL;
	}

	libusb_free_device_list( list, 1 );
	bool busy = false;
	if ( dongle == NULL )
	{
		busy = true;
		if ((DWORD) index < (DWORD) RtlSdrArea->activeEntries )
		{
			//	Hail Mary - and figure the dongle is busy.
			dongle = find_known_device( Dongles[ index ].vid, Dongles[ index ].pid );
		}
		if ( dongle == NULL )
			return "";
	}

	donglename = dongle->name;
	if ( !busy )//&& !test_busy( index )) Cannot do test_busy here because dev already open.
	{
		//  Skip past the "* " busy indicator.
		donglename += 2;
	}
	
	return donglename;
}


// STATIC //
int rtlsdr::open_requested_device( libusb_context *ctx
								 , uint32_t index
								 , libusb_device_handle **ldevh
								 , bool devindex
								 )
{
	libusb_device **list;
	struct libusb_device_descriptor dd;
	libusb_device *device = NULL;
	int r = -1;

	uint32_t cnt = (uint32_t) libusb_get_device_list( ctx, &list );

	if ( devindex )
	{
		device = list[ index ];
	}
	else
	{
		for ( uint32_t i = 0; i < cnt; i++ )
		{
			device = NULL;
			device = list[ i ];
			libusb_get_device_descriptor( list[ i ], &dd );
			TRACE( "Device list %d, vid %x, pid %x\n", i, dd.idVendor, dd.idProduct );

			if (( find_known_device( dd.idVendor, dd.idProduct ) )
			&&	( index < (uint32_t) RtlSdrArea->activeEntries ))
			{
				TRACE( "  Found known device \"index\" %d, i %d\n", index, i );
				BYTE portnums[ MAX_USB_PATH ] = { 0 };
				int portcnt = libusb_get_port_numbers( list[ i ]
													 , portnums
													 , MAX_USB_PATH
													 );
				if ( portcnt > 0 )
				{
					if ( memcmp( portnums
							   , &Dongles[ index ].usbpath
							   , portcnt
							   ) == 0 )
					{
						TRACE( "    Successful port match\n" );
						//	This is done here so that if a dongle does not open
						//	the software will look for a second match. JellyImages
						//	seems to have a Dell computer which produces two
						//	device list entries (2 and 4) for the same dongle.
						//	And the first one does not work.
						r = libusb_open( device, ldevh );
						if ( r >= 0 )
							break;
					}
					else
					{
						TRACE( "   Failed USB port match\n" );
					}
				}
			}
			device = NULL;
		}
	}

	if ( device )
	{
//		r = libusb_open( device, ldevh );
		if ( r < 0 )
		{
			ldevh = NULL;
			fprintf( stderr, "usb_open error %d\n", r );
			TRACE( "open_requested_device (%d) error %d\n", index, r );
			if ( r == LIBUSB_ERROR_ACCESS )
			{
				fprintf( stderr, "Please fix the device permissions, e.g. "
								 "by installing the udev rules file "\
								 "rtl-sdr.rules\n" );
				TRACE( "Please fix the device permissions, e.g. "
					   "by installing the udev rules file rtl-sdr.rules\n");
			}
		}
	}

	//	This must be done after the call.
	libusb_free_device_list( list, 1 );

	if ( ldevh == NULL )
		rtlsdr_close();

	return r;
}

// STATIC //
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
	if ( string )
	{
		TCHAR* tdata = string->GetBuffer( 256 );
		memset( tdata, 0, 256 );
		for( int i = pos + 2; i < pos + size; i++ )
		{
			*tdata++ += data[ i++ ];
		}
		string->ReleaseBuffer();
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
	int r = GetEepromStrings( (BYTE*) data
							, sizeof( eepromdata )
							, tdongle.manfIdStr
							, tdongle.prodIdStr
							, tdongle.sernIdStr
							);
	int len = (int) strnlen_s( tdongle.manfIdStr, MAX_STR_SIZE );
	memset( &tdongle.manfIdStr[ len ], 0, MAX_STR_SIZE - len );
	len = (int) strnlen_s( tdongle.prodIdStr, MAX_STR_SIZE );
	memset( &tdongle.prodIdStr[ len ], 0, MAX_STR_SIZE - len );
	len = (int) strnlen_s( tdongle.sernIdStr, MAX_STR_SIZE );
	memset( &tdongle.sernIdStr[ len ], 0, MAX_STR_SIZE - len );
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

	if ( RtlSdrArea->activeEntries == 0 )
		ReadRegistry();

	for ( DWORD entry = 0; entry < RtlSdrArea->activeEntries; entry++ )
	{
		//	Make sure the dongle we're testing is marked not busy.
		bool donglebusy = Dongles[ entry ].busy;
		Dongles[ entry ].busy = false;
		bool ourboy  = tdongle == Dongles[ entry ];
		Dongles[ entry ].busy = donglebusy;
		if ( ourboy )
		{
			return entry;
		}
	}
	return -1;
}


int rtlsdr::rtlsdr_get_index_by_serial( const char *serial )
{
	return srtlsdr_get_index_by_serial( serial );
}

// STATIC //
int rtlsdr::srtlsdr_get_index_by_serial( const char *serial )
{
	int i;
	int cnt;
	int r;
	char str[ 256 ];

	if ( !serial )
		return -1;

	cnt = srtlsdr_get_device_count();

	if ( !cnt )
		return -2;

	CMutexLock cml( dongle_mutex );

	for ( i = 0; i < cnt; i++ )
	{
		r = srtlsdr_get_device_usb_strings( i, NULL, NULL, str );
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
	for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
	{
		Dongle *dingle = &Dongles[ i ];
		//	possible TCHAR fix.
		CString manfIdStr( dingle->manfIdStr );
		CString prodIdStr( dingle->prodIdStr );
		CString sernIdStr( dingle->sernIdStr );
		if (( manfIdStr == manufact )
		&&	( prodIdStr == product )
		&&	( sernIdStr == serial ))
			return i;
	}
	return -1;
}

// STATIC //	Do this within the dongles_lock mutex
int rtlsdr::GetDongleIndexFromDongle( const Dongle dongle )
{
	CMutexLock cml( dongle_mutex );
	for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
	{
		if ( Dongles[ i ] == dongle )
			return i;
	}
	return -1;
}

// STATIC //	Fill an eepromdata package from regustry data.
int	rtlsdr::srtlsdr_eep_img_from_Dongle( eepromdata&	dat
									   , Dongle*		regentry
									   )
{
	memset( dat, 0, sizeof( eepromdata ));
	dat[ 0 ] = 0x28;
	dat[ 1 ] = 0x32;
	dat[ 2 ] = (uint8_t) ( regentry->vid & 0xff );
	dat[ 3 ] = (uint8_t) ( regentry->vid >> 8 );
	dat[ 4 ] = (uint8_t) ( regentry->pid & 0xff );
	dat[ 5 ] = (uint8_t) ( regentry->pid >> 8 );
	//	6, 7, 8 = 0;
	uint8_t pos = STR_OFFSET;

	CString manfIdCStr( regentry->manfIdStr );
	int count = manfIdCStr.GetLength();
	uint8_t size = (uint8_t) (( count + 1 ) * sizeof( wchar_t ));
	LPCTSTR chars = (LPCTSTR) manfIdCStr;
	dat[ pos ] = size;
	dat[ pos + 1 ] = 0x03;	// mandatory
	if ( sizeof( eepromdata ) < ( pos + size + 4 ))	// +4 allows 2 empty strings
	{
		return -1;			//	Can't go on. Registry is evil.
	}

	wchar_t* chrdat = (wchar_t*) &dat[ pos + 2 ];
	for( int i = 0; i < count; i++ )
	{
		*chrdat++ = chars[ i ];
	}
	pos += size;

	CString prodIdCStr( regentry->prodIdStr );
	count = prodIdCStr.GetLength();
	size = (uint8_t) (( count + 1 ) * sizeof( wchar_t ));
	chars = (LPCTSTR) prodIdCStr;
	dat[ pos ] = size;
	dat[ pos + 1 ] = 0x03;	// mandatory
	if ( sizeof( eepromdata ) < ( pos + size + 2 ))	// +2 allows 1 empty strings
	{
		return -1;			//	Can't go on. Registry is evil.
	}

	chrdat = (wchar_t*) &dat[ pos + 2 ];
	for( int i = 0; i < count; i++ )
	{
		*chrdat++ = chars[ i ];
	}
	pos += size;

	CString sernIdCStr( regentry->sernIdStr );
	count = sernIdCStr.GetLength();
	size = (uint8_t) (( count + 1 ) * sizeof( wchar_t ));
	chars = (LPCTSTR) sernIdCStr;
	dat[ pos ] = size;
	dat[ pos + 1 ] = 0x03;	// mandatory
	if ( sizeof( eepromdata ) < ( pos + size ))
	{
		return -1;			//	Can't go on. Registry is evil.
	}

	chrdat = (wchar_t*) &dat[ pos + 2 ];
	for( int i = 0; i < count; i++ )
	{
		*chrdat++ = chars[ i ];
	}

	return pos + size;
}


int rtlsdr::FindInMasterDB( Dongle* dng, bool exact )
{
	for( int i = 0; i < (int) RtlSdrArea->activeEntries; i++ )
	{
		Dongle *test = &Dongles[ i ];
		if (( strcmp( dng->manfIdStr, test->manfIdStr ) == 0 )
		&&	( strcmp( dng->prodIdStr, test->prodIdStr ) == 0 )
		&&	( strcmp( dng->sernIdStr, test->sernIdStr ) == 0 )
		&&	( dng->vid == test->vid )
		&&	( dng->pid == test->pid ))
		{
			if ( !exact )
				return i;
			else
			if ( memcmp( dng->usbpath, test->usbpath, MAX_USB_PATH ) == 0 )
				return i;
		}
		i = i;
	}
	return -1;
}


int rtlsdr::FindGuessInMasterDB( Dongle* dng )
{
	for( int i = 0; i < (int) RtlSdrArea->activeEntries; i++ )
	{
		Dongle *test = &Dongles[ i ];
		if (( dng->vid == test->vid )
		&&	( dng->pid == test->pid )
		&&	( memcmp( dng->usbpath, test->usbpath, MAX_USB_PATH ) == 0 ))
		{
			return i;
		}
		i = i;
	}
	return -1;
}

