/*
 * Fitipower FC0013 tuner driver
 *
 * Copyright (C) 2012 Hans-Frieder Vogt <hfvogt@gmx.net>
 *
 * modified for use in librtlsdr
 * Copyright (C) 2012 Steve Markgraf <steve@steve-m.de>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#pragma once

#include "librtlsdr.h"

#define FC0013_I2C_ADDR		0xc6
#define FC0013_CHECK_ADDR	0x00
#define FC0013_CHECK_VAL	0xa3

class fc0013Tuner : public ITuner
{
public:
	fc0013Tuner( rtlsdr* in_dev );
	~fc0013Tuner();

	//	ITuner interface
	virtual int init					( rtlsdr* base );
	virtual int exit					( void ) _DUMMY_
	virtual int set_freq				( uint32_t freq /* Hz */
										, uint32_t *lo_freq
										);
	virtual int set_bw					( int bw /* Hz */) _DUMMY_
	virtual	int get_gain				( void );	/* tenth dB */
	virtual int set_gain				( int gain	/* tenth dB */);
	virtual int set_if_gain				( int stage
										, int gain /* tenth dB */
										) _DUMMY_
	virtual int get_tuner_stage_count	( void ) _DUMMY_
	virtual int get_tuner_stage_gains	( uint8_t stage
										, const int32_t **gains
										, const char **description
										) _DUMMY_
	virtual int get_tuner_stage_gain	( uint8_t stage ) _DUMMY_
	virtual int set_tuner_stage_gain	( uint8_t stage
										, int gain
										) _DUMMY_
	virtual int set_gain_mode			( int manual );
	virtual int get_tuner_bandwidths	( const uint32_t **bandwidths
										, int *len
										) _DUMMY_
	virtual int get_bandwidth_set_name	( int nSet
										, char* pString
										) _DUMMY_
	virtual int set_bandwidth_set		( int nSet ) _DUMMY_
	virtual int set_dither				( int dither ) _DUMMY_

	virtual int	get_xtal_frequency		( uint32_t& xtalfreq );
	virtual int	set_xtal_frequency		( uint32_t xtalfreq ) _DUMMY_
	virtual int	get_tuner_gains			( const int **ptr
										, int *len
										);

#if defined SET_SPECIAL_FILTER_VALUES
	virtual int SetFilterValuesDirect	( BYTE  regA
										, BYTE  regB
										, DWORD ifFreq
										) _DUMMY_
#endif

protected:
	int			fc0013_writereg			( uint8_t reg
										, uint8_t val
										);
	int			fc0013_readreg			( uint8_t reg
										, uint8_t *val
										);
	int			fc0013_rc_cal_add		( int rc_val );
	int			fc0013_rc_cal_reset		( void );
	int			fc0013_set_vhf_track	( uint32_t freq );


	int fc0013_init						( void );
	int fc0013_set_params				( uint32_t freq
										, uint32_t bandwidth
										);
	int fc0013_set_gain_mode			( int manual );
	int fc0013_set_lna_gain				( int gain );

protected:
	int				setgain;
	rtlsdr*			rtldev;
};
