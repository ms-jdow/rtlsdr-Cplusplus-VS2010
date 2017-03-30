// rtlsdr_app.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "librtlsdr.h"

#include "rtlsdr_app.h"

#define HERE()	 __noop
//#define HERE() { CStringA msg; msg.Format( "API: %s\n", __FUNCTION__ ); OutputDebugStringA( msg ); }

static CList< rtlsdr* > appdongles;


//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CRTLDirectApp
#if defined THEAPP
#include "rtlsdr_app.h"
#define X theApp.

CStringA	RtlSdrVersionString;

BEGIN_MESSAGE_MAP(rtlsdr_app, CWinApp)
END_MESSAGE_MAP()


// CRTLDirectApp construction

rtlsdr_app::rtlsdr_app()
{
	// Place all significant initialization in InitInstance
	HERE();
	appdongles.RemoveAll();
}

rtlsdr_app::~rtlsdr_app()
{
	HERE();
}

// The one and only CRTLDirectApp object

rtlsdr_app theApp;

#pragma comment( lib, "version.lib" )

// CRTLDirectApp initialization
BOOL rtlsdr_app::InitInstance()
{
	HERE();
	CWinApp::InitInstance();

	HERE();
 	return true;
}

BOOL rtlsdr_app::ExitInstance()
{
	HERE();
	//	If something was left open try to hammer it into submission.
	while( appdongles.GetHeadPosition() != NULL )
	{
		rtlsdr* theDongle = appdongles.RemoveHead();
		theDongle->rtlsdr_close();
		delete theDongle;
	}

	CWinApp::ExitInstance();
	HERE();
	return true;
}

rtlsdr* rtlsdr_app::GetSdrDongle( void )	//	Used if needed.
{
	HERE();
	rtlsdr* theDongle = new rtlsdr;
	appdongles.AddTail( theDongle );
	HERE();
	return theDongle;
}

rtlsdr* rtlsdr_app::FindDongleByDeviceIndex	( int index )
{
	HERE();
	POSITION pos = appdongles.GetHeadPosition();
	while( pos )
	{
		rtlsdr* dongle = appdongles.GetNext( pos );
		if ( dongle->GetOpenedDeviceIndex() == index )
		{
			HERE();
			return dongle;
		}
	}
	HERE();
	return NULL;
}

void rtlsdr_app::CloseSdrDongle( rtlsdr* dongle )
{
	HERE();
	POSITION pos = appdongles.GetHeadPosition();
	while( pos )
	{
		if ( appdongles.GetAt( pos ) == dongle )
		{
			delete dongle;
			appdongles.RemoveAt( pos );
	HERE();
			return;
		}
		appdongles.GetNext( pos );
	}
	ASSERT( false );
	TRACE( "Fell through!\n" );
	HERE();
	delete dongle;	// Somehow we fell through. Try to do the right thing.
}

#else

#define X	// theApp.

static CStringA RtlSdrVersionString;
#pragma comment( lib, "version.lib" )

static rtlsdr* GetSdrDongle( void )	//	Used if needed.
{
	HERE();
	rtlsdr* theDongle = new rtlsdr;
	appdongles.AddTail( theDongle );
	HERE();
	return theDongle;
}

static void CloseSdrDongle( rtlsdr* dongle )
{
	HERE();
	POSITION pos = appdongles.GetHeadPosition();
	while( pos )
	{
		if ( appdongles.GetAt( pos ) == dongle )
		{
			delete dongle;
			appdongles.RemoveAt( pos );
	HERE();
			return;
		}
		appdongles.GetNext( pos );
	}
	ASSERT( false );
	TRACE( "Fell through!\n" );
	HERE();
	delete dongle;	// Somehow we fell through. Try to do the right thing.
}

#endif


// OLD STYLE EXPORTED RTLSDR FUNCTIONS
RTLSDR_API uint32_t rtlsdr_get_device_count( void )
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_device_count();
}


RTLSDR_API const char* rtlsdr_get_device_name( uint32_t index )
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_device_name( index );
}


RTLSDR_API int rtlsdr_get_device_usb_strings( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											)
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_device_usb_strings( index
											 , manufact
											 , product
											 , serial
											 );
}


RTLSDR_API int rtlsdr_get_device_usb_cstrings( uint32_t index
											 , CString& manufact
											 , CString& product
											 , CString& serial
											 )
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_device_usb_strings( index
											 , manufact
											 , product
											 , serial
											 );
}


RTLSDR_API int rtlsdr_get_index_by_serial( const char *serial )
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_index_by_serial( serial );
}


RTLSDR_API int rtlsdr_open( rtlsdr_dev_t **dev, uint32_t index )
{
	HERE();
	int ret = -1;
	rtlsdr* dongle;
	if (( dongle = X GetSdrDongle()) != NULL )
	{
		ret = dongle->rtlsdr_open( index );
		if ( ret == 0 )
		{
			*dev = (rtlsdr_dev_t *) dongle;
		}
		else
		{
			X CloseSdrDongle( dongle );
			dongle = NULL;
		}
	}

	HERE();
	return ret;
}


RTLSDR_API int rtlsdr_close( rtlsdr_dev_t *dev )
{
	HERE();
	int ret = 1;	// Benign error return
	if ( dev != NULL )
	{
		ret = ((rtlsdr*) dev )->rtlsdr_close();
		X CloseSdrDongle((rtlsdr*) dev );
	}
//	delete (rtlsdr*) dev;
	HERE();
	return ret;
}


RTLSDR_API int rtlsdr_set_xtal_freq( rtlsdr_dev_t *dev
								   , uint32_t rtl_freq
								   , uint32_t tuner_freq
								   )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )
				-> rtlsdr_set_xtal_freq( rtl_freq, tuner_freq );
	return -1;
}


RTLSDR_API int rtlsdr_get_xtal_freq( rtlsdr_dev_t *dev
								   , uint32_t *rtl_freq
								   , uint32_t *tuner_freq
								   )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )
				->rtlsdr_get_xtal_freq( rtl_freq, tuner_freq );
	return -1;
}


RTLSDR_API int rtlsdr_get_usb_strings( rtlsdr_dev_t *dev
									 , char *manufact
									 , char *product
									 , char *serial
									 )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_usb_strings( manufact
													   , product
													   , serial
													   );
	return NULL;
}


RTLSDR_API int rtlsdr_write_eeprom( rtlsdr_dev_t *dev
								  , uint8_t *data
								  , uint8_t offset
								  , uint16_t len
								  )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_write_eeprom( data, offset, len );
	return -1;
}


RTLSDR_API int rtlsdr_read_eeprom( rtlsdr_dev_t *dev
								 , uint8_t *data
								 , uint8_t offset
								 , uint16_t len
								 )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_read_eeprom( data, offset, len );
	return -1;
}


RTLSDR_API int rtlsdr_set_center_freq( rtlsdr_dev_t *dev, uint32_t freq )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_center_freq( freq );
	return -1;
}


RTLSDR_API uint32_t rtlsdr_get_center_freq( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_center_freq();
	return -1;
}


RTLSDR_API int rtlsdr_set_freq_correction( rtlsdr_dev_t *dev, int ppm )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_freq_correction( ppm );
	return -1;
}


RTLSDR_API int rtlsdr_get_freq_correction( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_freq_correction();
	return -1;
}


RTLSDR_API enum rtlsdr_tuner rtlsdr_get_tuner_type( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return (enum rtlsdr_tuner) ((rtlsdr*) dev )->rtlsdr_get_tuner_type();
	return RTLSDR_TUNER_UNKNOWN;
}


RTLSDR_API int rtlsdr_get_tuner_gains( rtlsdr_dev_t *dev, int *gains )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_gains( gains );
	return -1;
}


RTLSDR_API int rtlsdr_set_tuner_gain( rtlsdr_dev_t *dev, int gain )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_gain( gain );
	return -1;
}


RTLSDR_API int rtlsdr_get_tuner_gain( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_gain();
	return -1;
}


RTLSDR_API int rtlsdr_set_tuner_if_gain( rtlsdr_dev_t *dev
									   , int stage
									   , int gain
									   )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_if_gain( stage, gain );
	return -1;
}

#if 1	//  ADDED_STAGE_GAIN_MATERIAL

RTLSDR_API int rtlsdr_get_tuner_stage_gains( rtlsdr_dev_t *dev
										   , uint8_t stage
										   , int32_t *gains
										   , char *description
										   )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_stage_gains( stage, gains, description );
	return -1;
}


RTLSDR_API int rtlsdr_get_tuner_stage_count( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_stage_count();
	return -1;
}


RTLSDR_API int rtlsdr_get_tuner_stage_gain( rtlsdr_dev_t *dev
										  , uint8_t stage
										  )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_stage_gain( stage );
	return -1;
}


RTLSDR_API int rtlsdr_set_tuner_stage_gain( rtlsdr_dev_t *dev
										  , uint8_t stage
										  , int32_t gain
										  )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_stage_gain( stage, gain );
	return -1;
}
#endif	//  ADDED_STAGE_GAIN_MATERIAL


RTLSDR_API int rtlsdr_set_tuner_gain_mode( rtlsdr_dev_t *dev, int manual )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_gain_mode( manual );
	return -1;
}


RTLSDR_API int rtlsdr_set_sample_rate( rtlsdr_dev_t *dev, uint32_t rate )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_sample_rate( rate );
	return -1;
}


RTLSDR_API uint32_t rtlsdr_get_sample_rate( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_sample_rate();
	return -1;
}


RTLSDR_API uint32_t rtlsdr_get_corr_sample_rate( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_corr_sample_rate();
	return -1;
}


RTLSDR_API int rtlsdr_set_testmode( rtlsdr_dev_t *dev, int on )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_testmode( on );
	return -1;
}


RTLSDR_API int rtlsdr_set_agc_mode( rtlsdr_dev_t *dev, int on )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_agc_mode( on );
	return -1;
}


RTLSDR_API int rtlsdr_set_bias_tee( rtlsdr_dev_t *dev, int on )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_bias_tee( on );
	return -1;
}


RTLSDR_API int rtlsdr_set_direct_sampling( rtlsdr_dev_t *dev, int on )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_direct_sampling( on );
	return -1;
}


RTLSDR_API int rtlsdr_get_direct_sampling( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_direct_sampling();
	return -1;
}


RTLSDR_API int rtlsdr_set_offset_tuning( rtlsdr_dev_t *dev, int on )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_offset_tuning( on );
	return -1;
}


RTLSDR_API int rtlsdr_get_offset_tuning( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_offset_tuning();
	return -1;
}


RTLSDR_API int rtlsdr_set_dithering( rtlsdr_dev_t *dev, int dither )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_dithering( dither );
	return -1;
}


RTLSDR_API int rtlsdr_set_gpio_bit(rtlsdr_dev_t *dev, int bit, int enableout, int on)
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_gpio_bit( bit, enableout, on );
	return -1;
}


RTLSDR_API int rtlsdr_get_gpio_bit(rtlsdr_dev_t *dev, int bit )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_gpio_bit( bit );
	return -1;
}


RTLSDR_API int rtlsdr_reset_buffer( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_reset_buffer();
	return -1;
}


RTLSDR_API int rtlsdr_read_sync( rtlsdr_dev_t *dev
							   , void *buf
							   , int len
							   , int *n_read
							   )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_read_sync((BYTE*) buf, len, n_read );
	return -1;
}


RTLSDR_API int rtlsdr_wait_async( rtlsdr_dev_t *dev
								, rtlsdr_read_async_cb_t cb
								, void *ctx
								)
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_wait_async( cb, ctx );
	return -1;
}


RTLSDR_API int rtlsdr_read_async( rtlsdr_dev_t *dev
								, rtlsdr_read_async_cb_t cb
								, void *ctx
								, uint32_t buf_num
								, uint32_t buf_len )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_read_async( cb, ctx, buf_num, buf_len );
	return -1;
}


RTLSDR_API int rtlsdr_cancel_async( rtlsdr_dev_t *dev )
{
	HERE();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_cancel_async();
	return -1;
}


RTLSDR_API const char* rtlsdr_get_version( void )
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_version();
}


RTLSDR_API unsigned __int64 rtlsdr_get_version_int64( void )
{
	HERE();
	rtlsdr rtl;
	return rtl.srtlsdr_get_version_int64();
}


RTLSDR_API int rtlsdr_get_tuner_bandwidths( rtlsdr_dev_t *dev
										  , int *bandwidths 
										  )
{
	HERE();
	return ((rtlsdr*) dev )->rtlsdr_get_tuner_bandwidths( bandwidths );
}


RTLSDR_API int rtlsdr_set_tuner_bandwidth( rtlsdr_dev_t *dev, int bandwidth )
{
	HERE();
	return ((rtlsdr*) dev )->rtlsdr_set_tuner_bandwidth( bandwidth );
}


RTLSDR_API int rtlsdr_get_bandwidth_set_name( rtlsdr_dev_t *dev
											, int nSet
											, char* pString
											)
{
	HERE();
	return ((rtlsdr*) dev )->rtlsdr_get_bandwidth_set_name( nSet, pString );
}


RTLSDR_API int rtlsdr_set_bandwidth_set( rtlsdr_dev_t *dev, int nSet )
{
	HERE();
	return ((rtlsdr*) dev )->rtlsdr_set_bandwidth_set( nSet );
}

/*!
 * Set the sample rate for the device, also selects the baseband filters
 * according to the requested sample rate for tuners where this is possible.
 *
 * \param dev the device handle given by rtlsdr_open()
 * \param samp_rate the sample rate to be set, possible values are:
 * 		    225001 - 300000 Hz
 * 		    900001 - 3200000 Hz
 * 		    sample loss is to be expected for rates > 2400000
 * \return 0 on success, -EINVAL on invalid rate
 */


#if defined( SET_SPECIAL_FILTER_VALUES )
RTLSDR_API int rtlsdr_set_if_values( rtlsdr_dev_t *	dev, BYTE regA, BYTE regB, DWORD ifFreq )
{
	HERE();
	return ((rtlsdr*) dev )->rtlsdr_set_if_values( dev, regA, regB, ifFreq );
}
#endif
