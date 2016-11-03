#include "StdAfx.h"
#define LIBSDRNAMES
#include "RtlSdrList.h"
#include "MyMutex.h"

SharedMemoryFile	SharedDongleData;
RtlSdrAreaDef*		RtlSdrArea = NULL;
Dongle*				Dongles = NULL;

CMyMutex			RtlSdrList::registry_mutex( _T( "RtlSdr++ Mutex" ));

#define CATALOG_TIMEOUT		5		//	Not much changes in 5 seconds with dongles.
#define STR_OFFSET			9		// EEPROM string offset.

#define REGISTRYBASE	_T( "Software\\rtlsdr" )

enum usb_reg
{
	USB_SYSCTL		    = 0x2000,
	USB_CTRL		    = 0x2010,
	USB_STAT		    = 0x2014,
	USB_EPA_CFG		    = 0x2144,
	USB_EPA_CTL		    = 0x2148,
	USB_EPA_MAXPKT		= 0x2158,
	USB_EPA_MAXPKT_2	= 0x215a,
	USB_EPA_FIFO_CFG	= 0x2160,
};

enum sys_reg {
	DEMOD_CTL		= 0x3000,
	GPO			    = 0x3001,
	GPI			    = 0x3002,
	GPOE			= 0x3003,
	GPD			    = 0x3004,
	SYSINTE			= 0x3005,
	SYSINTS			= 0x3006,
	GP_CFG0			= 0x3007,
	GP_CFG1			= 0x3008,
	SYSINTE_1		= 0x3009,
	SYSINTS_1		= 0x300a,
	DEMOD_CTL_1		= 0x300b,
	IR_SUSPEND		= 0x300c,
};

enum blocks {
	DEMODB			= 0,
	USBB			= 1,
	SYSB			= 2,
	TUNB			= 3,
	ROMB			= 4,
	IRB				= 5,
	IICB			= 6,
};

#define CTRL_IN		( LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN )
#define CTRL_OUT	( LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT )
#define CTRL_TIMEOUT	300

#define DEFAULT_BUF_NUMBER	15
#define DEFAULT_BUF_LENGTH	( 16 * 32 * 512 )

#define EEPROM_ADDR	0xa0


RtlSdrList::RtlSdrList( void )
	: lastCatalog( 0 )
	, ctx( NULL )
	, devh( NULL )
	, tuner_type( RTLSDR_TUNER_UNKNOWN )
	, i2c_repeater_on( false )
	, goodCatalog( false )
	, dev_lost( false )
{
	if ( RtlSdrArea == NULL )
	{
		if ( SharedDongleData.active )
		{
			RtlSdrArea = SharedDongleData.sharedMem;
			Dongles = RtlSdrArea->dongleArray;
		}
		else
		{
			AfxMessageBox( _T( "No shared memory area!" ));
		}
	}
}


RtlSdrList::~RtlSdrList( void )
{
}


int RtlSdrList::GetCount( void )
{
	return RtlSdrArea->activeEntries;
}


bool RtlSdrList::GetStrings( int index
						   , CString& man
						   , CString& prd
						   , CString& ser
						   )
{
	if ( (ULONG) index >= RtlSdrArea->activeEntries )
		return false;
	//	Automatic conversion takes place.
	man = Dongles[ index ].manfIdStr;
	prd = Dongles[ index ].prodIdStr;
	ser = Dongles[ index ].sernIdStr;
	return true;
}

void RtlSdrList::GetCatalog( void )
{
	// Make sure shared memory areas are loaded for this.
	if ( RtlSdrArea == NULL )
	{
		if ( SharedDongleData.active )
		{
			RtlSdrArea = SharedDongleData.sharedMem;
			Dongles = SharedDongleData.sharedMem->dongleArray;
		}
		else
		{
			RtlSdrArea = &TestDongles;
			Dongles = TestDongles.dongleArray;
		}
	}

	ReadRegistry();
	//	Now I must run through the interfaces and merge data.
	reinitDongles();
	lastCatalog = time( NULL );
	WriteRegLastCatalogTime();

	goodCatalog = true;
}


void RtlSdrList::ReadRegistry( void )
{
	TRACE( "ReadRegistry\n" );

	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;

	CMutexLock cml( registry_mutex );

	// Just to be sure - clear everthing here

	resVal = RegOpenKeyEx( HKEY_CURRENT_USER
						 , REGISTRYBASE
						 , 0
						 , KEY_READ
						 , &hRtlsdrKey
						 );

	if ( resVal == ERROR_SUCCESS ) 
	{
		TRACE( _T( "Found %s key\n" ), REGISTRYBASE );
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
				if ( returned.vid != returned.pid )
					Dongles[ RtlSdrArea->activeEntries++ ] = returned;
				TRACE( "Found %s %s %s", returned.manfIdStr, returned.prodIdStr, returned.sernIdStr );
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


//	This writes the dongle array values out to the registry.
//	We leave old, not replaced, values around so that we don't lose dongles that
//	are temporarily removed. Note that duplicated entries created chiefly by hand
//	editing the registry or by merging exported registry values have been removed
//	in the DongleArray "Add()" function.)
void RtlSdrList::WriteRegistry( void )
{
	CMutexLock cml( registry_mutex );
	DWORD   resVal;
	HKEY    hRtlsdrKey	= NULL;
	DWORD	status = 0;
	// Just to be sure - clear everthing here
	resVal = RegCreateKeyEx( HKEY_CURRENT_USER
						   , REGISTRYBASE	//_T( "Software\\rtlsdr" )
						   , 0
						   , NULL
						   , 0
						   , KEY_ALL_ACCESS | KEY_ENUMERATE_SUB_KEYS
						   , NULL
						   , &hRtlsdrKey
						   , &status		// 1 created key, 2 existing key
						   );
	//
	// If successful, read all the registry entries under this key    
	//
	if ( resVal == ERROR_SUCCESS )
	{
		//	First delete all registry entries.
		CString name;
		for( int i = 0; ; i++ )
		{
			DWORD lastTime;
			TCHAR nameBuf [ MAX_PATH + 1 ] = { 0 };
			DWORD nameBufSize = MAX_PATH;
			resVal = RegEnumValue( hRtlsdrKey
								 , i
								 , nameBuf
								 , &nameBufSize
								 , NULL
								 , NULL
								 , NULL
								 , &lastTime
								 );
			if ( resVal == ERROR_SUCCESS)
			{
				RegDeleteValue( hRtlsdrKey, nameBuf );
				i--;
			}
			else
				break;
		}




		// First initiate any global values the various classes have.
		DWORD resVal;
		DWORD keytype = REG_DWORD;
		DWORD Size = sizeof( DWORD );
		DWORD debugfile = true;
		// So now we read values we need.
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
				TRACE( L" error setting ThreadPriority value!" );
		}
		RegCloseKey( hRtlsdrKey );
	}
}


//	Call only from inside a mutex lock.
//	STATIC //
uint32_t RtlSdrList::WriteSingleRegistry( int index )
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


void RtlSdrList::WriteRegLastCatalogTime( void )
{
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
			TRACE( L" error resetting LastCatalog data %d!", resVal );
		RegCloseKey( hRtlsdrKey );
	}
}


void RtlSdrList::reinitDongles( void )
{
	TRACE( "reinitDongles\n" );
	bool rval = false;
	DWORD temp = 0;
	CDongleArray reinit_dongles;	//	The array of unmatched dongles.

	// Init libusb...
	
	libusb_context * tctx = NULL;
	int retval = libusb_init( &tctx );
	if ( retval < 0 )
	{
		tctx = NULL;
	}
	if ( tctx == NULL )
	{

		libusb_exit( tctx );
		return;
	}


	libusb_device** devlist;
	ssize_t dev_count = libusb_get_device_list( tctx, &devlist );
	TRACE( "Device list dev_count %d\n", dev_count );

	//	Clear Dongles array found devices value to not found, -1.
	for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
	{
		Dongles[ i ].found = -1;
		Dongles[ i ].busy = false;
	}

#if 0	//	This way SORT OF works.
	//	And clear the rest of the dongle area to 0.
	int rtlsdr_count = RtlSdrArea->activeEntries;	// Next entry in table
	if ( dev_count > 0 )
	{
		for( ssize_t i = 0; i < dev_count; i++ )
		{
			libusb_device_descriptor dd;

			int err = libusb_get_device_descriptor( devlist[ i ], &dd );

			const rtlsdr_dongle_t * known = find_known_device( dd.idVendor
															 , dd.idProduct
															 );
			if ( known == NULL )
				continue;

			//	We have a candidate. Start a "Dongle" for it.
			Dongle dongle;

			//	enter the vid, pid, etc into the record.
			dongle.vid		= dd.idVendor;
			dongle.pid		= dd.idProduct;
			dongle.found	= (char) i;

			//	enter some "useful" default strings
			CStringA manf( known->manfname );
			CStringA prod( known->prodname );
			CStringA sern( char( '0' + rtlsdr_count ));
			// TODOTODO testing.
			manf += char( '0' + rtlsdr_count );
			prod += char( '0' + rtlsdr_count );

			//	Find the USB path. If it matches something from Dongles array
			//	we (rashly) can presume a match.
			usbpath_t portnums = { 0 };
#if 1 || defined( PORT_PATH_WORKS_PORTNUMS_DOESNT )
			//	This works for X64 even though it's deprecated.
			int cnt = libusb_get_port_path( tctx
										  , devlist[ i ]
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
				//	enter port path into the record.
				memcpy( dongle.usbpath, portnums, sizeof( usbpath_t ));
			}
			else
			{
				memset( dongle.usbpath, 0, sizeof( usbpath_t ));
				TRACE( "	%d:  Error %d\n", i, err );
			}
#if 0	// For debugging
			CString text;
			text.Format( _T( "xxxxx Device %d 0x%x 0x%x:" ), i, dd.idVendor, dd.idProduct );
			CString t2;
			for( int g = 0; g < cnt; g++ )
			{
				t2.Format( _T( " 0x%x" ), portnums[ g ]);
				text += t2;
			}
			text += _T( "\n" );
			TRACE( text );
#endif

			//	Try to open the device to determine busy status and perhaps
			//	read the name.
			{
				int res = rtlsdr_open((uint32_t) i );
				if ( res >= 0 )
				{
					dongle.busy = false;

					// Figure out if we match strings somewhere - likely reg.
					eepromdata testdata = { 0 };
					res = rtlsdr_read_eeprom_raw( testdata );
					if ( res < 0 )
						res = res;

					//	Parse eeprom data to get the names and eventually add it.
					char* man = manf.GetBuffer( MAX_STR_SIZE );
					char* prd = prod.GetBuffer( MAX_STR_SIZE );
					char* ser = sern.GetBuffer( MAX_STR_SIZE );
					GetEepromStrings( testdata
									, sizeof( eepromdata )
									, man
									, prd
									, ser
									);
					manf.ReleaseBuffer();
					prod.ReleaseBuffer();
					sern.ReleaseBuffer();

					//	We have it open - get the tunertype now?
					dongle.tunerType = tuner_type;
					TRACE( "Sn dng %s\n", sern );
					//	Now run comparisons against only registered dongles.
					bool matched = false;
					DWORD dng = 0;
					for( ; dng < RtlSdrArea->activeEntries; dng++ )
					{
//						TRACE( "Sn dng %s, Sn test %s\n", sern, Dongles[ dng ].sernIdStr );
						if ( CompareSparseRegAndData( &Dongles[ dng ]
													, testdata
													))
						{
//							TRACE( "Compared OK\n" );
							//	Compare USB path....
							if (( portnums[ 0 ] != 0 )
							&&	( memcmp( portnums
										, Dongles[ dng ].usbpath
										, MAX_USB_PATH
										) == 0 )
							   )
							{
//								TRACE( "Matched\n" );
								matched = true;
								rtlsdr_count++;
								Dongles[ dng ].busy = false;
								Dongles[ dng ].found = (int) i;
							}
							else
							{
//								TRACE( "Marked duplicated\n" );
								dongle.duplicated = true;
								Dongles[ dng ].duplicated = true;
								//	This is a different baby. No match even if
								//	the names data match. Continue searching.
							}
						}
						else
						{
//							TRACE( "Compare failed\n" );
						}
					}
					rtlsdr_close();

					if ( matched && !dongle.duplicated )
					{
						continue;
					}

					if ( dng >= RtlSdrArea->activeEntries )
					{
						memcpy( dongle.manfIdStr, manf, manf.GetLength());
						memset( &dongle.manfIdStr[ manf.GetLength()]
							  , 0
							  , MAX_STR_SIZE - manf.GetLength());
						memcpy( dongle.prodIdStr, prod, prod.GetLength());
						memset( &dongle.prodIdStr[ prod.GetLength()]
							  , 0
							  , MAX_STR_SIZE - prod.GetLength());
						memcpy( dongle.sernIdStr, sern, sern.GetLength());
						memset( &dongle.sernIdStr[ sern.GetLength()]
							  , 0
							  , MAX_STR_SIZE - sern.GetLength());
						memcpy( dongle.usbpath, portnums, MAX_USB_PATH );
						dongle.found = (int) i;
					}
					else
					{
						//	Nothing matched so add it to reinit_dongles and move on.
						//	Parse eeprom data to get the names and eventually add it.
						char* manfs = manf.GetBuffer( 256 );
						char* prods = prod.GetBuffer( 256 );
						char* serns = sern.GetBuffer( 256 );
						GetEepromStrings( testdata, 256, manfs, prods, serns );
						manf.ReleaseBuffer();
						prod.ReleaseBuffer();
						sern.ReleaseBuffer();
					}
				}
				else
				{
					//	Test if the paths match in the data merge.
					dongle.busy = true;
				}
			}

			// Build strings into dongle entry.
			memset( dongle.manfIdStr, 0, sizeof( dongle.manfIdStr ));
			strcpy_s( dongle.manfIdStr, manf );
			memset( dongle.prodIdStr, 0, sizeof( dongle.prodIdStr ));
			strcpy_s( dongle.prodIdStr, prod );
			memset( dongle.sernIdStr, 0, sizeof( dongle.sernIdStr ));
			strcpy_s( dongle.sernIdStr, sern );

			reinit_dongles.Add( dongle );

			//	Items that have gotten to hear are either new or busy.
			//	Loop merging after we're all done here.

			rtlsdr_count++;
		}
		libusb_free_device_list( devlist, 1 );
		libusb_exit( tctx );

		//	Merge new dongle array with old dongle array here.
		for ( int i = 0; i < reinit_dongles.GetSize(); i++ )
		{
			Dongle dongle = reinit_dongles[ i ];
			//	At this point if a dongle is marked duplicated it may simply be
			//	a dongle moved to another port. Test for two name matches on the
			//	same dongle. If there is only one then fix the port data - which
			//	may be awkward.
			//	TODOTODO
			int count = 0;
			int lf = -1;
			TRACE( "Orphan dongle %d, found %d busy %d duplicated %d", i, dongle.found, dongle.busy, dongle.duplicated );
			for ( ULONG j = 0; j < RtlSdrArea->activeEntries; j++ )
			{
				Dongle* dtest = &Dongles[ j ];
				if (( dongle == *dtest )
				&&	( strncmp( dongle.manfIdStr, dtest->manfIdStr, MAX_STR_SIZE ) == 0 )
				&&	( strncmp( dongle.prodIdStr, dtest->prodIdStr, MAX_STR_SIZE ) == 0 )
				&&	( strncmp( dongle.sernIdStr, dtest->sernIdStr, MAX_STR_SIZE ) == 0 )
				&&	( dongle.vid == dtest->vid )
				&&	( dongle.pid == dtest->pid )
				&&	( dongle.tunerType == dtest->tunerType )
				   )
				{
					lf = j;
					count++;
				}
			}
			if ( count == 1 )
			{
				// Correct the port information
				memcpy( Dongles[ lf ].usbpath, dongle.usbpath, MAX_USB_PATH );
			}
			else
				mergeToMaster( dongle );
		}
		rval = true;
		WriteRegistry();
	}
#else
	if ( dev_count > 0 )
	{
		for( ssize_t i = 0; i < dev_count; i++ )
		{
			libusb_device_descriptor dd;

			int err = libusb_get_device_descriptor( devlist[ i ], &dd );

			const rtlsdr_dongle_t * known = find_known_device( dd.idVendor
															 , dd.idProduct
															 );
			if ( known == NULL )
				continue;

			//	We have a candidate. Start a "Dongle" for it.
			Dongle dongle;

			//	enter the vid, pid, etc into the record.
			dongle.vid		= dd.idVendor;
			dongle.pid		= dd.idProduct;
			dongle.found	= (char) i;

			//	enter some "useful" default strings
			CStringA manf;
			CStringA prod;
			CStringA sern;

			//	Find the USB path. If it matches something from Dongles array
			//	we (rashly) can presume a match.
			usbpath_t portnums = { 0 };
#if 1 || defined( PORT_PATH_WORKS_PORTNUMS_DOESNT )
			//	This works for X64 even though it's deprecated.
			int cnt = libusb_get_port_path( tctx
										  , devlist[ i ]
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
				//	enter port path into the record.
				memcpy( dongle.usbpath, portnums, sizeof( usbpath_t ));
			}
			else
			{
				memset( dongle.usbpath, 0, sizeof( usbpath_t ));
				TRACE( "	%d:  Error %d\n", i, err );
			}
#if 0	// For debugging
			CString text;
			text.Format( _T( "xxxxx Device %d 0x%x 0x%x:" ), i, dd.idVendor, dd.idProduct );
			CString t2;
			for( int g = 0; g < cnt; g++ )
			{
				t2.Format( _T( " 0x%x" ), portnums[ g ]);
				text += t2;
			}
			text += _T( "\n" );
			TRACE( text );
#endif

			int res = rtlsdr_open((uint32_t) i );
			if ( res >= 0 )
			{
				dongle.busy = false;	//	For comparisons....
				eepromdata testdata = { 0 };
				res = rtlsdr_read_eeprom_raw( testdata );
				if ( res < 0 )
				{
					MessageBox( 0
							  , _T( "Cannot read EEPROM." )
							  , _T( "EEPROM Error" )
							  , MB_ICONEXCLAMATION
							  );
					continue;			//	We're screwed!
				}

				//	Parse eeprom names from raw data.
				//	This nonsense is handy for tests below
				char* man = manf.GetBuffer( MAX_STR_SIZE );
				char* prd = prod.GetBuffer( MAX_STR_SIZE );
				char* ser = sern.GetBuffer( MAX_STR_SIZE );
				GetEepromStrings( testdata
								, sizeof( eepromdata )
								, man
								, prd
								, ser
								);
				manf.ReleaseBuffer();
				prod.ReleaseBuffer();
				sern.ReleaseBuffer();
				memset( dongle.manfIdStr, 0, MAX_STR_SIZE );
				memcpy( dongle.manfIdStr, manf, manf.GetLength());
				memset( dongle.prodIdStr, 0, MAX_STR_SIZE );
				memcpy( dongle.prodIdStr, prod, prod.GetLength());
				memset( dongle.sernIdStr, 0, MAX_STR_SIZE );
				memcpy( dongle.sernIdStr, sern, sern.GetLength());

				//	We have it open - get the tunertype now
				dongle.tunerType = tuner_type;
				TRACE( "Sn dng %s\n", sern );
				rtlsdr_close();
				dongle.found = (int) i;

				for( int t = 0; t < reinit_dongles.GetSize(); t++)
				{
					Dongle* test = &reinit_dongles[ t ];

					if (( manf.Compare( test->manfIdStr ) == 0 )
					&&	( prod.Compare( test->prodIdStr ) == 0 )
					&&	( sern.Compare( test->sernIdStr ) == 0 )
					   )
					{
						test->duplicated = true;
						dongle.duplicated = true;
					}
				}
				reinit_dongles.Add( dongle );
			}
			//	else can't open it so ignore it. We'll find it some time later.
			else
			{
				//	Well, we can try for vid, pid, path match. If that matches
				//	then "guess" this is a known dongle but leave names blank
				//	and flag busy.
				int dbindex;
				if (( dbindex = FindGuessInMasterDB( &dongle )) >= 0 )
				{
					dongle = Dongles[ dbindex ];
					dongle.busy = true;
					dongle.found = (int) i;
					reinit_dongles.Add( dongle );
				}
			}
		}	//	/for

		//	We're done with libusb in here.
		libusb_free_device_list( devlist, 1 );
		libusb_exit( tctx );
		//	Now merge this data into our database.
		int dbindex = -1;
		while( reinit_dongles.GetSize() > 0 )
		{
			if ( reinit_dongles.GetSize() == 1 )
				dbindex = dbindex;
			Dongle test = reinit_dongles[ 0 ];
			if (( dbindex = FindInMasterDB( &test, true )) >= 0 )
			{
				//	Simply remove it from local db - do nothing right here.
				Dongles[ dbindex ].busy = test.busy;
				Dongles[ dbindex ].found = test.found;
			}
			else
			if (( dbindex = FindInMasterDB( &test, false )) < 0 )
			{
				//	Name not even present so add it to db.
				Dongles[ RtlSdrArea->activeEntries ] = test;
				WriteSingleRegistry( RtlSdrArea->activeEntries );
				RtlSdrArea->activeEntries++;
			}
			else
			if ( !test.duplicated )
			{
				//	Name found in DB but not duplicated. Fix masterdb path data
				Dongle* tempd = &Dongles[ dbindex ];
				memcpy( tempd->usbpath, test.usbpath, MAX_USB_PATH );
				Dongles[ dbindex ].busy = test.busy;
				Dongles[ dbindex ].found = test.found;
				WriteSingleRegistry( dbindex );
			}
			else
			{
				//  Duplicated in local db and !exact match and not in db
				//  So we have a duplicate - let's deal with it.
				//  It might be a set of new dongles that duplicate what we have
				//  or it might be a new duplicate matching one entry we already
				//	have. Suppose we increment sn by 1 and retest for matches.
				//	Repeat until no match and add to database.
				int len = (int) strlen( test.sernIdStr );
				bool exact = false;		// True if exact match
				if ( len > 0 )
				{
					do
					{
						test.sernIdStr[ len ]++;
						dbindex = FindInMasterDB( &test, true );
						if ( dbindex >= 0 )
						{
							exact = true;
							//	Write fixed name to eeprom	TODOTODO
							break;
						}
					} while( FindInMasterDB( &test, false ) >= 0 );
					if ( !exact )
						Dongles[ RtlSdrArea->activeEntries++ ] = test;
				}
			}
			reinit_dongles.RemoveAt( 0 );
		}
	}
#endif	//	This way SORT OF works.
	return;
}


const rtlsdr_dongle_t * RtlSdrList::find_known_device( uint16_t vid
													 , uint16_t pid
													 )
{
	unsigned int i;
	const rtlsdr_dongle_t *device = NULL;

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


int RtlSdrList::rtlsdr_open( uint32_t devindex )
{
	if (( devh != NULL ) || ( ctx != NULL )) 
		return -10;

	int r;
	uint8_t  reg;

	ClearVars();

	r = basic_open( devindex );

	if ( r < 0 )
		goto err;

	//	Now we "really" open the device and discover the tuner type.

	rtlsdr_init_baseband();
	dev_lost = false;

	/* Probe tuners */
	tuner_type = RTLSDR_TUNER_UNKNOWN;
	rtlsdr_set_i2c_repeater( 1 );

#define E4K_I2C_ADDR	0xc8
#define E4K_CHECK_ADDR	0x02
#define E4K_CHECK_VAL	0x40

	reg = rtlsdr_i2c_read_reg( E4K_I2C_ADDR, E4K_CHECK_ADDR );
	if ( reg == E4K_CHECK_VAL )
	{
		TRACE( "Found Elonics E4000 tuner\n");
		tuner_type = RTLSDR_TUNER_E4000;
		goto found;
	}

#define FC0013_I2C_ADDR		0xc6
#define FC0013_CHECK_ADDR	0x00
#define FC0013_CHECK_VAL	0xa3
	reg = rtlsdr_i2c_read_reg( FC0013_I2C_ADDR, FC0013_CHECK_ADDR );
	if ( reg == FC0013_CHECK_VAL )
	{
		TRACE( "Found Fitipower FC0013 tuner\n");
		tuner_type = RTLSDR_TUNER_FC0013;
		goto found;
	}

#define R820T_I2C_ADDR		0x34
#define R828D_I2C_ADDR		0x74
#define R828D_XTAL_FREQ		16000000

#define R82XX_CHECK_ADDR	0x00
#define R82XX_CHECK_VAL		0x69

	reg = rtlsdr_i2c_read_reg( R820T_I2C_ADDR, R82XX_CHECK_ADDR );
	if ( reg == R82XX_CHECK_VAL )
	{
		TRACE( "Found Rafael Micro R820T tuner\n");
		tuner_type = RTLSDR_TUNER_R820T;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg( R828D_I2C_ADDR, R82XX_CHECK_ADDR );
	if ( reg == R82XX_CHECK_VAL )
	{
		TRACE( "Found Rafael Micro R828D tuner\n");
		tuner_type = RTLSDR_TUNER_R828D;
		goto found;
	}

	/* initialise GPIOs */
	rtlsdr_set_gpio_output( 5 );

	/* reset tuner before probing */
	rtlsdr_set_gpio_bit( 5, 1 );
	rtlsdr_set_gpio_bit( 5, 0 );

#define FC2580_I2C_ADDR		0xac
#define FC2580_CHECK_ADDR	0x01
#define FC2580_CHECK_VAL	0x56
	reg = rtlsdr_i2c_read_reg( FC2580_I2C_ADDR, FC2580_CHECK_ADDR );
	if (( reg & 0x7f ) == FC2580_CHECK_VAL )
	{
		TRACE( "Found FCI 2580 tuner\n");
		tuner_type = RTLSDR_TUNER_FC2580;
		goto found;
	}

#define FC0012_I2C_ADDR		0xc6
#define FC0012_CHECK_ADDR	0x00
#define FC0012_CHECK_VAL	0xa1
	reg = rtlsdr_i2c_read_reg( FC0012_I2C_ADDR, FC0012_CHECK_ADDR );
	if ( reg == FC0012_CHECK_VAL )
	{
		TRACE( "Found Fitipower FC0012 tuner\n");
		rtlsdr_set_gpio_output( 6 );
		tuner_type = RTLSDR_TUNER_FC0012;
		goto found;
	}

found:
	/* use the rtl clock value by default */
	rtlsdr_set_i2c_repeater( 0 );
//	m_dongle = Dongles[ index ];		TODOTODO

	return 0;

err:

	rtlsdr_close();

	return r;
}


void RtlSdrList::ClearVars( void )
{
	tuner_type = ( enum rtlsdr_tuner) 0;
	ctx = NULL;
	devh = NULL;
}


int RtlSdrList::basic_open( uint32_t devindex )
{
	if ( devh )
	{
		rtlsdr_close();
	}

	int r;
	int device_count = 0;
	libusb_device *device = NULL;
	libusb_device_handle * ldevh = NULL;

	libusb_init( &ctx );

	dev_lost = true;

	r = open_requested_device( ctx, devindex, &ldevh );

	if ( r < 0 )
		goto err;

	devh = ldevh;

	r = claim_opened_device();

err:
	return r;
}


int RtlSdrList::claim_opened_device( void )
{
	int r;
	if ( libusb_kernel_driver_active( devh, 0 ) == 1 )
	{
		TRACE(  "\nKernel driver is active, or device is "
				"claimed by second instance of librtlsdr."
				"\nIn the first case, please either detach"
				" or blacklist the kernel module\n"
				"(dvb_usb_rtl28xxu), or enable automatic"
				" detaching at compile time.\n\n");
		return -2;
	}

	r = libusb_claim_interface( devh, 0 );
	if ( r < 0 )
	{
		TRACE( "usb_claim_interface error %d\n", r);
		return r;
	}

	return 0;
}


int RtlSdrList::rtlsdr_close( void )
{
	if (( devh == NULL ) && ( ctx == NULL ))
		return -1;

	if ( devh != NULL )
	{

		rtlsdr_set_i2c_repeater( 0 );

		if ( !dev_lost )
		{
			/* block until all async operations have been completed (if any) */

			libusb_release_interface( devh, 0 );

			rtlsdr_deinit_baseband();
		}

		libusb_close( devh );
		devh = NULL;

		int index;
		{
			index = GetDongleIndexFromDongle( m_dongle );
		}

		if ( index >= 0 )
		{
			Dongles[ index ].busy = false;
			WriteSingleRegistry( index );
		}
	}

	if ( ctx != NULL )
	{
		libusb_exit( ctx );
		ctx = NULL;
	}

	return 0;
}


int RtlSdrList::open_requested_device( libusb_context *ctx
									 , uint32_t devindex
									 , libusb_device_handle **ldevh
									 )
{
	libusb_device **list;
	libusb_device *device = NULL;
	int r = -1;

	uint32_t cnt = (uint32_t) libusb_get_device_list( ctx, &list );
	device = list[ devindex ];

	if ( device )
	{
		r = libusb_open( device, ldevh );
		if ( r < 0 )
		{
			ldevh = NULL;
			TRACE( "open_requested_device (%d) error %d\n", devindex, r );
			if ( r == LIBUSB_ERROR_ACCESS )
			{
				TRACE( "Please fix the device permissions, e.g. "
					   "by installing the udev rules file rtl-sdr.rules\n");
			}
		}
	}

	//	This must be done after the call.
	libusb_free_device_list( list, 1 );

	return r;
}


void RtlSdrList::rtlsdr_set_i2c_repeater( int on )
{
	if ( on == i2c_repeater_on )
		return;
	if ( on == -1 )
		on = 0;
	else
		on = !!on; /* values +2 to force on */
	i2c_repeater_on = on;
	int ret = rtlsdr_demod_write_reg( 1, 0x01, on ? 0x18 : 0x10, 1 );
	ASSERT( ret == 0 );
}


int RtlSdrList::rtlsdr_demod_write_reg( uint8_t page
									  , uint16_t addr
									  , uint16_t val
									  , uint8_t len
									  )
{
	if ( devh == NULL )
		return -1;

	int r;
	unsigned char data[ 2 ];
	uint16_t index = 0x10 | page;
	addr = ( addr << 8 ) | 0x20;

	if ( len == 1 )
		data[ 0 ] = val & 0xff;
	else
		data[ 0 ] = val >> 8;

	data[ 1 ] = val & 0xff;

	r = libusb_control_transfer( devh
							   , CTRL_OUT
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	rtlsdr_demod_read_reg( 0x0a, 0x01, 1 );

	return ( r == len ) ? 0 : -1;
}


uint16_t RtlSdrList::rtlsdr_demod_read_reg( uint8_t page
										  , uint16_t addr
										  , uint8_t len
										  )
{
	int r;
	unsigned char data[ 2 ];

	uint16_t index = page;
	uint16_t reg;
	addr = ( addr << 8 ) | 0x20;

	r = libusb_control_transfer( devh
							   , CTRL_IN
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	reg = ( data[ 1 ] << 8 ) | data[ 0 ];

	return reg;
}


void RtlSdrList::rtlsdr_init_baseband( void )
{
	unsigned int i;

	/* initialize USB */
	rtlsdr_write_reg( USBB, USB_SYSCTL, 0x09, 1 );
	rtlsdr_write_reg( USBB, USB_EPA_MAXPKT, 0x0002, 2 );
	rtlsdr_write_reg( USBB, USB_EPA_CTL, 0x1002, 2 );

	/* poweron demod */
	rtlsdr_write_reg( SYSB, DEMOD_CTL_1, 0x22, 1 );
	rtlsdr_write_reg( SYSB, DEMOD_CTL, 0xe8, 1 );

	/* reset demod (bit 3, soft_rst) */
	rtlsdr_demod_write_reg( 1, 0x01, 0x14, 1 );
	rtlsdr_demod_write_reg( 1, 0x01, 0x10, 1 );

	/* disable spectrum inversion and adjacent channel rejection */
	rtlsdr_demod_write_reg( 1, 0x15, 0x00, 1 );
//	spectrum_inversion = 0;		Performed in ClearVars.
	rtlsdr_demod_write_reg( 1, 0x16, 0x0000, 2 );

	/* clear both DDC shift and IF frequency registers  */
	for ( i = 0; i < 6; i++ )
		rtlsdr_demod_write_reg( 1, 0x16 + i, 0x00, 1 );

	/* enable SDR mode, disable DAGC (bit 5) */
	rtlsdr_demod_write_reg( 0, 0x19, 0x05, 1 );

	/* init FSM state-holding register */
	rtlsdr_demod_write_reg( 1, 0x93, 0xf0, 1 );
	rtlsdr_demod_write_reg( 1, 0x94, 0x0f, 1 );

	/* disable AGC (en_dagc, bit 0) (this seems to have no effect) */
	rtlsdr_demod_write_reg( 1, 0x11, 0x00, 1 );

	/* disable RF and IF AGC loop */
	rtlsdr_demod_write_reg( 1, 0x04, 0x00, 1 );

	/* disable PID filter (enable_PID = 0) */
	rtlsdr_demod_write_reg( 0, 0x61, 0x60, 1 );

	/* opt_adc_iq = 0, default ADC_I/ADC_Q datapath */
	rtlsdr_demod_write_reg( 0, 0x06, 0x80, 1 );

	/* Enable Zero-IF mode (en_bbin bit), DC cancellation (en_dc_est),
	 * IQ estimation/compensation (en_iq_comp, en_iq_est) */
	rtlsdr_demod_write_reg( 1, 0xb1, 0x1b, 1 );

	/* disable 4.096 MHz clock output on pin TP_CK0 */
	rtlsdr_demod_write_reg( 0, 0x0d, 0x83, 1 );
}

int RtlSdrList::rtlsdr_deinit_baseband( void )
{
	int r = 0;

	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	/* poweroff demodulator and ADCs */
	rtlsdr_write_reg( SYSB, DEMOD_CTL, 0x20, 1 );

	return r;
}


uint16_t RtlSdrList::rtlsdr_read_reg( uint8_t block
									, uint16_t addr
									, uint8_t len
									)
{
	int r;
	unsigned char data[ 2 ];
	uint16_t index = ( block << 8 );
	uint16_t reg;

	r = libusb_control_transfer( devh
							   , CTRL_IN
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	reg = (data[1] << 8) | data[0];

	return reg;
}

int RtlSdrList::rtlsdr_write_reg( uint8_t block
								, uint16_t addr
								, uint16_t val
								, uint8_t len
								)
{
	int r;
	unsigned char data[ 2 ];

	uint16_t index = ( block << 8 ) | 0x10;

	if ( len == 1 )
		data[ 0 ] = val & 0xff;
	else
		data[ 0 ] = val >> 8;

	data[ 1 ] = val & 0xff;

	r = libusb_control_transfer( devh
							   , CTRL_OUT
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	return r;
}


int RtlSdrList::rtlsdr_read_array( uint8_t block
								 , uint16_t addr
								 , uint8_t *data
								 , uint8_t len
								 )
{
	if ( !devh )
		return -1;

	int r;
	uint16_t index = ( block << 8 );

	r = libusb_control_transfer( devh
							   , CTRL_IN
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );
	return r;
}

int RtlSdrList::rtlsdr_write_array( uint8_t block
								  , uint16_t addr
								  , uint8_t *data
								  , uint8_t len
								  )
{
	if ( !devh )
		return -1;

	int r;
	uint16_t index = ( block << 8 ) | 0x10;

	r = libusb_control_transfer( devh
							   , CTRL_OUT
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );
	return r;
}

int RtlSdrList::rtlsdr_i2c_write_reg( uint8_t i2c_addr
									, uint8_t reg
									, uint8_t val
									)
{
	if ( !devh )
		return -1;

	uint16_t addr = i2c_addr;
	uint8_t data[2];

	data[ 0 ] = reg;
	data[ 1 ] = val;
	return rtlsdr_write_array( IICB, addr, (uint8_t *) &data, 2 );
}

uint8_t RtlSdrList::rtlsdr_i2c_read_reg( uint8_t i2c_addr
									   , uint8_t reg
									   )
{
	if ( !devh )
		return (uint8_t) -1;

	uint16_t addr = i2c_addr;
	uint8_t data = 0;

	rtlsdr_write_array( IICB, addr, &reg, 1 );
	rtlsdr_read_array( IICB, addr, &data, 1 );

	return data;
}


void RtlSdrList::rtlsdr_set_gpio_bit( uint8_t gpio, int val )
{
	uint16_t r;

	gpio = 1 << gpio;
	r = rtlsdr_read_reg( SYSB, GPO, 1 );
	r = val ?  (r | gpio ) : ( r & ~gpio );
	rtlsdr_write_reg( SYSB, GPO, r, 1 );
}

void RtlSdrList::rtlsdr_set_gpio_output( uint8_t gpio )
{
	int r;
	gpio = 1 << gpio;

	r = rtlsdr_read_reg( SYSB, GPD, 1 );
	rtlsdr_write_reg( SYSB, GPO, r & ~gpio, 1 );
	r = rtlsdr_read_reg( SYSB, GPOE, 1 );
	rtlsdr_write_reg( SYSB, GPOE, r | gpio, 1 );
}


int RtlSdrList::GetDongleIndexFromDongle( const Dongle dongle )
{
	for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
	{
		if ( Dongles[ i ] == dongle )
			return i;
	}
	return -1;
}


void RtlSdrList::mergeToMaster( Dongle& tempd )
{
	for ( DWORD mast = 0; mast < RtlSdrArea->activeEntries; mast++ )
	{
		Dongle* md = &Dongles[ mast ];
		if ( tempd == *md )
		{
			md->busy = tempd.busy;
			md->found = tempd.found;
			if ( !tempd.duplicated )
				memcpy( md->usbpath, tempd.usbpath, MAX_USB_PATH );
			else
			{
				if ( memcmp( md->usbpath, tempd.usbpath, MAX_USB_PATH ) != 0 )
				{
					continue;
				}
			}
			return;
		}
	}

	Dongles[ RtlSdrArea->activeEntries++ ] = tempd;
}


#define EEPROM_READ_SIZE 1		//	Maximum safe read it appears.

int RtlSdrList::rtlsdr_read_eeprom_raw( eepromdata& data )
{
	int r = 0;
	int i;
	int len;
	uint8_t offset = 0;

	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( -1 );

	r = rtlsdr_write_array(  IICB, EEPROM_ADDR, &offset, 1 );
	if ( r < 0 )
		return -3;

	len = sizeof( eepromdata );;
	for ( i = 0; i < len; i++ )
	{
		r = rtlsdr_read_array( IICB, EEPROM_ADDR, data + i, 1 );
		if ( r < 0 )
			return -3;
	}

	return r;
}


int RtlSdrList::GetEepromStrings( uint8_t *data
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

int RtlSdrList::GetEepromString( const uint8_t *data
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


bool RtlSdrList::CompareSparseRegAndData( Dongle*		dng
										, eepromdata&	data
										)
{
	eepromdata dngd;
	int err = srtlsdr_eep_img_from_Dongle( dngd, dng );
	if ( err < 0 )
		return 0;

	int r = 0;
	int i;
	uint8_t pos = 0;

	for( i = 0; i < 6; i++ )
	{
		if ( data[ i ] != dngd[ i ])
			return false;
	}

	pos = STR_OFFSET;
	//	Match manf string's size and type
	if ( memcmp( &data[ pos ], &dngd[ pos ], data[ pos ] ) != 0 )
		return false;
	pos += data[ pos ];

	//	Match last char of manf string and prod string size and type
	if ( memcmp( &data[ pos ], &dngd[ pos ], data[ pos ] ) != 0 )
		return false;
	pos += data[ pos ];

	//	Match last char of prod string and sern string size and type
	if ( memcmp( &data[ pos ], &dngd[ pos ], data[ pos ] ) != 0 )
		return false;

	return true;
}


int	RtlSdrList::srtlsdr_eep_img_from_Dongle( eepromdata&	dat
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


bool RtlSdrList::IsBusy( DWORD index )
{
	if ( index < RtlSdrArea->activeEntries )
		return Dongles[ index ].busy;
	else
		return true;
}


bool RtlSdrList::IsFound( DWORD index )
{
	if ( index < RtlSdrArea->activeEntries )
		return Dongles[ index ].found >= 0;
	else
		return false;
}


void RtlSdrList::RemoveDongle( int index )
{
	//	Dongles array - move entry + 1 to entry, until entry is empty.
	int workindex = index;
	Dongle* dongle = &Dongles[ workindex ];
	while (( dongle->vid != 0 )
		&& ( dongle->pid != 0 )
		&& ( workindex < ( MAX_DONGLES - 1 )))
	{
		workindex++;
		*dongle = Dongles[ workindex ];
		dongle = &Dongles[ workindex ];
	}
	if ( workindex <= ( MAX_DONGLES -1 ))
		Dongles[ workindex ].Clear();			// In case we hit the end
	RtlSdrArea->activeEntries--;
	//	Rewrite the registry
	WriteRegistry();
}


int RtlSdrList::FindInMasterDB( Dongle* dng, bool exact )
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


int RtlSdrList::FindGuessInMasterDB( Dongle* dng )
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

