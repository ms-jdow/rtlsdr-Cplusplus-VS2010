// rtlsdr_app.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "stdafx.h"

#include "librtlsdr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#define TRAP trap
#else
#define TRAP HERE //__noop
#endif

#define HERE() /*\
{ \
	CStringA msg; \
	msg.Format( "%s\n", __FUNCTION__ ); \
	OutputDebugStringA( msg ); \
}
*/

void trap()
{

}

static CList< rtlsdr*, rtlsdr* > appdongles;

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

// CRTLDirectApp initialization
BOOL rtlsdr_app::InitInstance()
{
	HERE();
	CWinApp::InitInstance();

	HERE();
 	return TRUE;
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
	HERE();
			return dongle;
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
// EXPORTS
RTLSDR_API uint32_t rtlsdr_get_device_count( void )
{
	HERE();
	return rtlsdr::rtlsdr_get_device_count();
}

RTLSDR_API const char* rtlsdr_get_device_name( uint32_t index )
{
	HERE();
	return rtlsdr::rtlsdr_get_device_name( index );
}

RTLSDR_API int rtlsdr_get_device_usb_strings( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											)
{
	HERE();
		return rtlsdr::rtlsdr_get_device_usb_strings( index
													, manufact
													, product
													, serial
													);
}

RTLSDR_API int rtlsdr_get_index_by_serial(const char *serial)
{
	HERE();
	return rtlsdr::rtlsdr_get_index_by_serial( serial );
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
	TRAP();
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
	TRAP();
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
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )
				->rtlsdr_get_usb_strings(((rtlsdr*) dev )->GetDeviceHandle()
										 , manufact
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
	TRAP();
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
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_read_eeprom( data, offset, len );
	return -1;
}

RTLSDR_API int rtlsdr_set_center_freq( rtlsdr_dev_t *dev, uint32_t freq )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_center_freq( freq );
	return -1;
}

RTLSDR_API uint32_t rtlsdr_get_center_freq(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_center_freq();
	return -1;
}

RTLSDR_API int rtlsdr_set_freq_correction( rtlsdr_dev_t *dev, int ppm )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_freq_correction( ppm );
	return -1;
}

RTLSDR_API int rtlsdr_get_freq_correction( rtlsdr_dev_t *dev )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_freq_correction();
	return -1;
}

RTLSDR_API enum rtlsdr_tuner rtlsdr_get_tuner_type( rtlsdr_dev_t *dev )
{
	TRAP();
	if ( dev != NULL )
		return (enum rtlsdr_tuner) ((rtlsdr*) dev )->rtlsdr_get_tuner_type();
	return RTLSDR_TUNER_UNKNOWN;
}

RTLSDR_API int rtlsdr_get_tuner_gains( rtlsdr_dev_t *dev, int *gains )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_gains( gains );
	return -1;
}

RTLSDR_API int rtlsdr_set_tuner_gain(rtlsdr_dev_t *dev, int gain)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_gain( gain );
	return -1;
}

RTLSDR_API int rtlsdr_get_tuner_gain(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_tuner_gain();
	return -1;
}

RTLSDR_API int rtlsdr_set_tuner_if_gain(rtlsdr_dev_t *dev, int stage, int gain)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_if_gain( stage, gain );
	return -1;
}

RTLSDR_API int rtlsdr_set_tuner_gain_mode(rtlsdr_dev_t *dev, int manual)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_tuner_gain_mode( manual );
	return -1;
}


RTLSDR_API int rtlsdr_set_sample_rate(rtlsdr_dev_t *dev, uint32_t rate)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_sample_rate( rate );
	return -1;
}

RTLSDR_API uint32_t rtlsdr_get_sample_rate(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_sample_rate();
	return -1;
}

RTLSDR_API uint32_t rtlsdr_get_corr_sample_rate(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_corr_sample_rate();
	return -1;
}

RTLSDR_API int rtlsdr_set_testmode(rtlsdr_dev_t *dev, int on)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_testmode( on );
	return -1;
}

RTLSDR_API int rtlsdr_set_agc_mode(rtlsdr_dev_t *dev, int on)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_agc_mode( on );
	return -1;
}

RTLSDR_API int rtlsdr_set_direct_sampling(rtlsdr_dev_t *dev, int on)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_direct_sampling( on );
	return -1;
}

RTLSDR_API int rtlsdr_get_direct_sampling(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_direct_sampling();
	return -1;
}

RTLSDR_API int rtlsdr_set_offset_tuning( rtlsdr_dev_t *dev, int on )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_offset_tuning( on );
	return -1;
}

RTLSDR_API int rtlsdr_get_offset_tuning( rtlsdr_dev_t *dev )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_get_offset_tuning();
	return -1;
}

RTLSDR_API int rtlsdr_set_dithering( rtlsdr_dev_t *dev, int dither )
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_set_dithering( dither );
	return -1;
}

RTLSDR_API int rtlsdr_reset_buffer(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_reset_buffer();
	return -1;
}

RTLSDR_API int rtlsdr_read_sync(rtlsdr_dev_t *dev, void *buf, int len, int *n_read)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_read_sync((BYTE*) buf, len, n_read );
	return -1;
}

RTLSDR_API int rtlsdr_wait_async(rtlsdr_dev_t *dev, rtlsdr_read_async_cb_t cb, void *ctx)
{
	TRAP();
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
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_read_async( cb, ctx, buf_num, buf_len );
	return -1;
}

RTLSDR_API int rtlsdr_cancel_async(rtlsdr_dev_t *dev)
{
	TRAP();
	if ( dev != NULL )
		return ((rtlsdr*) dev )->rtlsdr_cancel_async();
	return -1;
}
