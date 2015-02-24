//	librtlsdr.h	// Defines the rtlsdr_dev class.

#pragma once

#define _DUMMY_	{ return -1; }

#include "libusb/libusb.h"
#include "ITuner.h"
#include "rtl-sdr.h"
#include "DongleArrays.h"
#include "Recognized Dongles.h"
#include "MyMutex.h"
#include "IRtlSdr.h"

#define FIR_LEN 16
class e4kTuner;
class fc0012Tuner;
class fc0013Tuner;
class fc2580Tuner;
class r82xxTuner;

//	Stages is a private mode to emphasize per stage control.
#define MAX_TUNER_GAIN_MODES GAIN_MODE_STAGES

class /*SDRDAPI */ rtlsdr : public IRtlSdr
{
public:
	rtlsdr									();
	~rtlsdr									();	// Unwind tuners.

protected:
	friend e4kTuner;
	friend fc0012Tuner;
	friend fc0013Tuner;
	friend fc2580Tuner;
	friend r82xxTuner;

	//	Registry and data parsing are handled in the registry and names class
#include "librtlsdr_registryAndNames.h"
	//	Dongle communications are handled by the rtlsdr_dongle_comms class
#include "librtlsdr_dongle_comms.h"

protected:	// The real work.
	int			basic_open					( uint32_t index
											, struct libusb_device_descriptor *out_dd
											);
	int			claim_opened_device			( void );


	int			rtlsdr_set_fir				( void );
	int			rtl2832_set_if_freq			( uint32_t freq
											, uint32_t *freq_out
											);
	int			set_spectrum_inversion		( int inverted );
	int			rtlsdr_set_sample_freq_correction
											( int ppm );
	
public:
	int			rtlsdr_set_xtal_freq		( uint32_t rtl_freq
											, uint32_t tuner_freq
											);
	int			rtlsdr_get_xtal_freq		( uint32_t *rtl_freq
											, uint32_t *tuner_freq
											);
	int			rtlsdr_write_eeprom			( uint8_t *data
											, uint8_t offset
											, uint16_t len
											);
	int			rtlsdr_read_eeprom			( uint8_t *data
											, uint8_t offset
											, uint16_t len
											);
	int			rtlsdr_read_eeprom			( eepromdata& data );
	int			rtlsdr_set_center_freq		( uint32_t in_freq );
	uint32_t	rtlsdr_get_center_freq		( void );
	int			rtlsdr_set_freq_correction	( int ppm );
	int			rtlsdr_get_freq_correction
											( void );
	int			rtlsdr_get_tuner_type		( void );
	int			rtlsdr_get_tuner_gains		( int *gains );
	int			rtlsdr_set_tuner_gain		( int gain );
	int			rtlsdr_get_tuner_gain		( void );
	int			rtlsdr_set_tuner_if_gain	( int stage
											, int gain
											);
	int			rtlsdr_get_tuner_stage_gains( uint8_t stage
											, int32_t *gains
											, char *description
											);
	int			rtlsdr_get_tuner_stage_count( void );
	int			rtlsdr_set_tuner_stage_gain	( uint8_t stage
											, int32_t gain
											);
	int			rtlsdr_get_tuner_stage_gain	( uint8_t stage );
	int			rtlsdr_set_tuner_gain_mode	( int mode );
	int			rtlsdr_set_sample_rate		( uint32_t samp_rate );
	uint32_t	rtlsdr_get_sample_rate		( void );
	uint32_t	rtlsdr_get_corr_sample_rate	( void );
	int			rtlsdr_set_testmode			( int on );
	int			rtlsdr_set_agc_mode			( int on );
	int			rtlsdr_set_direct_sampling	( int on );
	int			rtlsdr_get_direct_sampling
											( void );
	int			rtlsdr_set_offset_tuning	( int on );
	int			rtlsdr_get_offset_tuning	( void );
	int			rtlsdr_set_dithering		( int dither );
//	int			rtlsdr_get_tuner_type		( int index );
	int			rtlsdr_open					( uint32_t index );
	int			rtlsdr_close				( void );
	uint32_t	rtlsdr_get_tuner_clock		( void );
	int			rtlsdr_get_usb_strings		( char *manufact
											, char *product
											, char *serial
											);
	int			rtlsdr_get_usb_strings		( CString& manufact
											, CString& product
											, CString& serial
											);

public:
	// Static functions
	static uint32_t	rtlsdr_get_device_count	( void );
	static const char *
					rtlsdr_get_device_name	( uint32_t index );
	static int		rtlsdr_get_device_usb_strings
											( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											);
	static int	rtlsdr_get_index_by_serial	( const char *serial );

	static int	rtlsdr_static_get_tuner_type( int index );

protected:
	static void	GetCatalog					( void );
	static bool	reinitDongles				( void );

private:
	void		ClearVars					( void );

private:
	/* rtl demod context */
	uint32_t								rate; /* Hz */
	uint32_t								rtl_xtal; /* Hz */
	int										fir[ FIR_LEN ];
	int										direct_sampling;
	/* tuner context */
	enum rtlsdr_tuner						tuner_type;
	ITuner *								tuner;
	uint32_t								tun_xtal; /* Hz */
	uint32_t								freq; /* Hz */
	uint32_t								offs_freq; /* Hz */
	uint32_t								effective_freq; /* Hz */
	int										corr; /* ppm */
	int										gain; /* tenth dB */
	int										gain_mode;
	/* status */
	int										driver_active;
	ITuner*									tunerset[ RTLSDR_TUNER_R828D + 1 ];


	int										tuner_initialized;
	int										spectrum_inversion;
	static int								inReinitDongles;
};
