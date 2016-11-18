/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012-2014 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 * Copyright (C) 2014-2015 by Joanne Dow <jdow@earthlink.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//	This file is the basic rtlsdr command processing and general
//	coordination portion of the rtlsdr.dll project.

#include "stdafx.h"
#include "rtlsdr_app.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#include "librtlsdr.h"

/*
 * All libusb callback functions should be marked with the LIBUSB_CALL macro
 * to ensure that they are compiled with the same calling convention as libusb.
 *
 * If the macro isn't available in older libusb versions, we simply define it.
 */
//#ifndef LIBUSB_CALL
//#define LIBUSB_CALL
//#endif

/* libusb < 1.0.9 doesn't have libusb_handle_events_timeout_completed */
#ifndef HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED
#define libusb_handle_events_timeout_completed(ctx, tv, c) \
	libusb_handle_events_timeout(ctx, tv)
#endif

#define CATALOG_TIMEOUT		5		//	Not much changes in 5 seconds with dongles.

/* two raised to the power of n */
#define TWO_POW(n)		((double)(1ULL<<(n)))
#define APPLY_PPM_CORR( _val_, _ppm_ ) ((( _val_ ) * ( 1.0 + ( _ppm_ ) / 1e6 )))

#include "tuner_Dummy.h"
#include "tuner_e4k.h"
#include "tuner_fc0012.h"
#include "tuner_fc0013.h"
#include "tuner_fc2580.h"
#include "tuner_r82xx.h"

/*
 * FIR coefficients.
 *
 * The filter is running at XTal frequency. It is symmetric filter with 32
 * coefficients. Only first 16 coefficients are specified, the other 16
 * use the same values but in reversed order. The first coefficient in
 * the array is the outer one, the last, the last is the inner one.
 * First 8 coefficients are 8 bit signed integers, the next 8 coefficients
 * are 12 bit signed integers. All coefficients have the same weight.
 *
 * Default FIR coefficients used for DAB/FM by the Windows driver,
 * the DVB driver uses different ones
 */
static const int fir_default[ FIR_LEN ] =
{
	-54, -36, -41, -40, -32, -14, 14, 53,	/* 8 bit signed */
	101, 156, 215, 273, 327, 372, 404, 421	/* 12 bit signed */
};

int rtlsdr::inReinitDongles = false;
//SharedMemoryFile	rtlsdr::SharedDongleData;

rtlsdr::rtlsdr( void )
	: rate( 0 )
	, rtl_xtal( 0 )
	, direct_sampling( 0 )
	, is_direct_sampling( false )
	, tuner_type(( enum rtlsdr_tuner) 0 )
	, tuner( NULL )
	, tun_xtal( 0 )
	, freq( 0 )
	, offs_freq( 0 )
	, effective_freq( 0 )
	, nominal_if_freq( 0 )
	, corr( 0 )
	, gain( 0 )
	, gain_mode( GAIN_MODE_MANUAL )
	, rtl_gain_mode( 0 )
	, bias_tee( 0 )
	, ditheron( 1 )
	, tuner_bw_set( 0 )
	, tuner_bw_val( 0 )
	, tuner_initialized( -1 )
	, spectrum_inversion( 0 )
	// librtlsdr_dongle_comms
	, ctx( NULL )
	, devh( NULL )
	, i2c_repeater_on( 0 )
	, xfer_buf_num( 0 )
	, xfer_buf_len( 0 )
	, xfer( 0 )
	, xfer_buf( 0 )
	, cb( 0 )
	, cb_ctx( 0 )
	, async_status( RTLSDR_INACTIVE )
	, async_cancel( 0 )
	, xfer_errors( 0 )
	, dev_lost( 0 )
	, active_index( -1 )
	//	librtlsdr_registryAndNames
	//		no entries
{
//	if ( RtlSdrArea == NULL )
	{
		if ( SharedDongleData.active )
		{
			RtlSdrArea = SharedDongleData.sharedMem;
			Dongles = RtlSdrArea->dongleArray;
		}
		else
		{
			RtlSdrArea = &TestDongles;
			Dongles = TestDongles.dongleArray;
		}
	}

	memset( fir, 0, sizeof( fir ));
	memset( tunerset, 0, sizeof( tunerset ));
	int i = 0;
	tunerset[ i++ ] = new dummy_Tuner;
	tunerset[ i++ ] = new e4kTuner( this );
	tunerset[ i++ ] = new fc0012Tuner( this );
	tunerset[ i++ ] = new fc0013Tuner( this );
	tunerset[ i++ ] = new fc2580Tuner( this );
	tunerset[ i++ ] = new r82xxTuner( this, RTLSDR_TUNER_R820T );
	tunerset[ i++ ] = new r82xxTuner( this, RTLSDR_TUNER_R828D );
}

rtlsdr::~rtlsdr()
{
	rtlsdr_close();
	int i = 0;
	delete tunerset[ i++ ];
	delete tunerset[ i++ ];
	delete tunerset[ i++ ];
	delete tunerset[ i++ ];
	delete tunerset[ i++ ];
	delete tunerset[ i++ ];
	delete tunerset[ i++ ];
	SharedDongleData.Close();
}

void rtlsdr::ClearVars( void )
{
	m_dongle.Clear();
	rate = 0;
	rtl_xtal = 0;
	direct_sampling  = 0;
	tuner_type = ( enum rtlsdr_tuner) 0;
	tuner = NULL;
	tun_xtal = 0;
	freq = 0;
	offs_freq = 0;
	effective_freq = 0;
	corr = 0;
	gain = 0;
	tuner_initialized = -1;
	spectrum_inversion = 0;
	//	librtlsdr_dongle_comms
	ctx = NULL;
	devh = NULL;
	i2c_repeater_on = 0;
	xfer_buf_num = 0;
	xfer_buf_len = 0;
	xfer = 0;
	xfer_buf = 0;
	cb = 0;
	cb_ctx = 0;
	async_status = RTLSDR_INACTIVE;
	async_cancel = 0;
	xfer_errors = 0;
	dev_lost = 0;
	//	librtlsdr_registryAndNames
	//		no entries


}



#define DEF_RTL_XTAL_FREQ	28800000
#define MIN_RTL_XTAL_FREQ	(DEF_RTL_XTAL_FREQ - 1000)
#define MAX_RTL_XTAL_FREQ	(DEF_RTL_XTAL_FREQ + 1000)




int rtlsdr::rtlsdr_set_fir( void )
{
	uint8_t lfir[ 20 ];

	int i;
	/* format: int8_t[8] */
	for ( i = 0; i < 8; ++i )
	{
		const int val = fir[ i ];
		if ( val < -128 || val > 127 )
		{
			return -1;
		}
		lfir[ i ] = val;
	}
	/* format: int12_t[ 8 ] */
	for ( i = 0; i < 8; i += 2 )
	{
		const int val0 = fir[ 8 + i ];
		const int val1 = fir[ 8 + i + 1 ];
		if ( val0 < -2048 || val0 > 2047 || val1 < -2048 || val1 > 2047 )
		{
			return -1;
		}
		lfir[ 8 + i * 3 / 2 ] = val0 >> 4;
		lfir[ 8 + i * 3 / 2 + 1 ] = ( val0 << 4 ) | (( val1 >> 8 ) & 0x0f );
		lfir[ 8 + i * 3 / 2 + 2 ] = val1;
	}

	for ( i = 0; i < (int) sizeof( lfir ); i++ )
	{
		if ( rtlsdr_demod_write_reg( 1, 0x1c + i, lfir[ i ], 1 ))
				return -1;
	}

	return 0;
}

/*	Protected function not visible outside. Currently only used by R820x */
int rtlsdr::rtlsdr_set_if_freq( uint32_t in_freq, uint32_t *freq_out )
{
	uint32_t rtl_xtal;
	int32_t if_freq;
	uint8_t tmp;
	int r;

	if (!devh)
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	/* read corrected clock value */
	if ( rtlsdr_get_xtal_freq( &rtl_xtal, NULL ))
		return -2;

	// Step sizes are apparently 28,8e6/2^22 or a touch under 7 Hz.
	if_freq = (int32_t) (( rtl_xtal / 2 + (uint64_t) in_freq * TWO_POW( 22 )) / rtl_xtal ) * ( -1 );
	if ( if_freq <= -0x200000 )			//	-2097152 Effectively rtl_xtal/2 nominally 14.4 MHz
	{
		fprintf(stderr, "rtlsdr_set_if_freq(): %u Hz out of range for downconverter if_freq = %d\n", in_freq, if_freq);
		TRACE( "rtlsdr_set_if_freq(): %u Hz out of range for downconverter if_freq = %d\n", in_freq, if_freq);
		return -2;
	}

	tmp = ( if_freq >> 16 ) & 0x3f;
	r = rtlsdr_demod_write_reg( 1, 0x19, tmp, 1 );
	tmp = ( if_freq  >> 8 ) & 0xff;
	r |= rtlsdr_demod_write_reg( 1, 0x1a, tmp, 1 );
	tmp = if_freq & 0xff;
	r |= rtlsdr_demod_write_reg( 1, 0x1b, tmp, 1 );

	if ( freq_out )
		*freq_out = (uint32_t) (((int64_t) if_freq * rtl_xtal * -1 + TWO_POW( 21 )
								) / TWO_POW( 22 ));

	return r;
}

int rtlsdr::rtlsdr_set_sample_freq_correction( int ppm )
{
	int r = 0;
	uint8_t tmp;
	//	Protect the math below.
	if ( abs( ppm ) > 250 )
	{
		if ( ppm > 0 )
			ppm = 250;
		else
			ppm = -250;
	}

	int16_t offs = ( int16_t ) ( ppm * ( -1 ) * TWO_POW( 24 ) / 1000000 );

	rtlsdr_set_i2c_repeater( 0 );

	tmp = offs & 0xff;
	r |= rtlsdr_demod_write_reg( 1, 0x3f, tmp, 1 );
	tmp = ( offs >> 8 ) & 0x3f;
	r |= rtlsdr_demod_write_reg( 1, 0x3e, tmp, 1 );

	return r;
}

int rtlsdr::rtlsdr_set_xtal_freq( uint32_t rtl_freq, uint32_t tuner_freq )
{
	int r = 0;

	if ( !devh  )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	if (( rtl_freq > 0 )
	&&	(( rtl_freq < MIN_RTL_XTAL_FREQ ) || ( rtl_freq > MAX_RTL_XTAL_FREQ )))
		return -2;

	if (( rtl_freq > 0 ) && ( rtl_xtal != rtl_freq ) || is_direct_sampling)
	{
		rtl_xtal = rtl_freq;

		/* update xtal-dependent settings */
		if ( rate )
			r = rtlsdr_set_sample_rate( rate );
	}

	if (( tuner != NULL ) && ( tun_xtal != tuner_freq ))
	{
		if ( tuner_freq == 0 )
			tun_xtal = rtl_xtal;
		else
			tun_xtal = tuner_freq;

		/* read corrected clock value into e4k and r82xx structure */
		uint32_t osc;
		if (( rtlsdr_get_xtal_freq( NULL, &osc ) < 0 )
		||	( tuner == NULL )
		||	( tuner->set_xtal_frequency( osc ) < 0 ))
			return -3;

		/* update xtal-dependent settings */
		if ( freq != 0 )
			r = rtlsdr_set_center_freq( freq );
	}

	return r;
}

int rtlsdr::rtlsdr_get_xtal_freq( uint32_t *rtl_freq, uint32_t *tuner_freq )
{
	if ( !devh )
		return -1;

	if ( rtl_freq )
		*rtl_freq = (uint32_t) APPLY_PPM_CORR( rtl_xtal, corr );

	if ( tuner_freq )
		*tuner_freq = (uint32_t) APPLY_PPM_CORR( tun_xtal,  corr );

	return 0;
}

int rtlsdr::rtlsdr_get_usb_strings( char *manufact
								  , char *product
								  , char *serial
								  )
{
	eepromdata data;
	memset( data, 0xff, sizeof( eepromdata ));
	int r = rtlsdr_read_eeprom_raw( data );
	if ( r >= 0 )
	{
		r = GetEepromStrings( data
							, EEPROM_SIZE
							, manufact
							, product
							, serial
							);
	}
	return r;
}

int rtlsdr::rtlsdr_get_usb_cstrings( CString& manufact
								   , CString& product
								   , CString& serial
								   )
{
	eepromdata data;
	memset( data, 0xff, sizeof( eepromdata ));
	int r = rtlsdr_read_eeprom_raw( data );
	if ( r >= 0 )
	{
		r = GetEepromStrings( data
							, &manufact
							, &product
							, &serial
							);
	}
	return r;
}

//	Get the USB strings the old way with CStrings
int rtlsdr::rtlsdr_get_usb_strings_canonical( CString& manufact
											, CString& product
											, CString& serial
											)
{
	char man[ 256 ];
	char prd[ 256 ];
	char ser[ 256 ];
	int ret = rtlsdr_get_usb_strings_canonical( man, prd, ser );
	if ( ret >= 0 )
	{
		manufact = man;
		product = prd;
		serial = ser;
	}
	return ret;
}

//	Get the USB strings the old way with char pointers
int rtlsdr::rtlsdr_get_usb_strings_canonical( char *manufact
											, char *product
											, char *serial
											)
{
	struct libusb_device_descriptor dd;
	libusb_device *device = NULL;
	const int buf_max = 256;
	int r = 0;

	device = libusb_get_device( devh );

	r = libusb_get_device_descriptor( device, &dd );
	if ( r < 0 )
		return -1;

	if ( manufact )
	{
		memset( manufact, 0, buf_max );
		libusb_get_string_descriptor_ascii( devh
										  , dd.iManufacturer
										  , (unsigned char *) manufact
										  , buf_max
										  );
	}

	if ( product )
	{
		memset( product, 0, buf_max );
		libusb_get_string_descriptor_ascii( devh
										  , dd.iProduct
										  , (unsigned char *) product
										  , buf_max
										  );
	}

	if ( serial )
	{
		memset( serial, 0, buf_max );
		libusb_get_string_descriptor_ascii( devh
										  , dd.iSerialNumber
										  , (unsigned char *) serial
										  , buf_max
										  );
	}

	return 0;
}


int rtlsdr::rtlsdr_write_eeprom( uint8_t *data, uint8_t offset, uint16_t len )
{
	if (( !devh ) || (( offset + len ) > EEPROM_SIZE ))
		return -1;
	//	Parse the eeprom to see if it is "real"
	//	Find it in the Dongles list.

	//	Read the current eeprom data to tdata. Save it for later and
	//	use it as template in case offset != 0 and offset + len < EEPROM_SIZE.
	eepromdata olddata ={ 0xff };

	int r = rtlsdr_read_eeprom_raw( olddata );
	if ( r < 0 )
		return r;

	Dongle olddongle;
	r = FullEepromParse( olddata, olddongle );
	olddongle.found = active_index;
	olddongle.tunerType = tuner_type;

	eepromdata workdata;
	memcpy( workdata, olddata, sizeof( eepromdata ));
	memcpy( &workdata[ offset ], data, len );
	
	Dongle tdongle;
	r = FullEepromParse( workdata, tdongle );
	if ( r < 0 )
		return -4;						// Not safe to write

	r = rtlsdr_write_eeprom_raw( workdata );

	if ( r >= 0 )
	{
		int parsed;
		{
			CMutexLock cml( dongle_mutex );
			//	Find our old entry in current list
			olddongle.busy = true;
			parsed = SafeFindDongle( olddongle );
			if ( parsed >= 0 )
			{
				//	Got ourself in the database update it.
				tdongle.tunerType = tuner_type;
				//	TODOTODO which of these two options is right?
				tdongle.found = olddongle.found;
//				tdongle.found = parsed;
				tdongle.busy = Dongles[ parsed ].busy;
				Dongles[ parsed ] = tdongle;
			}
		}
		//	Exit the dongle lock before entring the registry lock.
		//	It's a race that's too tiny to worry about with dongles.
		if ( parsed >= 0 )
		{
			CMutexLock cml( registry_mutex );
			WriteSingleRegistry( parsed );
			r = 0;
		}
		else
		{
			//	Not in the database - so do we add it? Maybe.
			r = -1;	// Unknown dongle
		}
	}

	return r;
}

int rtlsdr::rtlsdr_read_eeprom( uint8_t *data, uint8_t offset, uint16_t len )
{
	eepromdata tdata;
	memset( data, 0xff, sizeof( eepromdata ));
	//	Read the whole block.
	int r =  rtlsdr_read_eeprom( tdata );
	if ( r < 0 )
		return r;

	//	Copy data where it belongs
	memcpy( data + offset, tdata, min( len, sizeof( eepromdata ) - offset ));

	return r;
}

int rtlsdr::rtlsdr_read_eeprom( eepromdata& data )
{
	eepromdata tdata;
	memset( data, 0xff, sizeof( eepromdata ));
	//	Read the whole block.
	int r =  rtlsdr_read_eeprom_raw( tdata );
	if ( r < 0 )
		return r;

	//	Parse our copy the eeprom data block to see if it is "real"
	Dongle tdongle;
	r = FullEepromParse( tdata, tdongle );
	if ( r >= 0 )
	{
		GetCatalog();					//	Make sure we have a db to search

		tdongle.tunerType = tuner_type;	//	We know our tuner t
		tdongle.busy = true;			//	Of course

		bool changed = false;
		int parsed = -1;
		{
			CMutexLock cml( dongle_mutex );
			if ( TestMaster())
				//	Find our new entry since database is already updated.
				parsed = SafeFindDongle( tdongle );
			else
				//	Find our old entry.
				parsed = SafeFindDongle( m_dongle );

			if ( !parsed )
			{
				//	if not found try to find us by USB path, VID, and pID only.
				tdongle.busy = true;
				parsed = SafeFindDongle( tdongle );
			}

			ASSERT( parsed >= 0 );
			if ( parsed >= 0 )
			{
				Dongle *work = &Dongles[ parsed ];
				if ( !work->busy )
				{
					//	TODOTODO the found value is iffy here...
					//	Alas, I don't find a safe workaround- yet.
					work->busy = true;	//	Of course. Should already be marked.
					changed = true;
				}
			}
			else
			{
				//	Um, simply add it to the database?
				//	By definition we should not be here
				tdongle.tunerType = tuner_type;
				Dongles[ RtlSdrArea->activeEntries++ ] = tdongle;
				changed = true;
			}
		}

		if ( changed )
		{
			CMutexLock cml( registry_mutex );
			WriteSingleRegistry( parsed );
		}
	}
	else
	{
		r = r;
		return -9;	// Failed to parse here.
	}
	memcpy( data, tdata, sizeof( eepromdata ));

	return 0;
}

int rtlsdr:: set_spectrum_inversion( int inverted )
{
	int r = 0;

	if ( spectrum_inversion == inverted )
		return r;

	r |= rtlsdr_demod_write_reg( 1, 0x15, inverted, 1 );

	/* reset demod (bit 3, soft_rst) */
	r |= rtlsdr_demod_write_reg( 1, 0x01, 0x14, 1 );
	r |= rtlsdr_demod_write_reg( 1, 0x01, 0x10, 1 );

	spectrum_inversion = inverted;
	return r;
}

int rtlsdr::rtlsdr_set_center_freq( uint32_t in_freq )
{
	int r = -1;
	int rc = 0;
	uint32_t tuner_lo;					/* Actual tuner LO frequency. */
	uint32_t tuner_if;					/* Desired tuner IF frequency. */
	uint32_t actual_if = 0;
	int inverted;

	if ( tuner == NULL )
		return rc;

	//	Even if we have direct sampling set bypass it and use real sampling
	//	when the input frequency is too high.
	if ( direct_sampling && ( in_freq <= 24000000 ))
	{
		if ( !is_direct_sampling )
			rtlsdr_do_direct_sampling( true );
		//  This path works for all dongles - sorta kinda.
		tuner_lo = 0;
	}
	else
	{
		if ( is_direct_sampling )
		{
			freq = in_freq;
			rtlsdr_do_direct_sampling( false );
		}

		//	This path will try to tune the tuner to the indicated frequency
		//	with some exceptions.
		//	For R820T family dongles this path will go into a tuner bypass
		//	mode automatically at 14.4MHz. (TODO - is this optimum?)
		rtlsdr_set_i2c_repeater( 1 );
		r = tuner->set_freq( in_freq - offs_freq, &tuner_lo );
		rtlsdr_set_i2c_repeater( 0 );
		if ( r < 0 )
			return rc;
	}

	if ( tuner_lo > in_freq )
	{
		/* high-side mixing, enable spectrum inversion */
		tuner_if = tuner_lo - in_freq;
		inverted = 1;
	}
	else
	{
		/* low-side mixing, or zero-IF, or direct sampling; disable spectrum inversion */
		tuner_if = in_freq - tuner_lo;
		inverted = 0;
	}

	r = set_spectrum_inversion( inverted );
	if ( r < 0 )
		rc = r;						/* We valiantly try to continue */

	/*	Set 2832 chip IF frequency as best we can. */
	r = rtlsdr_set_if_freq( tuner_if, &actual_if );
	if ( r < 0 )
		rc = r;						/* We valiantly try to continue */

	freq = in_freq;

	if ( inverted )
		effective_freq = tuner_lo - actual_if;
	else
		effective_freq = tuner_lo + actual_if;

	return r;
}

uint32_t rtlsdr::rtlsdr_get_center_freq( void )
{
	return effective_freq;
}

int rtlsdr::rtlsdr_set_freq_correction( int ppm )
{
	int r = 0;

	if ( devh == NULL )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	if ( corr == ppm )
		return -2;

	corr = ppm;

	r |= rtlsdr_set_sample_freq_correction( ppm );

	/* read corrected clock value into e4k and r82xx structure */
	uint32_t osc;
	if (( rtlsdr_get_xtal_freq( NULL, &osc ) < 0 )
	||	( tuner == NULL )
	||	( tuner->set_xtal_frequency( osc ) < 0 ))
		return -3;

	if ( freq ) /* retune to apply new correction value */
		r |= rtlsdr_set_center_freq( freq );

	return r;
}

int rtlsdr::rtlsdr_get_freq_correction( void )
{
	return corr;
}

int rtlsdr::rtlsdr_get_tuner_type( void )
{
	if ( !devh || !tuner )
		return RTLSDR_TUNER_UNKNOWN;

	return tuner_type;
}

int rtlsdr::rtlsdr_get_tuner_type( int index )
{
	return rtlsdr_static_get_tuner_type( index );
}

//	STATIC
int	rtlsdr::rtlsdr_static_get_tuner_type( int index )
{
	if ((DWORD) index < (DWORD) RtlSdrArea->activeEntries )
	{
		int tindex = 0;
		for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
		{
			if ( Dongles[ i ].found == index )
			{
				return Dongles[ i ].tunerType;
			}
		}
	}
	return RTLSDR_TUNER_UNKNOWN;
}


int rtlsdr::rtlsdr_get_tuner_bandwidths	( int *bandwidths )
{
	const uint32_t *ptr;
	int len;

	if ( tuner == NULL )
		return -1;

	if (( tuner_type == RTLSDR_TUNER_R820T )
	||	( tuner_type == RTLSDR_TUNER_R828D ))
	{
		tuner->get_tuner_bandwidths( &ptr, &len );
		if ( bandwidths )
		{
			/* buffer provided, copy data */
			memcpy( bandwidths, ptr, len * sizeof( int ));
		}
		return len;
	}
	else
	{
		if ( bandwidths )
			*bandwidths = 0;
		return 1;
	}
}


int rtlsdr::rtlsdr_get_tuner_bandwidths_safe( int *bandwidths
											, int in_len
											)
{
	const uint32_t *ptr;
	int len;

	if ( tuner == NULL )
		return -1;

	if (( tuner_type == RTLSDR_TUNER_R820T )
	||	( tuner_type == RTLSDR_TUNER_R828D ))
	{
		tuner->get_tuner_bandwidths( &ptr, &len );
		if ( bandwidths && ( in_len >= len ))
		{
			/* buffer provided, copy data */
			memcpy( bandwidths, ptr, len );
		}
		return len / sizeof( int );
	}
	else
	{
		if ( bandwidths && ( in_len >= sizeof( int )))
		{
			/* buffer provided, set data to 0 */
			*bandwidths = 0;
		}
		return 1;
	}
}


int rtlsdr::rtlsdr_get_bandwidth_set_name( int nSet
										 , char* pString
										 )
{
	if (( pString == NULL )
	||  ( tuner == NULL ))
		return -1;

	return tuner->get_bandwidth_set_name( nSet, pString );
}


int rtlsdr::rtlsdr_set_bandwidth_set( int nSet )
{
	if ( !devh || !tuner || is_direct_sampling )
		return -1;
	int r = tuner->set_bandwidth_set( nSet );
	if ( r >= 0 )
		tuner_bw_set = nSet;
	return r;
}


int rtlsdr::rtlsdr_set_tuner_bandwidth( int bandwidth )
{
	int r = 0;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	if (( tuner_type == RTLSDR_TUNER_R820T )
	||	( tuner_type == RTLSDR_TUNER_R828D ))
	{
		rtlsdr_set_i2c_repeater(  1 );
		r = tuner->set_bw( bandwidth );
		rtlsdr_set_i2c_repeater( 0 );
		if ( r >= 0 )
		{
			tuner_bw_val = bandwidth;
			/* This also sets required IF */
			rtlsdr_set_center_freq( freq );
		}
	}
	return r;
}


int rtlsdr::rtlsdr_get_tuner_gains( int *gains )
{
	const int *ptr = NULL;
	int len = 0;

	if ( tuner )
	{
		//	Get tuner gains and gains length in BYTES.
		//	This is corrected below.
		if ( gains == NULL )
			tuner->get_tuner_gains( NULL, &len );
		else
			tuner->get_tuner_gains( &ptr, &len );
	}

	if ( gains && ( len > 0 ))
		memcpy( gains, ptr, len );
		
	return len / sizeof( int );
}

int rtlsdr::rtlsdr_set_tuner_gain( int in_gain )
{
	int r = 0;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	//	If we are in per stage gain mode protect
	//	the gains as individually setup.
	if ( gain_mode < GAIN_MODE_STAGES )
	{
		rtlsdr_set_i2c_repeater( 1 );
		r = tuner->set_gain( in_gain );
		rtlsdr_set_i2c_repeater( 0 );
	}

	if ( r >= 0 )
		gain = in_gain;
	else
		gain = 0;

	return r;
}

int rtlsdr::rtlsdr_get_tuner_gain( void )
{
	if ( tuner == NULL )
		return 0;

	//	Always return what the tuner's opinion on the matter.
	return tuner->get_gain();
}

int rtlsdr::rtlsdr_set_tuner_if_gain( int stage, int gain )
{
	int r = 0;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	rtlsdr_set_i2c_repeater( 1 );
	r = tuner->set_if_gain( stage, gain );
	rtlsdr_set_i2c_repeater( 0 );

	//	TODO - these should be cached someday...
	return r;
}

int rtlsdr::rtlsdr_get_tuner_stage_gains( uint8_t stage, int32_t *gains, char *description)
{
	const int32_t *gainptr = NULL;
	const char *desc;
	int len = 0;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	len = tuner->get_tuner_stage_gains( stage, &gainptr, &desc );

	if (! gains )
	{
		/* no buffer provided, just return the count */
		return len;
	}
	else
	{
		if ( len > 0 )
			memcpy( gains, gainptr, len * sizeof( *gains ));
		if ( description )
		{
			memcpy( description, desc, DESCRIPTION_MAXLEN - 1 );
			description[ DESCRIPTION_MAXLEN - 1 ] = 0;
		}
		return len;
	}
}

int rtlsdr::rtlsdr_get_tuner_stage_count( void )
{
	if ( !devh || !tuner || is_direct_sampling )
		return -1;
	return tuner->get_tuner_stage_count();
}

int rtlsdr::rtlsdr_get_tuner_stage_gain( uint8_t stage )
{
	if ( !devh || !tuner || is_direct_sampling )
		return 0;

	return tuner->get_tuner_stage_gain( stage );
}

int rtlsdr::rtlsdr_set_tuner_stage_gain( uint8_t stage, int32_t gain )
{
	int rc;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	rtlsdr_set_i2c_repeater( 2 );
	rc = tuner->set_tuner_stage_gain( stage, gain );
	rtlsdr_set_i2c_repeater( 0 );
	//	TODOTODO - someday this should get remembered.
	return rc;
}

int rtlsdr::rtlsdr_set_tuner_gain_mode( int mode )
{
	int r = 0;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	if ((DWORD) mode < (DWORD) GAIN_MODE_STAGES )
	{
		rtlsdr_set_i2c_repeater( 1 );
		r = tuner->set_gain_mode( mode );
		rtlsdr_set_i2c_repeater( 0 );
	}
	gain_mode = mode;
	return r;
}


int rtlsdr::rtlsdr_set_sample_rate( uint32_t samp_rate )
{
	int r = 0;
	uint16_t tmp;
	uint32_t rsamp_ratio;
	uint32_t real_rsamp_ratio;
	double real_rate;

	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	/* check if the rate is supported by the resampler */
	if (( samp_rate <= 225000 )
	||	( samp_rate > 3200000 )
	||	(( samp_rate > 300000 )
	 &&  ( samp_rate <= 900000 )))
	{
		fprintf(stderr, "Invalid sample rate: %u Hz\n", samp_rate);
		TRACE( "Invalid sample rate: %u Hz\n", samp_rate);
		return -EINVAL;
	}

	rsamp_ratio = (uint32_t) (( rtl_xtal * TWO_POW( 22 )) / samp_rate );
	rsamp_ratio &= 0x0ffffffc;

	real_rsamp_ratio = rsamp_ratio | (( rsamp_ratio & 0x08000000 ) << 1 );
	real_rate = ( rtl_xtal * TWO_POW( 22 )) / real_rsamp_ratio;

	if (((double) samp_rate) != real_rate )
	{
		fprintf(stderr, "Exact sample rate is: %f Hz\n", real_rate);
		TRACE( "Exact sample rate is: %f Hz\n", real_rate);
	}

	if (( tuner != NULL ) && !is_direct_sampling )
	{
		rtlsdr_set_i2c_repeater( 1 );
		r = tuner->set_bw((int) real_rate );
		rtlsdr_set_i2c_repeater( 0 );
		if (( r >= 0 )
		&&	(( tuner_type == RTLSDR_TUNER_R820T )
		||	 ( tuner_type == RTLSDR_TUNER_R828D )))
		{
			nominal_if_freq = r;
			if ( freq > 0 )
				rtlsdr_set_center_freq( freq );	// rtlsdr_set_if_freq
			else
				rtlsdr_set_center_freq( 500000000 );	// rtlsdr_set_if_freq
			r = 0;
		}
		rate = (uint32_t) real_rate;

		tmp = ( rsamp_ratio >> 16 );
		r |= rtlsdr_demod_write_reg( 1, 0x9f, tmp, 2 );
		tmp = rsamp_ratio & 0xffff;
		r |= rtlsdr_demod_write_reg( 1, 0xa1, tmp, 2 );

		r |= rtlsdr_set_sample_freq_correction( corr );

		/* reset demod (bit 3, soft_rst) */
		r |= rtlsdr_demod_write_reg( 1, 0x01, 0x14, 1 );
		r |= rtlsdr_demod_write_reg( 1, 0x01, 0x10, 1 );

		/* recalculate offset frequency if offset tuning is enabled */
		if ( offs_freq )
			rtlsdr_set_offset_tuning( 1 );
	}
	return r;
}

uint32_t rtlsdr::rtlsdr_get_sample_rate( void )
{
	return rate;
}

uint32_t rtlsdr::rtlsdr_get_corr_sample_rate( void )
{
	return (uint32_t) APPLY_PPM_CORR( rate,  corr );
}

int rtlsdr::rtlsdr_set_testmode( int on )
{
	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	return rtlsdr_demod_write_reg( 0, 0x19, on ? 0x03 : 0x05, 1 );
}

int rtlsdr::rtlsdr_set_agc_mode( int on )
{
	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	rtl_gain_mode = on;

	return rtlsdr_demod_write_reg( 0, 0x19, on ? 0x25 : 0x05, 1);
}

int rtlsdr::rtlsdr_set_bias_tee( int on )
{
	if ( !devh || !tuner || is_direct_sampling )
 		return -1;

	rtlsdr_set_gpio_output( 0 );
	rtlsdr_set_gpio_bit( 0, on );
	bias_tee = on;
	return 0;
}

int rtlsdr::rtlsdr_set_direct_sampling( int on )
{
	if ( !devh )
		return -1;

	if ( on == direct_sampling )
		return 0;

//	if ( direct_sampling )
//		rtlsdr_do_direct_sampling( true );
	direct_sampling = on;

#if 0
	/* retune now that we have changed the config */
	//	But note that the center frequency may not
	//	have been set yet.
	int r = -1;
	if ( freq > offs_freq )
		r = rtlsdr_set_center_freq( freq );
	return r;
#else
	return 0;
#endif
}


int rtlsdr::rtlsdr_do_direct_sampling( bool on )
{
	int r = 0;

	rtlsdr_set_i2c_repeater( 0 );

	/* common to all direct modes */
	if (( direct_sampling != 0 ) && on )
	{
		if ( tuner != NULL )
		{
			rtlsdr_set_i2c_repeater( 1 );
			r = tuner->exit();
			rtlsdr_set_i2c_repeater( 0 );
		}

		/* disable Zero-IF mode */
		r |= rtlsdr_demod_write_reg( 1, 0xb1, 0x1a, 1 );

		/* only enable In-phase ADC input */
		r |= rtlsdr_demod_write_reg( 0, 0x08, 0x4d, 1 );

		/* swap I and Q ADC, this allows selection between two inputs */
		r |= rtlsdr_demod_write_reg( 0, 0x06, ( direct_sampling == 2 ) ? 0x90 : 0x80, 1);

		/* disable spectrum inversion */
		r |= set_spectrum_inversion( 0 );

		fprintf( stderr, "Enabled direct sampling mode, input %i\n", direct_sampling );
		TRACE( "Enabled direct sampling mode, input %i\n", direct_sampling );

		/* Direct sampling is active now. */
		is_direct_sampling = true;
	}
	else
	{
		/* direct sampling is not active */
		is_direct_sampling = false;

		if ( tuner != NULL )
		{
			rtlsdr_set_i2c_repeater( 1 );
			r |= tuner->init( this ); 
			rtlsdr_set_i2c_repeater( 0 );
		}

		if (( tuner_type == RTLSDR_TUNER_R820T )
		||	( tuner_type == RTLSDR_TUNER_R828D ))
		{
			/* disable Zero-IF mode */
			r |= rtlsdr_demod_write_reg( 1, 0xb1, 0x1a, 1 );

			/* only enable In-phase ADC input */
			r |= rtlsdr_demod_write_reg( 0, 0x08, 0x4d, 1 );

			/* the R82XX use 3.57 MHz IF for the DVB-T 6 MHz mode, and
			 * 4.57 MHz for the 8 MHz mode */
			rtlsdr_set_if_freq( R82XX_DEFAULT_IF_FREQ, NULL );

			/* enable spectrum inversion */
			rtlsdr_demod_write_reg( 1, 0x15, 0x01, 1 );
		}
		else
		{
			/* enable In-phase + Quadrature ADC input */
			r |= rtlsdr_demod_write_reg( 0, 0x08, 0xcd, 1 );

			/* Enable Zero-IF mode */
			r |= rtlsdr_demod_write_reg( 1, 0xb1, 0x1b, 1 );
		}

		/* Now we fill in the last known good tuner values as best we can. */
		if ( tuner != NULL )
			tuner->set_xtal_frequency( rtl_xtal );

		rtlsdr_set_sample_rate( rate );

		/* opt_adc_iq = 0, default ADC_I/ADC_Q datapath */
		r |= rtlsdr_demod_write_reg( 0, 0x06, 0x80, 1 );

		//	Now reset operating modes.

		//	rtlagcmode                  rtl_gain_mode
		rtlsdr_set_i2c_repeater( 0 );
		rtlsdr_demod_write_reg( 0, 0x19, rtl_gain_mode ? 0x25 : 0x05, 1);

		//	gain mode                   gainmode
		rtlsdr_set_i2c_repeater( 1 );
		tuner->set_gain_mode( gain_mode );

		//	gain                        gain
		tuner->set_gain( gain );

		//	Offset                      offs_freq
		rtlsdr_set_offset_tuning( offs_freq != 0 );


		//	stage gains                 TODO
//		//	xtalfrequency               rtl_xtal, tun_xtal
//		int rx = rtl_xtal;
//		int tx = tun_xtal;
//		rtl_xtal = DEF_RTL_XTAL_FREQ;
//		tun_xtal = DEF_RTL_XTAL_FREQ;
//		rtlsdr_set_xtal_freq( rtl_xtal, tun_xtal );

		//	Frequency Correction        corr
		int ppm = corr;
		corr = 0;
		rtlsdr_set_freq_correction( ppm );

		//	Balances
		//	TODOTODO

		//	IFBwSet                     tuner_bw_set
		tuner->set_bandwidth_set( tuner_bw_set );

		//	IFBwHz                      tuner_bw_val
		rtlsdr_set_tuner_bandwidth( tuner_bw_val );

		//	dithering                   ditheron
		tuner->set_dither( ditheron );

		//	bias t                      bias_tee    BOTH ON AND OFF need work.		
		rtlsdr_set_bias_tee( bias_tee );

		fprintf( stderr, "Disabled direct sampling mode\n" );
		TRACE( "Disabled direct sampling mode\n");
	}

	return r;
}

int rtlsdr::rtlsdr_get_direct_sampling( void )
{
	return direct_sampling;
}

int rtlsdr::rtlsdr_set_offset_tuning( int on )
{
	int r = 0;

	if ( !devh || !tuner || is_direct_sampling )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	if (( tuner_type == RTLSDR_TUNER_R820T )
	||	( tuner_type == RTLSDR_TUNER_R828D ))
		return -2;

	if ( direct_sampling )	//  Note that R820T dongles do not get here.
		return -3;

	/* based on keenerds 1/f noise measurements */
	offs_freq = on ? (( rate / 2 ) * 170 / 100 ) : 0;

	rtlsdr_set_i2c_repeater( 1 );
	r = tuner->set_bw( on ? ( 2 * offs_freq ) : rate );
	rtlsdr_set_i2c_repeater( 0 );
	//	Of course no special R820x dongle stuff is needed.


	/* retune now that we have changed the config */
	//	But note that the center frequency may not
	//	have been set yet.
	if ( freq > offs_freq )
		r = rtlsdr_set_center_freq( freq );

	return r;
}

int rtlsdr::rtlsdr_get_offset_tuning( void )
{
	return ( offs_freq != NULL ) ? 1 : 0;
}

int rtlsdr::rtlsdr_set_dithering( int dither )
{
	if ( tuner_type == RTLSDR_TUNER_R820T )
	{
		int r = -1;
		if ( !devh || !tuner || is_direct_sampling )
			r = tuner->set_dither( dither );
		if ( r =  0 )
			ditheron = dither;
	}
	return -1;
}

uint32_t rtlsdr::rtlsdr_get_device_count( void )
{
	GetCatalog();
	return RtlSdrArea->activeEntries;
}

// STATIC //
uint32_t rtlsdr::srtlsdr_get_device_count( void )
{
	GetCatalog();
	return RtlSdrArea->activeEntries;
}

const char *rtlsdr::rtlsdr_get_device_name( uint32_t index )
{
	return srtlsdr_get_device_name( index );
}

// STATIC //
const char *rtlsdr::srtlsdr_get_device_name( uint32_t index )
{
	libusb_context *ctx;
	libusb_init( &ctx );

	const char* name = find_requested_dongle_name( ctx, index );
	
	libusb_exit( ctx );

	return name;
}

int rtlsdr::rtlsdr_get_device_usb_strings( uint32_t index
										 , char *manufact
										 , char *product
										 , char *serial
										 )
{
	return srtlsdr_get_device_usb_strings( index, manufact, product, serial );
}

int rtlsdr::rtlsdr_get_device_usb_cstrings( uint32_t index
										  , CString& manufact
										  , CString& product
										  , CString& serial
										  )
{
	return srtlsdr_get_device_usb_strings( index, manufact, product, serial );
}

// STATIC //
int rtlsdr::srtlsdr_get_device_usb_strings( uint32_t index
										  , char *manufact
										  , char *product
										  , char *serial
										  )
{
	int r = -2;
	const rtlsdr_dongle_t *device = NULL;
	uint32_t device_count = 0;

	GetCatalog();
	switch( goodCatalog )
	{
	case true:
		{
			CMutexLock cml( dongle_mutex );
			Dongle* dongle = NULL;
			int tindex = 0;
			dongle = &Dongles[ index ];
			if ( dongle )
			{
				CStringA t;
				if ( manufact )
				{
					if ( test_busy( index ))
						t = "* ";
					t += dongle->manfIdStr;
					strncpy_s( manufact, 256, t, 256 );
				}
				if ( product )
				{
					t = dongle->prodIdStr;
					strncpy_s( product, 256, t, 256 );
				}
				if ( serial )
				{
					t = dongle->sernIdStr;
					strncpy_s( serial, 256, t, 256 );
				}
				r = 0;	// Of course!
				break;
			}
		}
	case false:
		memset( manufact, 0, 256 );
		memset( product, 0, 256 );
		memset( serial, 0, 256 );

		rtlsdr work;
		int r = work.rtlsdr_open( index );
		if ( r >= 0 )
		{
			eepromdata data;
			memset( data, 0xff, sizeof( eepromdata ));
			r = work.rtlsdr_read_eeprom_raw( data );
			if ( r >= 0 )
			{
				r = work.GetEepromStrings( data
										 , EEPROM_SIZE
										 , manufact
										 , product
										 , serial
										 );
			}
		}
		work.rtlsdr_close();
		break;
	}

	return r;
}

// STATIC //
int rtlsdr::srtlsdr_get_device_usb_strings( uint32_t index
										  , CString& manufact
										  , CString& product
										  , CString& serial
										  )
{
	int r = -2;
	const rtlsdr_dongle_t *device = NULL;
	uint32_t device_count = 0;

	GetCatalog();
	switch( goodCatalog )
	{
	case true:
		{
			CMutexLock cml( dongle_mutex );
			Dongle* dongle = NULL;
			int tindex = 0;
			dongle = &Dongles[ index ];
			if ( dongle )
			{
				if ( manufact )
				{
					if ( test_busy( index ))
						manufact = "* ";
					manufact += CString( dongle->manfIdStr );
				}
				if ( product )
				{
					product = CString( dongle->prodIdStr );
				}
				if ( serial )
				{
					serial = CString( dongle->sernIdStr );
				}
				r = 0;	// Of course!
				break;
			}
		}
	case false:
		manufact.Empty();
		product.Empty();
		serial.Empty();

		rtlsdr work;
		int r = work.rtlsdr_open( index );
		if ( r >= 0 )
		{
			eepromdata data;
			memset( data, 0xff, sizeof( eepromdata ));
			r = work.rtlsdr_read_eeprom_raw( data );
			if ( r >= 0 )
			{
				r = work.GetEepromStrings( data
										 , &manufact
										 , &product
										 , &serial
										 );
			}
		}
		work.rtlsdr_close();
		break;
	}

	return r;
}

/* STATIC */
bool rtlsdr::test_busy( uint32_t index )
{
	int r;
	libusb_device_handle * ldevh = NULL;
	libusb_context * ctx = NULL;

	int err = libusb_init( &ctx );

	r = open_requested_device( ctx, index, &ldevh, false );

	libusb_close( ldevh );
	libusb_exit( ctx );
	return r < 0;
}

int rtlsdr::basic_open( uint32_t index
					  , struct libusb_device_descriptor *out_dd
					  , bool devindex
					  )
{
	if ( devh )
	{
		rtlsdr_close();
	}

	int r;
	int device_count = 0;
	libusb_device *device = NULL;
	struct libusb_device_descriptor dd;
	libusb_device_handle * ldevh = NULL;

	libusb_init( &ctx );

	dev_lost = 1;

	r = open_requested_device( ctx, index, &ldevh, devindex );

	if ( r < 0 )
		goto err;

	if ( out_dd )
	{
		libusb_device* d = libusb_get_device( ldevh );

		libusb_get_device_descriptor( d, &dd );
		memcpy( out_dd, &dd, sizeof( struct libusb_device_descriptor ));
	}

	devh = ldevh;

	r = claim_opened_device();

err:
	return r;
}

int rtlsdr::claim_opened_device( void )
{
	int r;
	if ( libusb_kernel_driver_active( devh, 0 ) == 1 )
	{

#ifdef DETACH_KERNEL_DRIVER
		if (!libusb_detach_kernel_driver( devh, 0 ))
		{
			fprintf(stderr, "Detached kernel driver\n");
		}
		else
		{
			fprintf(stderr, "Detaching kernel driver failed!");
			goto err;
		}
#else
		fprintf(stderr, "\nKernel driver is active, or device is "
						"claimed by second instance of librtlsdr."
						"\nIn the first case, please either detach"
						" or blacklist the kernel module\n"
						"(dvb_usb_rtl28xxu), or enable automatic"
						" detaching at compile time.\n\n");
		TRACE(  "\nKernel driver is active, or device is "
				"claimed by second instance of librtlsdr."
				"\nIn the first case, please either detach"
				" or blacklist the kernel module\n"
				"(dvb_usb_rtl28xxu), or enable automatic"
				" detaching at compile time.\n\n");
#endif
		return -2;
	}

	r = libusb_claim_interface( devh, 0 );
	if ( r < 0 )
	{
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		TRACE( "usb_claim_interface error %d\n", r);
		return r;
	}

	return 0;
}

int rtlsdr::rtlsdr_open( uint32_t index )
{
	return rtlsdr_open_( index, false );
}

int rtlsdr::rtlsdr_open_( uint32_t index, bool devindex /* = true*/ )
{
	if (( devh != NULL ) || ( ctx != NULL )) 
		return -10;

	int r;
	uint32_t device_count = 0;
	uint8_t  reg;
	libusb_device_descriptor dd;

	//	Perform some basic initialization
	if ( lastCatalog == 0 )
	{
		ReadRegistry();
		if ( lastCatalog == 0 )
			lastCatalog = 1;
	}

	ClearVars();

	memcpy( fir, fir_default, sizeof(fir_default));

	r = basic_open( index, &dd, devindex );

	if ( r < 0 )
		goto err;

	rtlsdr_set_bias_tee( 0 );	//	Kill bias T for safety.

	active_index = index;

	//	Now we "really" open the device and discover the tuner type.
	rtl_xtal = DEF_RTL_XTAL_FREQ;

	/* perform a dummy write, if it fails, reset the device */
	if ( dummy_write())
	{
		fprintf(stderr, "Resetting device...\n");
		TRACE( "Resetting device...\n");
		libusb_reset_device( devh );
		Sleep( 30 );	//	Give the device a wee little time to recover.
	}

	rtlsdr_init_baseband();
	dev_lost = 0;

	/* Probe tuners */
	tuner_type = RTLSDR_TUNER_UNKNOWN;
	rtlsdr_set_i2c_repeater( 1 );

	reg = rtlsdr_i2c_read_reg( E4K_I2C_ADDR, E4K_CHECK_ADDR );
	if ( reg == E4K_CHECK_VAL )
	{
		if ( !inReinitDongles )
			fprintf(stderr, "Found Elonics E4000 tuner\n");
		TRACE( "Found Elonics E4000 tuner\n");
		tuner_type = RTLSDR_TUNER_E4000;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg( FC0013_I2C_ADDR, FC0013_CHECK_ADDR );
	if ( reg == FC0013_CHECK_VAL )
	{
		if ( !inReinitDongles )
			fprintf(stderr, "Found Fitipower FC0013 tuner\n");
		TRACE( "Found Fitipower FC0013 tuner\n");
		tuner_type = RTLSDR_TUNER_FC0013;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg( R820T_I2C_ADDR, R82XX_CHECK_ADDR );
	if ( reg == R82XX_CHECK_VAL )
	{
		if ( !inReinitDongles )
			fprintf(stderr, "Found Rafael Micro R820T tuner\n");
		TRACE( "Found Rafael Micro R820T tuner\n");
		tuner_type = RTLSDR_TUNER_R820T;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg( R828D_I2C_ADDR, R82XX_CHECK_ADDR );
	if ( reg == R82XX_CHECK_VAL )
	{
		if ( !inReinitDongles )
			fprintf(stderr, "Found Rafael Micro R828D tuner\n");
		TRACE( "Found Rafael Micro R828D tuner\n");
		tuner_type = RTLSDR_TUNER_R828D;
		goto found;
	}

	/* initialise GPIOs */
	rtlsdr_set_gpio_output( 5 );

	/* reset tuner before probing */
	rtlsdr_set_gpio_bit( 5, 1 );
	rtlsdr_set_gpio_bit( 5, 0 );

	reg = rtlsdr_i2c_read_reg( FC2580_I2C_ADDR, FC2580_CHECK_ADDR );
	if (( reg & 0x7f ) == FC2580_CHECK_VAL )
	{
		if ( !inReinitDongles )
			fprintf(stderr, "Found FCI 2580 tuner\n");
		TRACE( "Found FCI 2580 tuner\n");
		tuner_type = RTLSDR_TUNER_FC2580;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg( FC0012_I2C_ADDR, FC0012_CHECK_ADDR );
	if ( reg == FC0012_CHECK_VAL )
	{
		if ( !inReinitDongles )
			fprintf(stderr, "Found Fitipower FC0012 tuner\n");
		TRACE( "Found Fitipower FC0012 tuner\n");
		rtlsdr_set_gpio_output( 6 );
		tuner_type = RTLSDR_TUNER_FC0012;
		goto found;
	}

found:
	TRACE( "tuner_type = %d.\n", tuner_type );
	/* use the rtl clock value by default */
	tun_xtal = rtl_xtal;
	tuner = tunerset[ tuner_type ];
	r = tuner->init( this );

	switch ( tuner_type )
	{
	case RTLSDR_TUNER_R828D:
		tun_xtal = R828D_XTAL_FREQ;
	case RTLSDR_TUNER_R820T:
		/* disable Zero-IF mode */
		rtlsdr_demod_write_reg( 1, 0xb1, 0x1a, 1 );

		/* only enable In-phase ADC input */
		rtlsdr_demod_write_reg( 0, 0x08, 0x4d, 1 );

		/* the R82XX use 3.57 MHz IF for the DVB-T 6 MHz mode, and
		 * 4.57 MHz for the 8 MHz mode */
		rtlsdr_set_if_freq( R82XX_DEFAULT_IF_FREQ, NULL );

		/* enable spectrum inversion */
		rtlsdr_demod_write_reg( 1, 0x15, 0x01, 1 );

		break;
	case RTLSDR_TUNER_UNKNOWN:
		fprintf(stderr, "No supported tuner found\n");
		TRACE( "No supported tuner found\n");
		rtlsdr_set_direct_sampling( 1 );
		break;
	default:	// FitiPower etc....
		break;
	}

	tuner_initialized = index;

	//	Make sure this is initialized in the dongles. There is a
	//	path that skips this if the right commands are not issued.
	tuner->set_xtal_frequency( rtl_xtal );

	rtlsdr_set_i2c_repeater( 0 );
	Dongles[ index ].busy = true;
	m_dongle = Dongles[ index ];
	WriteSingleRegistry( index );

	return 0;

err:

	rtlsdr_close();

//	deviceOpened = -1;

	return r;
}

int rtlsdr::rtlsdr_close( void )
{
	if (( devh == NULL ) && ( ctx == NULL ))
		return -1;

	if ( devh != NULL )
	{
		rtlsdr_set_i2c_repeater( 0 );

		// Turn off the bias tee
		//rtlsdr_set_gpio_output(dev, 0);
		rtlsdr_set_gpio_bit( 0, 0 );

		rtlsdr_set_i2c_repeater( 0 );

		if ( !dev_lost )
		{
			/* block until all async operations have been completed (if any) */
			while ( RTLSDR_INACTIVE != async_status)
			{
#ifdef _WIN32
				Sleep( 1 );
#else
				usleep(1000);
#endif
			}

			libusb_release_interface( devh, 0 );

			rtlsdr_deinit_baseband();
		}

		libusb_close( devh );
		devh = NULL;
		tuner_initialized = -1;

		int index;
		{
			CMutexLock cml( dongle_mutex );
			index = GetDongleIndexFromDongle( m_dongle );
		}

		if ( index >= 0 )
		{
			Dongles[ index ].busy = false;
			CMutexLock cml( registry_mutex );
			WriteSingleRegistry( index );
			m_dongle.Clear();
		}
	}

	if ( ctx != NULL )
	{
		libusb_exit( ctx );
		ctx = NULL;
	}

	if ((DWORD) active_index >= RtlSdrArea->activeEntries )
	{
		Dongles[ active_index ].busy = false;
		WriteSingleRegistry( active_index );
	}
	active_index = -1;
	return 0;
}

uint32_t rtlsdr::rtlsdr_get_tuner_clock( void )
{
	uint32_t tuner_freq;

	if ( !devh )
		return 0;

	/* read corrected clock value */
	if ( rtlsdr_get_xtal_freq( NULL, &tuner_freq ))
		return 0;

	return tuner_freq;
}

// Return true on success
//	STATIC
bool rtlsdr::reinitDongles( void )
{
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
		return false;
	}


	libusb_device** devlist;
	ssize_t dev_count = libusb_get_device_list( tctx, &devlist );
	TRACE( "Device list dev_count %d\n", dev_count );

	//	Tell rtlsdr_open variants that we're in reinitdongles
	inReinitDongles = true;

	//	Clear Dongles array found devices value to not found, -1.
	for( DWORD i = 0; i < RtlSdrArea->activeEntries; i++ )
	{
		Dongles[ i ].found = -1;
		Dongles[ i ].busy = false;
	}

#if 0	//	This way SORT OF works.
	//	And clear the rest of the dongle area to 0.
	int rtlsdr_count = RtlSdrArea->activeEntries;
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

			//	Try to open the device to determine busy status and perhaps
			//	read the name.
			{
				rtlsdr work;
				int res = work.rtlsdr_open((uint32_t) rtlsdr_count );
				if ( res >= 0 )
				{
					dongle.busy = false;

					// Figure out if we match strings somewhere - likely reg.
					eepromdata testdata = { 0 };
					res = work.rtlsdr_read_eeprom_raw( testdata );
					if ( res < 0 )
						res = res;

					//	Parse eeprom data to get the names and eventually add it.
					char* manfs = manf.GetBuffer( 256 );
					char* prods = prod.GetBuffer( 256 );
					char* serns = sern.GetBuffer( 256 );
					work.GetEepromStrings( testdata
										 , sizeof( eepromdata )
										 , manfs
										 , prods
										 , serns
										 );
					manf.ReleaseBuffer();
					prod.ReleaseBuffer();
					sern.ReleaseBuffer();

					//	We have it open - get the tunertype now?
					dongle.tunerType = work.rtlsdr_get_tuner_type();

					//	Now run comparisons against only registered dongles.
					bool matched = false;
					DWORD dng = 0;
					for( ; dng < RtlSdrArea->activeEntries; dng++ )
					{
						if ( CompareSparseRegAndData( &Dongles[ dng ]
													, testdata
													))
						{
							//	Compare USB path....
							if (( portnums[ 0 ] != 0 )
							&&	( memcmp( portnums
										, Dongles[ dng ].usbpath
										, MAX_USB_PATH
										) == 0 )
							   )
							{
								matched = true;
								rtlsdr_count++;
								Dongles[ dng ].busy = false;
								Dongles[ dng ].found = (int) i;
							}
							else
							{
								dongle.duplicated = true;
								Dongles[ dng ].duplicated = true;
								//	This is a different baby. No match even if
								//	the names data match. Continue searching.
							}
						}
					}
					work.rtlsdr_close();

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
						work.GetEepromStrings( testdata, 256, manfs, prods, serns );
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

			rtlsdr work;
			int res = work.rtlsdr_open((int) i );
			if ( res >= 0 )
			{
				dongle.busy = false;	//	For comparisons....
				eepromdata testdata = { 0 };
				res = work.rtlsdr_read_eeprom_raw( testdata );
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
				work.GetEepromStrings( testdata
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
				dongle.tunerType = work.rtlsdr_get_tuner_type();
				TRACE( "Sn dng %s\n", sern );
				work.rtlsdr_close();
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
				if (( dbindex = work.FindGuessInMasterDB( &dongle )) >= 0 )
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
			if (( dbindex = rtlsdr::FindInMasterDB( &test, true )) >= 0 )
			{
				//	Simply remove it from local db - do nothing right here.
				Dongles[ dbindex ].busy = test.busy;
				Dongles[ dbindex ].found = test.found;
			}
			else
			if (( dbindex = rtlsdr::FindInMasterDB( &test, false )) < 0 )
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

	inReinitDongles = false;

	return rval;
}


bool rtlsdr::TestMaster( void )
{
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

	RtlSdrArea->MasterPresent = false;
	Sleep( 20 );
	if ( RtlSdrArea->MasterPresent )
	{
		goodCatalog = true;
		return true;
	}
	Sleep( 20 );
	if ( RtlSdrArea->MasterPresent )
	{
		goodCatalog = true;
		return true;
	}
	return false;
}


//	STATIC	//
void rtlsdr::GetCatalog( void )
{
	// Make sure shared memory areas are loaded for this.
	if ( TestMaster())
		return;					//  We have known good data already.

	//	Else master catalog tool not running so fall through
	//	and do it the hard way.

	//	test to see if the catalog started in the last second. Otherwise
	//	use what we have.
	if ( time( NULL ) - lastCatalog < CATALOG_TIMEOUT )
		return;

	ReadRegistry();
	//	Now I must run through the interfaces and merge data.
	if ( time( NULL ) - lastCatalog > CATALOG_TIMEOUT )
	{
		reinitDongles();
		lastCatalog = time( NULL );
		WriteRegLastCatalogTime();
	}
	goodCatalog = true;
}

CStringA rtlsdr::RtlSdrVersionString;
__int64  rtlsdr::RtlSdrVersion64;

const char* rtlsdr::rtlsdr_get_version( void )
{
	return srtlsdr_get_version();
}

const char* rtlsdr::srtlsdr_get_version( void )
{
	if ( RtlSdrVersionString.IsEmpty())
	{
		srtlsdr_get_version_int64();
	}
	return RtlSdrVersionString;
}

unsigned __int64 rtlsdr::rtlsdr_get_version_int64( void )
{
	return srtlsdr_get_version_int64();
}

unsigned __int64 rtlsdr::srtlsdr_get_version_int64( void )
{
	if ( RtlSdrVersionString.IsEmpty())
	{
		RtlSdrVersion64 = 0;
		CString work;
		TCHAR *me = work.GetBuffer( _MAX_PATH );
		::GetModuleFileName( NULL, me, _MAX_PATH );
		work.ReleaseBuffer();
		if ( work.IsEmpty())
		{
			RtlSdrVersionString = "We don't exist?";
			return 0;		//	We don't really exist! (out of memory)
		}
		int loc = work.ReverseFind( _T( '\\' ));
		if ( loc < 0 )
		{
			RtlSdrVersionString = "Can't find SDRConsole";
			return 0;		//	We're confused.
		}

		work = work.Left( loc ) + _T( "\\rtlsdr.dll" );


		int size = ::GetFileVersionInfoSize( work, NULL );
		if ( size == 0 )
			return 0;

		BYTE *vinfo = new BYTE[ size ];
		if ( vinfo != NULL )
		{
			memset( vinfo, 0, sizeof( vinfo ));
			if ( ::GetFileVersionInfo( work, NULL, size, vinfo ) != 0 )
			{
				VS_FIXEDFILEINFO *ffinfo;
				UINT ffinfosize = sizeof( VS_FIXEDFILEINFO );
				BOOL ret = VerQueryValue( vinfo, _T( "\\" ), (LPVOID*)&ffinfo, &ffinfosize );

				WORD verhi = HIWORD( ffinfo->dwFileVersionMS );
				WORD verlo = LOWORD( ffinfo->dwFileVersionMS );
				WORD revhi = HIWORD( ffinfo->dwFileVersionLS );
				WORD revlo = LOWORD( ffinfo->dwFileVersionLS );

				RtlSdrVersionString.Format( "%u.%u.%u.%u", verhi, verlo, revhi, revlo );
				RtlSdrVersion64 = (((unsigned __int64)verhi ) << 48 )
								| (((unsigned __int64)verlo ) << 32 )
								| (((unsigned __int64)revhi ) << 16 )
								| (unsigned __int64)revlo;
			}
			else
			{
				TRACE( "Last error %d\n", GetLastError());
			}
			delete vinfo;
		}
	}
	return RtlSdrVersion64;
}




extern "C"
{
	SDRDAPI IRtlSdr* CreateRtlSdr()
	{
		return new rtlsdr;
	}

	SDRDAPI void DeleteRtlSdr( IRtlSdr* me)
	{
		delete (rtlsdr*) me;
	}
}



#if defined SET_SPECIAL_FILTER_VALUES
int rtlsdr::rtlsdr_set_if_values( rtlsdr_dev_t *dev
								, BYTE			regA
								, BYTE			regB
								, DWORD			ifFreq
								)
{
	int r = 0;

	if ( tuner == NULL )
		return -1;

	if (( tuner_type == RTLSDR_TUNER_R820T )
	||	( tuner_type == RTLSDR_TUNER_R828D ))
	{
		rtlsdr_set_i2c_repeater(  1 );
		r = tuner->SetFilterValuesDirect( regA, regB, ifFreq );
		rtlsdr_set_i2c_repeater( 0 );
		if ( r >= 0 )
		{
			/* This also sets required IF */
			rtlsdr_set_center_freq( freq );
		}
	}
	return r;
}
#endif