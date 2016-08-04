#include "StdAfx.h"
#include "LoadDongle.h"
#include <WinError.h>
//	define safe functions
uint32_t	safe00( void )									{ return 0; }
const char *safe01( uint32_t )								{ return NULL; }
int			safe02( uint32_t, char*, char*, char* )			{ return -1; }
int			safe03( const char *)							{ return -1; }
int			safe04( rtlsdr_dev_t **, uint32_t )				{ return -1; }
int			safe05( rtlsdr_dev_t * )						{ return -1; }
int			safe06( rtlsdr_dev_t *, uint32_t, uint32_t )	{ return -1; }
int			safe07( rtlsdr_dev_t *, uint32_t*, uint32_t* )	{ return -1; }
int			safe08( rtlsdr_dev_t *, char *, char *, char * ){ return -1; }
int			safe09( rtlsdr_dev_t *, uint8_t *, uint8_t, uint16_t )
															{ return -1; }
#define		safe10		safe09
int			safe11( rtlsdr_dev_t *, uint32_t )				{ return -1; }
uint32_t	safe12( rtlsdr_dev_t * )						{ return -1; }
int			safe13( rtlsdr_dev_t *, int )					{ return -1; }
#define		safe14		safe05
enum rtlsdr_tuner
			safe15( rtlsdr_dev_t * )						{ return RTLSDR_TUNER_UNKNOWN; }
int			safe16( rtlsdr_dev_t *, int * )					{ return -1; }
#define		safe17		safe13
#define		safe18		safe05
int			safe19( rtlsdr_dev_t *, int, int )				{ return -1; }
#define		safe20		safe13
#define		safe21		safe11
#define		safe22		safe12
#define		safe23		safe12
#define		safe24		safe13
#define		safe25		safe13
#define		safe26		safe13
#define		safe27		safe05
#define		safe28		safe13
#define		safe29		safe05
#define		safe30		safe13
#define		safe31		safe05
int			safe32( rtlsdr_dev_t *, void *, int, int * )
															{ return -1; }
int			safe33( rtlsdr_dev_t *, rtlsdr_read_async_cb_t, void * )
															{ return -1; }
int			safe34( rtlsdr_dev_t *, rtlsdr_read_async_cb_t, void *, uint32_t, uint32_t )
															{ return -1; }
int			safe35( rtlsdr_dev_t *dev)						{ return -1; }
unsigned __int64
			safe36( void )									{ return 0; }
int			safe37(	rtlsdr_dev_t *, uint8_t, int32_t *, char * )
															{ return -1; }
#define		safe38		safe05
int			safe39(	rtlsdr_dev_t *, uint8_t )				{ return -1; }
int			safe40(	rtlsdr_dev_t *, uint8_t, int32_t )		{ return -1; }

IRtlSdr*		LoadDongle::surr = NULL;
static IRtlSdr* GetSurrogate	( void ) { return LoadDongle::surr; }
static void		DeleteSurrogate	( IRtlSdr* ) {}

LoadDongle::LoadDongle(void)
	: m_hModule( NULL )
{
	CreateRtlSdr = NULL;
	DeleteRtlSdr = NULL;
	makeSafe();
#if defined( UNICODE )
	m_hModule = LoadLibrary( _T( "rtlsdru.dll" ));
#else
	m_hModule = LoadLibrary( "rtlsdr.dll" );
#endif
	TRACE( "LoadLibrary GetLastError = %d\n", GetLastError());
	if ( m_hModule )
	{
		rtlsdr_get_device_count       = (uint32_t (*) ( void ))
										GetProcAddress( m_hModule, "rtlsdr_get_device_count" );
		rtlsdr_get_device_name        = ( const char* (*) ( uint32_t ))
										GetProcAddress( m_hModule, "rtlsdr_get_device_name" );
		rtlsdr_get_index_by_serial    = ( int (*) ( const char *serial ))
										GetProcAddress( m_hModule, "rtlsdr_get_index_by_serial" );
		rtlsdr_open                   = ( int (*) ( rtlsdr_dev_t **, uint32_t ))
										GetProcAddress( m_hModule, "rtlsdr_open" );
		rtlsdr_close                  = ( int (*) ( rtlsdr_dev_t *dev ))
										GetProcAddress( m_hModule, "rtlsdr_close" );
		rtlsdr_set_xtal_freq          = ( int (*) ( rtlsdr_dev_t *, uint32_t, uint32_t ))
										GetProcAddress( m_hModule, "rtlsdr_set_xtal_freq" );
		rtlsdr_get_xtal_freq          = ( int (*) ( rtlsdr_dev_t *, uint32_t *, uint32_t * ))
										GetProcAddress( m_hModule, "rtlsdr_get_xtal_freq" );
		rtlsdr_get_usb_strings        = ( int (*) ( rtlsdr_dev_t *, char *, char *, char *))
										GetProcAddress( m_hModule, "rtlsdr_get_usb_strings" );
		rtlsdr_get_usb_cstrings       = ( int (*) ( rtlsdr_dev_t *, CString&, CString&, CString& ))
										GetProcAddress( m_hModule, "rtlsdr_get_usb_cstrings" );
		rtlsdr_write_eeprom           = ( int (*) ( rtlsdr_dev_t *, uint8_t *, uint8_t, uint16_t len ))
										GetProcAddress( m_hModule, "rtlsdr_write_eeprom" );
		rtlsdr_read_eeprom            = ( int (*) ( rtlsdr_dev_t *, uint8_t *, uint8_t, uint16_t len ))
										GetProcAddress( m_hModule, "rtlsdr_read_eeprom" );
		rtlsdr_set_center_freq        = ( int (*) ( rtlsdr_dev_t *, uint32_t ))
										GetProcAddress( m_hModule, "rtlsdr_set_center_freq" );
		rtlsdr_get_center_freq        = ( uint32_t (*) ( rtlsdr_dev_t * ))
										GetProcAddress( m_hModule, "rtlsdr_get_center_freq" );
		rtlsdr_set_freq_correction    = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_freq_correction" );
		rtlsdr_get_freq_correction    = ( int (*) ( rtlsdr_dev_t *dev ))
										GetProcAddress( m_hModule, "rtlsdr_get_freq_correction" );
		rtlsdr_get_tuner_type         = ( enum rtlsdr_tuner (*) ( rtlsdr_dev_t * ))
										GetProcAddress( m_hModule, "rtlsdr_get_tuner_type" );
		rtlsdr_get_tuner_gains        = ( int (*) ( rtlsdr_dev_t *, int * ))
										GetProcAddress( m_hModule, "rtlsdr_get_tuner_gains" );
		rtlsdr_set_tuner_gain         = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_tuner_gain" );
		rtlsdr_get_tuner_gain         = ( int (*) ( rtlsdr_dev_t *dev ))
										GetProcAddress( m_hModule, "rtlsdr_get_tuner_gain" );
		rtlsdr_set_tuner_if_gain      = ( int (*) ( rtlsdr_dev_t *, int, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_tuner_if_gain" );
		rtlsdr_set_tuner_gain_mode    = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_tuner_gain_mode" );
		rtlsdr_set_sample_rate        = ( int (*) ( rtlsdr_dev_t *, uint32_t ))
										GetProcAddress( m_hModule, "rtlsdr_set_sample_rate" );
		rtlsdr_get_sample_rate        = ( uint32_t (*) ( rtlsdr_dev_t * ))
										GetProcAddress( m_hModule, "rtlsdr_get_sample_rate" );
		rtlsdr_get_corr_sample_rate	  = ( uint32_t (*) ( rtlsdr_dev_t *dev ))
										GetProcAddress( m_hModule, "rtlsdr_get_corr_sample_rate" );
		rtlsdr_set_testmode           = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_testmode" );
		rtlsdr_set_agc_mode           = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_agc_mode" );
		rtlsdr_set_direct_sampling    = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_direct_sampling" );
		rtlsdr_get_direct_sampling    = ( int (*) ( rtlsdr_dev_t * ))
										GetProcAddress( m_hModule, "rtlsdr_get_direct_sampling" );
		rtlsdr_set_offset_tuning      = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_offset_tuning" );
		rtlsdr_get_offset_tuning      = ( int (*) ( rtlsdr_dev_t * ))
										GetProcAddress( m_hModule, "rtlsdr_get_offset_tuning" );
		rtlsdr_set_dithering          = ( int (*) ( rtlsdr_dev_t *, int ))
										GetProcAddress( m_hModule, "rtlsdr_set_dithering" );
		rtlsdr_reset_buffer           = ( int (*) ( rtlsdr_dev_t *dev ))
										GetProcAddress( m_hModule, "rtlsdr_reset_buffer" );
		rtlsdr_read_sync              = ( int (*) ( rtlsdr_dev_t *, void *, int, int * ))
										GetProcAddress( m_hModule, "rtlsdr_read_sync" );
		rtlsdr_wait_async             = ( int (*) ( rtlsdr_dev_t *
												  , rtlsdr_read_async_cb_t
												  , void *
												  ))
										GetProcAddress( m_hModule, "rtlsdr_wait_async" );
		rtlsdr_read_async	          = ( int (*) ( rtlsdr_dev_t *
												  , rtlsdr_read_async_cb_t
												  , void *
												  , uint32_t
												  , uint32_t
												  ))
										GetProcAddress( m_hModule, "rtlsdr_read_async" );
		rtlsdr_cancel_async           = ( int (*) ( rtlsdr_dev_t * ))
										GetProcAddress( m_hModule, "rtlsdr_cancel_async" );
		rtlsdr_get_version_int64	  = ( unsigned __int64 (*) ( void ))
										GetProcAddress( m_hModule, "rtlsdr_get_version_int64" );

//  ADDED_STAGE_GAIN_MATERIAL
		rtlsdr_get_tuner_stage_gains  = ( int (*) ( rtlsdr_dev_t *dev
															   , uint8_t stage
															   , int32_t *gains
															   , char *description
															   ))
										GetProcAddress( m_hModule, "rtlsdr_get_tuner_stage_gains" );
		rtlsdr_get_tuner_stage_count  = ( int (*) ( rtlsdr_dev_t *dev ))
										GetProcAddress( m_hModule, "rtlsdr_get_tuner_stage_count" );
		rtlsdr_get_tuner_stage_gain	  = ( int (*) ( rtlsdr_dev_t *dev, uint8_t stage))
										GetProcAddress( m_hModule, "rtlsdr_get_tuner_stage_gain" );
		rtlsdr_set_tuner_stage_gain	  = ( int (*) ( rtlsdr_dev_t *dev, uint8_t stage, int32_t gain ))
										GetProcAddress( m_hModule, "rtlsdr_set_tuner_stage_gain" );
//  /ADDED_STAGE_GAIN_MATERIAL

//	Added static functions....
		rtlsdr_get_device_count	      = ( uint32_t (*) ( void ))
										GetProcAddress( m_hModule, "rtlsdr_get_device_count" );
		rtlsdr_get_device_name		  = ( const char* (*) ( uint32_t ))
										GetProcAddress( m_hModule, "rtlsdr_get_device_name" );
		rtlsdr_get_device_usb_strings = ( int (*) ( uint32_t, char *, char *, char *))
										GetProcAddress( m_hModule, "rtlsdr_get_device_usb_strings" );
		rtlsdr_get_device_usb_cstrings = ( int (*) ( uint32_t, CString&, CString&, CString&))
										GetProcAddress( m_hModule, "rtlsdr_get_device_usb_cstrings" );
		rtlsdr_get_index_by_serial	  = ( int (*) ( const char* ))
										GetProcAddress( m_hModule, "rtlsdr_get_index_by_serial" );
//	/Added static functions....



		merror = 0;

		if ( rtlsdr_get_device_count       == NULL )	merror |= 1I64 <<  0;
		if ( rtlsdr_get_device_name        == NULL )	merror |= 1I64 <<  1;
		if ( rtlsdr_get_device_usb_strings == NULL )	merror |= 1I64 <<  2;
		if ( rtlsdr_get_index_by_serial    == NULL )	merror |= 1I64 <<  3;
		if ( rtlsdr_open                   == NULL )	merror |= 1I64 <<  4;
		if ( rtlsdr_close                  == NULL )	merror |= 1I64 <<  5;
		if ( rtlsdr_set_xtal_freq          == NULL )	merror |= 1I64 <<  6;
		if ( rtlsdr_get_xtal_freq          == NULL )	merror |= 1I64 <<  7;
		if ( rtlsdr_get_usb_strings        == NULL )	merror |= 1I64 <<  8;
		if ( rtlsdr_write_eeprom           == NULL )	merror |= 1I64 <<  9;
		if ( rtlsdr_read_eeprom            == NULL )	merror |= 1I64 << 10;
		if ( rtlsdr_set_center_freq        == NULL )	merror |= 1I64 << 11;
		if ( rtlsdr_get_center_freq        == NULL )	merror |= 1I64 << 12;
		if ( rtlsdr_set_freq_correction    == NULL )	merror |= 1I64 << 13;
		if ( rtlsdr_get_freq_correction    == NULL )	merror |= 1I64 << 14;
		if ( rtlsdr_get_tuner_type         == NULL )	merror |= 1I64 << 15;
		if ( rtlsdr_get_tuner_gains        == NULL )	merror |= 1I64 << 16;
		if ( rtlsdr_set_tuner_gain         == NULL )	merror |= 1I64 << 17;
		if ( rtlsdr_get_tuner_gain         == NULL )	merror |= 1I64 << 18;
		if ( rtlsdr_set_tuner_if_gain      == NULL )	merror |= 1I64 << 19;
		if ( rtlsdr_set_tuner_gain_mode    == NULL )	merror |= 1I64 << 20;
		if ( rtlsdr_set_sample_rate        == NULL )	merror |= 1I64 << 21;
		if ( rtlsdr_get_sample_rate        == NULL )	merror |= 1I64 << 22;
		if ( rtlsdr_get_corr_sample_rate   == NULL )	merror |= 1I64 << 23;
		if ( rtlsdr_set_testmode           == NULL )	merror |= 1I64 << 24;
		if ( rtlsdr_set_agc_mode           == NULL )	merror |= 1I64 << 25;
		if ( rtlsdr_set_direct_sampling    == NULL )	merror |= 1I64 << 26;
		if ( rtlsdr_get_direct_sampling    == NULL )	merror |= 1I64 << 27;
		if ( rtlsdr_set_offset_tuning      == NULL )	merror |= 1I64 << 28;
		if ( rtlsdr_get_offset_tuning      == NULL )	merror |= 1I64 << 29;
		if ( rtlsdr_set_dithering          == NULL ) {	rtlsdr_set_dithering = safe30;
														merror |= 1I64 << 30;
													 }
		if ( rtlsdr_reset_buffer           == NULL )	merror |= 1I64 << 31;
		if ( rtlsdr_read_sync              == NULL )	merror |= 1I64 << 32;
		if ( rtlsdr_wait_async             == NULL )	merror |= 1I64 << 33;
		if ( rtlsdr_read_async	           == NULL )	merror |= 1I64 << 34;
		if ( rtlsdr_cancel_async           == NULL )	merror |= 1I64 << 35;
		if ( rtlsdr_get_version_int64	   == NULL ) {	merror |= 1I64 << 36;
														rtlsdr_get_version_int64 = safe36;
													 }
		if ( rtlsdr_get_tuner_stage_gains  == NULL ) {	merror |= 1I64 << 37;
														rtlsdr_get_tuner_stage_gains = safe37;
													 }
		if ( rtlsdr_get_tuner_stage_count  == NULL ) {	merror |= 1I64 << 38;
														rtlsdr_get_tuner_stage_count = safe38;
													 }
		if ( rtlsdr_get_tuner_stage_gain   == NULL ) {	merror |= 1I64 << 39;
														rtlsdr_get_tuner_stage_gain = safe39;
													 }
		if ( rtlsdr_set_tuner_stage_gain   == NULL ) {	merror |= 1I64 << 40;
														rtlsdr_set_tuner_stage_gain = safe40;
													 }

		uint64_t test = (uint64_t) -1;
		test ^= ( 1I64 << 30 ) | 1I64 << 23 | ( 0x1fI64 << 36 );
		if (( merror & test ) != 0 )
		{
			//	Some desired libraries are missing.
			TRACE( "LOADING ERROR 0x%I64x\n", merror );
			makeSafe();
			FreeLibrary( m_hModule );
			m_hModule = NULL;
			OutputDebugString( _T( "Some entry points failed to load\n" ));
			return;
		}

		CreateRtlSdr = (IRtlSdr* (*)(void))
			GetProcAddress( m_hModule, "CreateRtlSdr" );
		int err1 = GetLastError();
		DeleteRtlSdr = (void (*) (IRtlSdr*))
			GetProcAddress( m_hModule, "DeleteRtlSdr" );
		int err2 = GetLastError();
		err2 = err2;
		if ( err1 != 0 )
		{
			OutputDebugString( _T( "CreateRtlSdr entry point missing!\n" ));
			surr = new rtlsdr_surrogate( this );
			CreateRtlSdr = GetSurrogate;
			DeleteRtlSdr = DeleteSurrogate;
		}
		else
		if ( err2 != 0 )
		{
			OutputDebugString( _T( "Delete RtlSdr entry point missing!\n" ));
			surr = new rtlsdr_surrogate( this );
			CreateRtlSdr = GetSurrogate;
			DeleteRtlSdr = DeleteSurrogate;
		}
		else
			OutputDebugString( _T( "RtlTool::LoadLibrary fully successful\n" ));
	}
	else
	{
		OutputDebugString( _T( "RtlTool::LoadLibrary failed to get hModule\n" ));
		AfxMessageBox( _T( "rtlsdr.dll could not be loaded. Check if it, libusb-1.0.dll, and pthreadVCE2.dll are all present." ));
		merror = (uint64_t) -1;
	}
}

LoadDongle::~LoadDongle(void)
{
	if ( m_hModule )
		FreeLibrary( m_hModule );
	if ( surr )
		delete ( rtlsdr_surrogate* ) surr;
	makeSafe();
}

void LoadDongle::makeSafe( void )
{
	rtlsdr_get_device_count        = safe00;
	rtlsdr_get_device_name         = safe01;
	rtlsdr_get_device_usb_strings  = safe02;
	rtlsdr_get_device_usb_cstrings = NULL;
	rtlsdr_get_index_by_serial     = safe03;
	rtlsdr_open                    = safe04;
	rtlsdr_close                   = safe05;
	rtlsdr_set_xtal_freq           = safe06;
	rtlsdr_get_xtal_freq           = safe07;
	rtlsdr_get_usb_strings         = safe08;
	rtlsdr_get_usb_cstrings		   = NULL;
	rtlsdr_write_eeprom            = safe09;
	rtlsdr_read_eeprom             = safe10;
	rtlsdr_set_center_freq         = safe11;
	rtlsdr_get_center_freq         = safe12;
	rtlsdr_set_freq_correction     = safe13;
	rtlsdr_get_freq_correction     = safe14;
	rtlsdr_get_tuner_type          = safe15;
	rtlsdr_get_tuner_gains         = safe16; 
	rtlsdr_set_tuner_gain          = safe17;
	rtlsdr_get_tuner_gain          = safe18;
	rtlsdr_set_tuner_if_gain       = safe19;
	rtlsdr_set_tuner_gain_mode     = safe20;
	rtlsdr_set_sample_rate         = safe21;
	rtlsdr_get_sample_rate         = safe22;
	rtlsdr_get_corr_sample_rate    = safe23;
	rtlsdr_set_testmode            = safe24;
	rtlsdr_set_agc_mode            = safe25;
	rtlsdr_set_direct_sampling     = safe26;
	rtlsdr_get_direct_sampling     = safe27;
	rtlsdr_set_offset_tuning       = safe28;
	rtlsdr_get_offset_tuning       = safe29;
	rtlsdr_set_dithering           = safe30;
	rtlsdr_reset_buffer            = safe31;
	rtlsdr_read_sync               = safe32;
	rtlsdr_wait_async              = safe33;
	rtlsdr_read_async	           = safe34;
	rtlsdr_cancel_async            = safe35;
	rtlsdr_get_version_int64	   = safe36;
	rtlsdr_get_tuner_stage_gains   = safe37;
	rtlsdr_get_tuner_stage_count   = safe38;
	rtlsdr_get_tuner_stage_gain    = safe39;
}



//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

rtlsdr_surrogate::rtlsdr_surrogate( LoadDongle* in_LD )
	: dev( NULL )
	, LD( in_LD )
{
}

rtlsdr_surrogate::~rtlsdr_surrogate()
{
	if ( dev != NULL )
		rtlsdr_close();
}

#define HERE __noop

// OLD STYLE EXPORTED RTLSDR FUNCTIONS


int rtlsdr_surrogate::rtlsdr_open( uint32_t index )
{
	return LD->rtlsdr_open( &dev, index );
}

int rtlsdr_surrogate::rtlsdr_close()
{
	int rc = LD->rtlsdr_close( dev );
	dev = NULL;
	return rc;
}

uint32_t rtlsdr_surrogate::rtlsdr_get_tuner_clock( void )
{
	return 0;
/* perhaps
	uint32_t tuner_freq;
	return LD->rtlsdr_get_xtal_freq( dev, NULL, tuner_freq );
	return tuner_freq;
*/
}
int rtlsdr_surrogate::rtlsdr_set_xtal_freq( uint32_t rtl_freq
										  , uint32_t tuner_freq
										  )
{
	return LD->rtlsdr_set_xtal_freq( dev, rtl_freq, tuner_freq );
}

int rtlsdr_surrogate::rtlsdr_get_xtal_freq( uint32_t *rtl_freq
										  , uint32_t *tuner_freq
										  )
{
	return LD->rtlsdr_get_xtal_freq( dev, rtl_freq, tuner_freq );
}

int rtlsdr_surrogate::rtlsdr_get_usb_strings( char *manufact
											, char *product
											, char *serial
											)
{
	return LD->rtlsdr_get_usb_strings( dev
									 , manufact
									 , product
									 , serial
									 );
}

int rtlsdr_surrogate::rtlsdr_get_usb_strings( CString& manufact
											, CString& product
											, CString& serial
											)
{
	int rc = -1;
	if ( LD->rtlsdr_get_usb_cstrings )
	{
		rc = LD->rtlsdr_get_usb_cstrings( dev
										, manufact
										, product
										, serial
										);
	}
	else
	{
		CStringA man;
		CStringA prd;
		CStringA ser;
		char* manp = man.GetBuffer( 256 );
		char* prdp = prd.GetBuffer( 256 );
		char* serp = ser.GetBuffer( 256 );
		rc = LD->rtlsdr_get_usb_strings( dev 
									   , manp
									   , prdp
									   , serp
									   );
		man.ReleaseBuffer();
		prd.ReleaseBuffer();
		ser.ReleaseBuffer();
		if ( rc >= 0 )
		{
			manufact = man;
			product	 = prd;
			serial	 = ser;
		}
	}
	return rc;
}

int rtlsdr_surrogate::rtlsdr_write_eeprom( uint8_t *data
										 , uint8_t offset
										 , uint16_t len
										 )
{
	return LD->rtlsdr_write_eeprom( dev, data, offset, len );
}


int rtlsdr_surrogate::rtlsdr_read_eeprom( uint8_t *data
										, uint8_t offset
										, uint16_t len
										)
{
	return LD->rtlsdr_read_eeprom( dev, data, offset, len );
}

int rtlsdr_surrogate::rtlsdr_read_eeprom( eepromdata& data )
{
	return LD->rtlsdr_read_eeprom( dev, data, 0, EEPROM_SIZE );
}

int rtlsdr_surrogate::rtlsdr_set_center_freq( uint32_t freq )
{
	return LD->rtlsdr_set_center_freq( dev, freq );
}

uint32_t rtlsdr_surrogate::rtlsdr_get_center_freq( void )
{
	return LD->rtlsdr_get_center_freq( dev );
}

int rtlsdr_surrogate::rtlsdr_set_freq_correction( int ppm )
{
	return LD->rtlsdr_set_freq_correction( dev, ppm );
}

int rtlsdr_surrogate::rtlsdr_get_freq_correction( void )
{
	return LD->rtlsdr_get_freq_correction( dev );
}

int rtlsdr_surrogate::rtlsdr_get_tuner_type( void )
{
	return (int) LD->rtlsdr_get_tuner_type( dev );
}

int rtlsdr_surrogate::rtlsdr_get_tuner_gains( int *gains )
{
	return LD->rtlsdr_get_tuner_gains( dev, gains );
}

int rtlsdr_surrogate::rtlsdr_set_tuner_gain( int gain)
{
	return LD->rtlsdr_set_tuner_gain( dev, gain );
}

int rtlsdr_surrogate::rtlsdr_get_tuner_gain( void )
{
	return LD->rtlsdr_get_tuner_gain( dev );
}

int rtlsdr_surrogate::rtlsdr_set_tuner_if_gain( int stage, int gain)
{
	return LD->rtlsdr_set_tuner_if_gain( dev, stage, gain );
}

int rtlsdr_surrogate::rtlsdr_set_tuner_gain_mode( int manual)
{
	return LD->rtlsdr_set_tuner_gain_mode( dev, manual );
}


int rtlsdr_surrogate::rtlsdr_set_sample_rate( uint32_t rate)
{
	return LD->rtlsdr_set_sample_rate( dev, rate );
}

uint32_t rtlsdr_surrogate::rtlsdr_get_sample_rate( void )
{
	return LD->rtlsdr_get_sample_rate( dev );
}

uint32_t rtlsdr_surrogate::rtlsdr_get_corr_sample_rate( void )
{
	return LD->rtlsdr_get_corr_sample_rate( dev );
}

int rtlsdr_surrogate::rtlsdr_set_testmode( int on)
{
	return LD->rtlsdr_set_testmode( dev, on );
}

int rtlsdr_surrogate::rtlsdr_set_agc_mode( int on)
{
	return LD->rtlsdr_set_agc_mode( dev, on );
}

int rtlsdr_surrogate::rtlsdr_set_direct_sampling( int on)
{
	return LD->rtlsdr_set_direct_sampling( dev, on );
}

int rtlsdr_surrogate::rtlsdr_get_direct_sampling( void )
{
	return LD->rtlsdr_get_direct_sampling( dev );
}

int rtlsdr_surrogate::rtlsdr_set_offset_tuning( int on )
{
	return LD->rtlsdr_set_offset_tuning( dev, on );
}

int rtlsdr_surrogate::rtlsdr_get_offset_tuning( void )
{
	return LD->rtlsdr_get_offset_tuning( dev );
}

int rtlsdr_surrogate::rtlsdr_set_dithering( int dither )
{
	return LD->rtlsdr_set_dithering( dev, dither );
}

int rtlsdr_surrogate::rtlsdr_reset_buffer( void )
{
	return LD->rtlsdr_reset_buffer( dev );
}

int rtlsdr_surrogate::rtlsdr_read_sync( BYTE *buf, int len, int *n_read)
{
	return LD->rtlsdr_read_sync( dev, buf, len, n_read );
}

int rtlsdr_surrogate::rtlsdr_wait_async( rtlsdr_read_async_cb_t cb, void *ctx)
{
	return LD->rtlsdr_wait_async( dev, cb, ctx );
}

int rtlsdr_surrogate::rtlsdr_read_async( rtlsdr_read_async_cb_t cb
									   , void *ctx
									   , uint32_t buf_num
									   , uint32_t buf_len
									   )
{
	return LD->rtlsdr_read_async( dev, cb, ctx, buf_num, buf_len );
}

int rtlsdr_surrogate::rtlsdr_cancel_async( void )
{
	return LD->rtlsdr_cancel_async( dev );
}

const char* rtlsdr_surrogate::rtlsdr_get_version( void )
{
	return "0";
}

unsigned __int64 rtlsdr_surrogate::rtlsdr_get_version_int64( void )
{
	return 0;
}

int rtlsdr_surrogate::rtlsdr_get_tuner_stage_gains( uint8_t stage
												  , int32_t *gains
												  , char *description
												  )
{
	return LD->rtlsdr_get_tuner_stage_gains( dev, stage, gains, description );
}

int rtlsdr_surrogate::rtlsdr_get_tuner_stage_count( void )
{
	return LD->rtlsdr_get_tuner_stage_count( dev );
}

int rtlsdr_surrogate::rtlsdr_set_tuner_stage_gain( uint8_t stage
												 , int32_t gain
												 )
{
	return LD->rtlsdr_set_tuner_stage_gain( dev, stage, gain );
}

int rtlsdr_surrogate::rtlsdr_get_tuner_stage_gain( uint8_t stage )
{
	return LD->rtlsdr_get_tuner_stage_gain( dev, stage );
}

uint32_t rtlsdr_surrogate::rtlsdr_get_device_count( void )
{
	return LD->rtlsdr_get_device_count();
}

const char* rtlsdr_surrogate::rtlsdr_get_device_name( uint32_t index )
{
	return LD->rtlsdr_get_device_name( index );
}

int rtlsdr_surrogate::rtlsdr_get_device_usb_strings( uint32_t index
												   , char *manufact
												   , char *product
												   , char *serial
												   )
{
	return LD->rtlsdr_get_device_usb_strings( index, manufact, product, serial );
}

int rtlsdr_surrogate::rtlsdr_get_device_usb_strings( uint32_t index
												   , CString& manufact
												   , CString& product
												   , CString& serial
												   )
{
	if ( LD->rtlsdr_get_device_usb_cstrings )
		return LD->rtlsdr_get_device_usb_cstrings( index, manufact, product, serial );
	else
	{
		int rc;
		CStringA svid;
		CStringA spid;
		CStringA sser;
		char* vid = svid.GetBuffer( 256 );
		char* pid = spid.GetBuffer( 256 );
		char* ser = sser.GetBuffer( 256 );
		rc = LD->rtlsdr_get_device_usb_strings( index, vid, pid, ser );
		svid.ReleaseBuffer();
		spid.ReleaseBuffer();
		sser.ReleaseBuffer();
		if ( rc >= 0 )
		{
			manufact = svid;
			product = spid;
			serial = sser;
		}
		return rc;
	}
}

int rtlsdr_surrogate::rtlsdr_get_index_by_serial( const char *serial )
{
	return rtlsdr_get_index_by_serial( serial );
}

