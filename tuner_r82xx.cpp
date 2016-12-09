/*
 * Rafael Micro R820T/R828D driver
 *
 * Copyright (C) 2013 Mauro Carvalho Chehab <mchehab@redhat.com>
 * Copyright (C) 2013 Steve Markgraf <steve@steve-m.de>
 *
 * This driver is a heavily modified version of the driver found in the
 * Linux kernel:
 * http://git.linuxtv.org/linux-2.6.git/history/HEAD:/drivers/media/tuners/r820t.c
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

#include "stdafx.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

//#define VCO_CURRENT

typedef struct rtlsdr_dev rtlsdr_dev_t;

#include "tuner_r82xx.h"

/*
 * Static constants
 */

/*
 * These should be "safe" values, always. If we fail to get PLL lock in this range,
 * it's a hard error.
 */
#define PLL_SAFE_LOW 28000000		//28e6
#define PLL_SAFE_HIGH 1845000000	// 1845e6

/*
 * These are the initial, widest, PLL limits that we will try.
 *
 * Be cautious with lowering the low bound further - the PLL can claim to be locked
 * when configured to a lower frequency, but actually be running at around 26.6MHz
 * regardless of what it was configured for.
 *
 * This shows up as a tuning offset at low frequencies, and a "dead zone" about
 * 6MHz below the PLL lower bound where retuning within that region has no effect.
 */
#define PLL_INITIAL_LOW    26700000
#define PLL_INITIAL_HIGH 1885000000

/* We shrink the range edges by at least this much each time there is a soft PLL lock failure */
#define PLL_STEP_LOW   100000	//0.1e6
#define PLL_STEP_HIGH 1000000	// 1.0e6

struct r82xx_freq_range
{
	uint32_t	freq;
	uint8_t		open_d;
	uint8_t		rf_mux_ploy;
	uint8_t		tf_c;
	uint8_t		xtal_cap20p;
	uint8_t		xtal_cap10p;
	uint8_t		xtal_cap0p;
};

/* Those initial values start from REG_SHADOW_START */
static const uint8_t r82xx_init_array[ NUM_REGS ] =
{
	0x83, 0x32, 0x75,			/* 05 to 07 */
	0xc0, 0x40, 0xd6, 0x6c,			/* 08 to 0b */
	0xf5, 0x63, 0x75, 0x68,			/* 0c to 0f */
	0x6c, 0x83, 0x80, 0x00,			/* 10 to 13 */
	0x0f, 0x00, 0xc0, 0x30,			/* 14 to 17 */
	0x48, 0xcc, 0x60, 0x00,			/* 18 to 1b */
	0x54, 0xae, 0x4a, 0xc0			/* 1c to 1f */
};

/* Tuner frequency ranges */
static const struct r82xx_freq_range freq_ranges[] =
{
	{
	/* .freq = */			0,		/* Start freq, in MHz */
	/* .open_d = */			0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0xdf,	/* R27[7:0]  band2,band0 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			50,		/* Start freq, in MHz */
	/* .open_d = */			0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0xbe,	/* R27[7:0]  band4,band1  */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}, {
	/* .freq = */			55,		/* Start freq, in MHz */
	/* .open_d = */			0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x8b,	/* R27[7:0]  band7,band4 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			60,		/* Start freq, in MHz */
	/* .open_d = */			0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x7b,	/* R27[7:0]  band8,band4 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			65,		/* Start freq, in MHz */
	/* .open_d = */			0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x69,	/* R27[7:0]  band9,band6 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			70,		/* Start freq, in MHz */
	/* .open_d = */			0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x58,	/* R27[7:0]  band10,band7 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			75,		/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x44,	/* R27[7:0]  band11,band11 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			80,		/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x44,	/* R27[7:0]  band11,band11 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			90,		/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x34,	/* R27[7:0]  band12,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			100,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x34,	/* R27[7:0]  band12,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)    */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			110,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x24,	/* R27[7:0]  band13,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			120,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x24,	/* R27[7:0]  band13,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			140,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x14,	/* R27[7:0]  band14,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			180,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x13,	/* R27[7:0]  band14,band12 */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			220,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x13,	/* R27[7:0]  band14,band12 */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			250,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x11,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			280,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */			0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			310,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x41,	/* R26[7:6]=1 (bypass)  R26[1:0]=1 (middle) */
	/* .tf_c = */			0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			450,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x41,	/* R26[7:6]=1 (bypass)  R26[1:0]=1 (middle) */
	/* .tf_c = */			0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			588,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x40,	/* R26[7:6]=1 (bypass)  R26[1:0]=0 (highest) */
	/* .tf_c = */			0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
	,
	{
	/* .freq = */			650,	/* Start freq, in MHz */
	/* .open_d = */			0x00,	/* high */
	/* .rf_mux_ploy = */	0x40,	/* R26[7:6]=1 (bypass)  R26[1:0]=0 (highest) */
	/* .tf_c = */			0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */		0x00,
	}
};

static int r82xx_xtal_capacitor[][ 2 ] =
{
	{ 0x0b, XTAL_LOW_CAP_30P },
	{ 0x02, XTAL_LOW_CAP_20P },
	{ 0x01, XTAL_LOW_CAP_10P },
	{ 0x00, XTAL_LOW_CAP_0P  },
	{ 0x10, XTAL_HIGH_CAP_0P },
};

//	Original gain curve
static const int r82xx_gains[] = {   0,   9,  14,  27,  37,  77,  87, 125, 144, 157, 
								   166, 197, 207, 229, 254, 280, 297, 328, 338, 364,
								   372, 386, 402, 421, 434, 439, 445, 480, 496
								 };

#define SIZE_GAIN_TABLE	22

static const struct gain_index_table
{
	uint8_t lna_gain_index[ SIZE_GAIN_TABLE ];
	uint8_t mix_gain_index[ SIZE_GAIN_TABLE ];
	uint8_t vga_gain_index[ SIZE_GAIN_TABLE ];
} r82xx_gain_index_table[2] =
{
	{ // optimized for linearity
		{ 15,15,13,13,12,10, 9, 9, 8, 7, 7, 6, 6, 6, 6, 5, 5, 4, 3, 3, 2, 1 }, // LNA
		{ 12,10,10, 9, 8, 7, 6, 6, 5, 5, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 1, 0 }, // Mixer
		{ 13,13,12,11,11,11,11,10,10,10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 4, 4, 4 }, // VGA
	},
	{ // optimized for sensitivty
		{ 15,15,13,13,13,13,13,13,13,13,13,12,11,11,10, 9, 8, 7, 6, 5, 4, 3 }, // LNA
		{ 12,12,12,12,12,12,12,12,11,11,10,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }, // Mixer
		{ 13,12,12,11,10, 9, 8, 7, 6, 5, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, // VGA
	}
};

/* measured with a Racal 6103E GSM test set at 928 MHz with -60 dBm
 * input power, for raw results see:
 * http://steve-m.de/projects/rtl-sdr/gain_measurement/r820t/
 */

#define VGA_BASE_GAIN	-47
static const int r82xx_vga_gain_steps[]  =		// 16 values
{
	0, 26, 26, 30, 42, 35, 24, 13, 14, 32, 36, 34, 35, 37, 35, 36
};

static const int r82xx_lna_gain_steps[]  =		// 16 values
{
	0, 9, 13, 40, 38, 13, 31, 22, 26, 31, 26, 14, 19, 5, 35, 13
};

static const int r82xx_mixer_gain_steps[]  =	// 16 values
{
	0, 5, 10, 10, 19, 9, 10, 25, 17, 10, 8, 16, 13, 6, 3, -8
};

static const char LNA_desc[] = "LNA";
static const char Mixer_desc[] = "Mixer";
static const char IF_desc[] = "IF";

//    +0          +9         +22         +62
//  +100        +113        +144        +166
//  +192        +223        +249        +263
//  +282        +287        +322        +335
static int32_t LNA_stage[ ARRAY_SIZE( r82xx_lna_gain_steps )];
//    +0          +5         +15         +25
//   +44         +53         +63         +88
//  +105        +115        +123        +139
//  +152        +158        +161        +153
static int32_t Mixer_stage[ ARRAY_SIZE( r82xx_mixer_gain_steps )];
//   -47         -21          +5         +35
//   +77        +112        +136        +149
//  +163        +195        +231        +265
//  +300        +337        +372        +408
static int32_t IF_stage[ ARRAY_SIZE( r82xx_vga_gain_steps )];

#define MAX_GAIN_TABLE_SIZE \
	( sizeof( r82xx_lna_gain_steps ) / sizeof( r82xx_lna_gain_steps[ 0 ] ) +\
	  sizeof( r82xx_mixer_gain_steps ) / sizeof( r82xx_mixer_gain_steps[ 0 ] )\
	)
static int r82xx_gain_table_len;
static int r82xx_gain_table[ MAX_GAIN_TABLE_SIZE ];

const CString r82xxTuner::BandwidthSetNames[] =
{
	"Centered",
	"Leif High side"
};

const DWORD r82xxTuner::BWSNcount = 2;

#define BWSNdefault		0

const tFilterInfo	r82xxTuner::m_FilterInfo0[] =
{
//      BW          IF          0x0a    0x0b
    {	2100000,    1800000,    0xff,   0x0e, },
//  {	2000000,    1700000,    0xff,   0x8e, },
//  {	1850000,    1700000,    0xff,   0x8d, },
//  {	1750000,    1825000,    0xff,   0x8c, },
//  {	1600000,    1950000,    0xff,   0x8b, },
    {	1300000,    1675000,    0xef,   0xab, },
//  {	1300000,    2100000,    0xef,   0x8a, },
    {	1100000,    2050000,    0xef,   0x89, },
    {	 950000,    2150000,    0xef,   0x88, },
    {	 700000,    2250000,    0xef,   0x87, },
//  {	 650000,    1950000,    0xef,   0xA9, },
    {	 550000,    1700000,    0xef,   0xea, },
    {	 450000,    1750000,    0xef,   0xe9, },
    {	 250000,    1825000,    0xef,   0xe8, }
};
const tFilterInfo	r82xxTuner::m_FilterInfo1[] =
{
//      BW          IF          0x0a    0x0b
    {	2200000,    4700000,    0xef,   0x51, },
    {	1600000,    1200000,    0xef,   0xef, },
    {	1300000,    1300000,    0xef,   0xed, },
    {	1200000,    1350000,    0xef,   0xec, },
    {	1000000,    1550000,    0xef,   0xeb, },
    {	 700000,    1700000,    0xef,   0xea, },
    {	 550000,    2050000,    0xef,   0xe9, },
    {	 400000,    2100000,    0xef,   0xe8, },
    {	 300000,    2150000,    0xef,   0xe7, }
};


r82xxTuner::r82xxTuner( rtlsdr* base, enum rtlsdr_tuner type )
	: rtldev( base )
	, m_rafael_chip( type == RTLSDR_TUNER_R828D ? CHIP_R828D : CHIP_R820T )
	, m_i2c_addr( type == RTLSDR_TUNER_R828D ? R828D_I2C_ADDR : R820T_I2C_ADDR )
	, gain_mode( 0 )
	, m_CurBWSet( BWSNdefault )
	, m_FilterInfo( NULL )
	, m_bandwidths( NULL )
{
	memset( stagegains, 0, sizeof( stagegains ));
	r82xx_calculate_stage_gains();
	r82xx_compute_gain_table();

	r82xx_SetBWValues( BWSNdefault );
}


r82xxTuner::~r82xxTuner( void )
{
	if ( m_bandwidths )
		delete m_bandwidths;
}


void r82xxTuner::ClearVars( void )
{
//	i2c_addr        = false;			\ Set on class creation
//	rafael_chip     = CHIP_R820T;		/

	rtldev->rtlsdr_get_xtal_freq( NULL, &m_xtal );

	m_max_i2c_msg_len = 8;

	m_xtal_cap_sel      = XTAL_HIGH_CAP_0P;
	m_pll               = 0;
	m_nominal_int_freq  = 0;
	m_asked_for_freq    = 0;
	m_fil_cal_code	    = 0;
	m_input             = 0;
	m_init_done         = false;
	m_disable_dither    = false;
	m_reg_cache		    = 0;
	m_reg_batch         = 0;
	m_reg_low           = 0;
	m_reg_high          = 0;


	m_bw                = 0;
	m_if_filter_freq    = 0;
	m_pll_off           = 0;
	m_pll_low_limit     = 0;
	m_pll_high_limit    = 0;

	m_CurBWSet			= BWSNdefault;
	r82xx_SetBWValues( m_CurBWSet );
}

// ITuner interfaces
int r82xxTuner::init( rtlsdr* base )
{
	ClearVars();
	rtldev = base;

	return r82xx_init();
}

int r82xxTuner::exit( void )
{
	return r82xx_standby();
}

int r82xxTuner::set_freq( uint32_t freq /* Hz */, uint32_t *lo_freq_out )
{
	return r82xx_set_freq( freq, lo_freq_out );
}

int r82xxTuner::set_bw( int bw /* Hz */)
{
	return r82xx_set_bw( bw );
}

int r82xxTuner::get_gain( void )
{
	return stagegains[ 0 ]
		 + stagegains[ 1 ]
		 + stagegains[ 2 ]
		 - 163;					// Make display match what it used to

}

int r82xxTuner::set_gain( int gain /* tenth dB */)
{
	return r82xx_set_gain( gain );
}

int r82xxTuner::set_if_gain( int stage
						   , int gain /* tenth dB */
						   )
{
	return 0;
}

int r82xxTuner::get_tuner_stage_gains( uint8_t stage
									 , const int32_t **gains
									 , const char **description
									 )
{
	switch( stage )
	{
	case 0:
		*gains = LNA_stage;
		*description = LNA_desc;
		return ARRAY_SIZE( LNA_stage );
	case 1:
		*gains = Mixer_stage;
		*description = Mixer_desc;
		return ARRAY_SIZE( Mixer_stage );
	case 2:
		*gains = IF_stage;
		*description = IF_desc;
		return ARRAY_SIZE( IF_stage );
	}
	return 0;
}

int r82xxTuner::get_tuner_stage_gain( uint8_t stage )
{
	if ( stage < 3 )
		return stagegains[ stage ];
	return 0;
}

int r82xxTuner::set_tuner_stage_gain( uint8_t stage, int32_t gain )
{
	int rc = -10;
	if ( stage == 0 )
		rc = r82xx_set_lna_gain( gain );
	else
	if ( stage == 1 )
		rc = r82xx_set_mixer_gain( gain );
	else
		rc = r82xx_set_VGA_gain( gain );
	if ( rc < 0 )
		return rc;

	stagegains[ stage ] = gain;
	return 0;
}

int r82xxTuner::set_gain_mode( int manual )
{
	return r82xx_enable_manual_gain( manual );
}

int r82xxTuner::get_tuner_bandwidths( const uint32_t **bandwidths, int *len )
{
	return r82xx_get_tuner_bandwidths( bandwidths, len );
}

int r82xxTuner::get_bandwidth_set_name( int nSet, char* pString )
{
	return r82xx_get_bandwidth_set_name( nSet, pString );
}

int r82xxTuner::set_bandwidth_set( int nSet )
{
	return r82xx_set_bandwidth_set( nSet );
}

int r82xxTuner::set_dither( int dither )
{
	return r82xx_set_dither( dither );
}


int	r82xxTuner::get_xtal_frequency( uint32_t& xtalfreq )
{
	xtalfreq = m_xtal;
	return 0;
}

int	r82xxTuner::set_xtal_frequency( uint32_t xtalfreq )
{
	m_xtal = xtalfreq;
	return 0;
}

int	r82xxTuner::get_tuner_gains( const int **out_ptr
							   , int *out_len
							   )
{
	if ( !out_len )
		return -1;

	*out_len = r82xx_gain_table_len * sizeof( int );
	if ( out_ptr )
		*out_ptr = r82xx_gain_table;
	return 0;
}

/*
 * I2C read/write code and shadow registers logic
 */
void r82xxTuner::shadow_store( uint8_t reg, const uint8_t *val, int len )
{
	int r = reg - REG_SHADOW_START;

	if ( r < 0 )
	{
		len += r;
		r = 0;
	}
	if ( len <= 0 )
		return;
	if ( len > NUM_REGS - r )
		len = NUM_REGS - r;

	memcpy( &m_regs[ r ], val, len );
}

int r82xxTuner::r82xx_write( uint8_t reg , const uint8_t *val , unsigned int len )
{
	int rc;
	int size;
	int pos = 0;

	/* Store the shadow registers */
	shadow_store( reg, val, len );

	do
	{
		if ( len > m_max_i2c_msg_len - 1)
			size = m_max_i2c_msg_len - 1;
		else
			size = len;

		/* Fill I2C buffer */
		m_buf[ 0 ] = reg;
		memcpy( &m_buf[ 1 ], &val[ pos ], size );

		rc = rtldev->rtlsdr_i2c_write_fn( m_i2c_addr, m_buf, size + 1 );

		if ( rc != size + 1 )
		{
			fprintf(stderr, "%s: i2c wr failed=%d reg=%02x len=%d\n",
				   __FUNCTION__, rc, reg, size);
			TRACE( "%s: i2c wr failed=%d reg=%02x len=%d\n",
				   __FUNCTION__, rc, reg, size );
			if ( rc < 0 )
				return rc;
			return -1;
		}

		reg += size;
		len -= size;
		pos += size;
	} while ( len > 0 );

	return 0;
}

int r82xxTuner::r82xx_read_cache_reg( int reg )
{
	reg -= REG_SHADOW_START;

	if ( reg >= 0 && reg < NUM_REGS )
		return m_regs[ reg ];
	else
		return -1;
}

int r82xxTuner::r82xx_write_reg( uint8_t reg, uint8_t val )
{
	if ( m_reg_cache && ( r82xx_read_cache_reg( reg ) == val ))
		return 0;
	if ( m_reg_batch )
	{
		shadow_store( reg, &val, 1 );
		if ( reg < m_reg_low )
			m_reg_low = reg;
		if ( reg > m_reg_high )
			m_reg_high = reg;
		return 0;
	}
	return r82xx_write( reg, &val, 1 );
}

int r82xxTuner::r82xx_write_reg_mask( uint8_t reg, uint8_t val, uint8_t bit_mask )
{
	int rc = r82xx_read_cache_reg( reg );
	if ( rc < 0 )
		return rc;

	val = ( rc & ~bit_mask ) | ( val & bit_mask );

	return r82xx_write_reg( reg, val );
}

int r82xxTuner::r82xx_write_batch_init( void )
{
	m_reg_batch = 0;
	if ( m_reg_cache )
	{
		m_reg_batch = 1;
		m_reg_low   = NUM_REGS;
		m_reg_high  = 0;
	}
	return 0;
}

int r82xxTuner::r82xx_write_batch_sync( void )
{
	int rc, offset, len;
	if ( m_reg_cache == 0 )
		return -1;
	if ( m_reg_batch == 0 )
		return -1;
	m_reg_batch = 0;
	if ( m_reg_low > m_reg_high )
		return 0; /* No work to do */
	offset = m_reg_low - REG_SHADOW_START;
	len = m_reg_high - m_reg_low + 1;
	rc = r82xx_write( m_reg_low, m_regs + offset, len );
	return rc;
}

uint8_t r82xxTuner::r82xx_bitrev( uint8_t byte )
{
	const uint8_t lut[16] = { 0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
							  0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf };

	return ( lut[ byte & 0xf ] << 4 ) | lut[ byte >> 4 ];
}

int r82xxTuner::r82xx_read( uint8_t reg, uint8_t *val, int len )
{
	int rc, i;
	uint8_t *p = &m_buf[1];

	m_buf[ 0 ] = reg;

	rc = rtldev->rtlsdr_i2c_write_fn( m_i2c_addr, m_buf, 1 );
	if ( rc < 1 )
		return rc;

	rc = rtldev->rtlsdr_i2c_read_fn( m_i2c_addr, p, len );

	if ( rc != len )
	{
		fprintf(stderr, "%s: i2c rd failed=%d reg=%02x len=%d\n",
						 __FUNCTION__, rc, reg, len);
		if ( rc < 0 )
			return rc;
		return -1;
	}

	/* Copy data to the output buffer */
	for ( i = 0; i < len; i++ )
		val[ i ] = r82xx_bitrev( p[ i ]);

	return 0;
}

/*
 * r82xx tuning logic
 */

int r82xxTuner::r82xx_set_mux( uint32_t freq )
{
	const struct r82xx_freq_range *range;
	int rc;
	unsigned int i;
	uint8_t val;

	/* Get the proper frequency range */
	freq = freq / 1000000;
	for ( i = 0; i < ARRAY_SIZE( freq_ranges ) - 1; i++ )
	{
		if ( freq < freq_ranges[ i + 1 ].freq)
			break;
	}
	range = &freq_ranges[ i ];

	/* Open Drain */
	rc = r82xx_write_reg_mask( 0x17, range->open_d, 0x08 );
	if ( rc < 0 )
		return rc;

	/* RF_MUX,Polymux */
	rc = r82xx_write_reg_mask( 0x1a, range->rf_mux_ploy, 0xc3 );
	if ( rc < 0 )
		return rc;

	/* TF BAND */
	rc = r82xx_write_reg( 0x1b, range->tf_c );
	if ( rc < 0 )
		return rc;

	/* XTAL CAP & Drive */
	switch ( m_xtal_cap_sel )
	{
	case XTAL_LOW_CAP_30P:
	case XTAL_LOW_CAP_20P:
		val = range->xtal_cap20p | 0x08;
		break;
	case XTAL_LOW_CAP_10P:
		val = range->xtal_cap10p | 0x08;
		break;
	case XTAL_HIGH_CAP_0P:
		val = range->xtal_cap0p | 0x00;
		break;
	default:
	case XTAL_LOW_CAP_0P:
		val = range->xtal_cap0p | 0x08;
		break;
	}
	rc = r82xx_write_reg_mask( 0x10, val, 0x0b );
	if ( rc < 0 )
		return rc;

	rc = r82xx_write_reg_mask( 0x08, 0x00, 0x3f );
	if ( rc < 0 )
		return rc;

	rc = r82xx_write_reg_mask( 0x09, 0x00, 0x3f );

	return rc;
}

int r82xxTuner::r82xx_set_pll( uint32_t freq, uint32_t *freq_out )
{
	int rc;
	uint64_t vco_freq;
	uint64_t vco_div;
	uint32_t vco_min = 1750000; /* kHz */
	uint32_t freq_khz;
	uint32_t pll_ref;
	uint32_t sdm = 0;
	uint8_t mix_div = 2;
	uint8_t div_num = 0;
#if defined( VCO_CURRENT) // Variable VCO current
	uint8_t vco_power_ref = 2;
#endif
	uint8_t refdiv2 = 0;
	uint8_t ni;
	uint8_t si;
	uint8_t nint;
	uint8_t val;
#if defined( VCO_CURRENT) // Variable VCO current
	uint8_t data[ 5 ];
#endif

	r82xx_write_batch_init();

	/* Frequency in kHz */
	freq_khz    = ( freq + 500 ) / 1000;
	pll_ref     = m_xtal;

	rc = r82xx_write_reg_mask( 0x10, refdiv2, 0x10 );
	if ( rc < 0 )
		return rc;

	/* set pll autotune = 128kHz */
	rc = r82xx_write_reg_mask( 0x1a, 0x00, 0x0c );
	if ( rc < 0 )
		return rc;

	/* set VCO current = 100 */
	m_pll_off = 0;
	rc = r82xx_write_reg_mask( 0x12, 0x80, 0xe0 );
	if ( rc < 0 )
		return rc;

	/* Calculate divider */

	for ( mix_div = 2, div_num = 0; mix_div < 64; mix_div <<= 1, div_num++ )
	{
		if (( freq_khz * mix_div ) >= vco_min )
			break;
	}

	//	The mix_div code above and values for vco_max and vco_min imply that
	//	without overrange the VCO frequency runs from 1750 to 3500 MHz. These
	//	numbers put limits on other numbers below. The full range is available
	//	regardless of mix_div. Thus the tune frequencies can go as high as 1750 +
	//	f_if or about 1756 with Oliver's code. The low end is about 21.3 Mhz

#if 0
	if ( m_rafael_chip == CHIP_R828D )
		vco_power_ref = 1;
	/*	mutability////
	rc = r82xx_read(priv, 0x00, data, sizeof(data));
	if (rc < 0)
		return rc;
	vco_fine_tune = (data[4] & 0x30) >> 4;
	*/
	uint8_t vco_fine_tune = 2;

//	if ( CHIP_R828D )
//		vco_power_ref = 1;
//	else
//		vco_power_ref = 2;
	//	828  2  1
	//	oth  2  2
	if ( vco_fine_tune > vco_power_ref )	// vco_power_ref = 1 or 2.
		div_num = div_num - 1;
	else
	if ( vco_fine_tune < vco_power_ref )
		div_num = div_num + 1;
#else
	//	This is the short version of the above.
	if ( m_rafael_chip == CHIP_R828D )
		div_num = div_num - 1;
#endif


	rc = r82xx_write_reg_mask( 0x10, div_num << 5, 0xe0 );
	if ( rc < 0 )
		return rc;

	//	Remember that vco_freq is more or less constrained to the range of
	//	1750 to 3500 MHz above (and below actually).
	vco_freq = (uint64_t) freq * (uint64_t) mix_div;

	/*
	 * We want to approximate:
	 *  vco_freq / (2 * pll_ref)
	 * in the form:
	 *  nint + sdm / 65536
	 * where nint,sdm are integers and 0 < nint, 0 <= sdm < 65536
	 * Scaling to fixed point and rounding:
	 *  vco_div = 65536*(nint + sdm/65536) = int( 0.5 + 65536 * vco_freq / (2 * pll_ref) )
	 *  vco_div = 65536*nint + sdm         = int( (pll_ref + 65536 * vco_freq) / (2 * pll_ref) )
	 */

	//	So 1991111 <= vco_div <= 3982222 in integers.
	//	Note this gives a step size of its own if you reverse the calculation.
	//	It's very close to 879 Hz. That's the VCO step size, though.
	//	Next we divide by 65536. Call this vco_divd.
	//	30.3819427490234375 <= vco_divd <= 60.763885498046875
	//	Of course this is a ridiculous number of decimal points. But it is useful
	//	for looking at the numbers below and some of the tests. I do not know where
	//	these come from. It appears to be ridiculous.
	//
	//	One thing comes out of this discussion. The sdm figure can theoretically
	//	go through its whole range 0..65535 for each step. It is effectively a
	//	divisor that makes the comparison frequency for the PFD equal to about
	//	879 Hz rather than the apparent "real" comparison frequency of 57.6 MHz.
	//	Of course, lock time would be way to long to make this resemble the real
	//	circuit.
        
	vco_div = ( pll_ref + 65536 * vco_freq ) / ( 2 * pll_ref );
	nint = (uint32_t) ( vco_div / 65536 );
	sdm = (uint32_t) ( vco_div % 65536 );

	// nb. it appears nint MUST be between 30 and 60. The test below seems silly.
	if (( nint < 13 )
	||  ( m_rafael_chip == CHIP_R828D && nint > 127 )
	||  ( m_rafael_chip != CHIP_R828D && nint > 76 ))
	{
		fprintf(stderr, "[R82XX] No valid PLL values for %u Hz!\n", freq);
		return -1;
	}

	ni = ( nint - 13 ) / 4;
	si = nint - 4 * ni - 13;

	if ( freq_out )
	{
		uint64_t actual_vco = (uint64_t) 2 * pll_ref * nint
							+ (uint64_t) 2 * pll_ref * sdm / 65536;
		*freq_out = (uint32_t) (( actual_vco + mix_div / 2 ) / mix_div );
	}

	rc = r82xx_write_reg( 0x14, ni + ( si << 6 ));
	if ( rc < 0 )
		return rc;

#if defined( VCO_CURRENT)	// Variable VCO current
	/* pw_sdm */
	if ( sdm == 0 )
		val = 0x08;
	else
		val = 0x00;

	if ( m_disable_dither )
		val |= 0x10;

	rc = r82xx_write_reg_mask( 0x12, val, 0x18 );
	if ( rc < 0 )
		return rc;
#else
	val = 0;
	if ( m_disable_dither )
		val |= 0x10;

	rc = r82xx_write_reg_mask( 0x12, val, 0x10 );
	if ( rc < 0 )
		return rc;
#endif

	rc = r82xx_write_reg( 0x16, sdm >> 8 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x15, sdm & 0xff );
	if ( rc < 0 )
		return rc;

	if ( m_reg_batch )
	{
		rc = r82xx_write_batch_sync();
		if ( rc < 0 )
		{
			fprintf(stderr, "[R82XX] Batch error in PLL for %u Hz!\n", freq);
			TRACE( "[R82XX] Batch error in PLL for %u Hz!\n", freq);
			return rc;
		}
	}

#if defined( VCO_CURRENT)	// Variable VCO current
	for ( int i = 0; i < 2; i++ )
	{
//		usleep_range(sleep_time, sleep_time + 1000);

		/* Check if PLL has locked */
		data[ 2 ] = 0;
		rc = r82xx_read( 0x00, data, 3 );
		if ( rc < 0 )
			return rc;
		if ( data[ 2 ] & 0x40 )
			break;

		if ( i > 0 )
			break;

		/* Didn't lock. Increase VCO current */
		rc = r82xx_write_reg_mask( 0x12, 0x60, 0xe0 );
		if ( rc < 0 )
			return rc;
	}

	if (( data[ 2 ] & 0x40) == 0 )
	{
		fprintf(stderr, "[R82XX] PLL not locked!\n");
		TRACE( "[R82XX] PLL not locked!\n");
		return -42;
	}
#endif

	/* set pll autotune = 8kHz */
	rc = r82xx_write_reg_mask( 0x1a, 0x08, 0x08);

	return rc;
}

int r82xxTuner::r82xx_sysfreq_sel( void )
{
	int rc;
	uint8_t mixer_top;
	uint8_t lna_top;
	uint8_t cp_cur;
	uint8_t div_buf_cur;
	uint8_t lna_vth_l;
	uint8_t mixer_vth_l;
	uint8_t air_cable1_in;
	uint8_t cable2_in;
	uint8_t pre_dect;
	uint8_t lna_discharge;
	uint8_t filter_cur;

	mixer_top     = 0x24;			/* mixer top:13 , top-1, low-discharge */
	cp_cur        = 0x38;			/* 111, auto */
	div_buf_cur   = 0x30;			/* 11, 150u */
	lna_top       = 0xe5;			/* detect bw 3, lna top:4, predet top:2 */
	lna_vth_l     = 0x53;			/* lna vth 0.84	,  vtl 0.64 */
	mixer_vth_l   = 0x75;			/* mixer vth 1.04, vtl 0.84 */
	air_cable1_in = 0x00;			/* freq > MHZ( 345 ) */
	cable2_in     = 0x00;
	pre_dect      = 0x40;
	lna_discharge = 0x0e;
	filter_cur    = 0x40;			/* 10, low */

	rc = r82xx_write_reg_mask( 0x1d, lna_top, 0xc7 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg_mask( 0x1c, mixer_top, 0xf8 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x0d, lna_vth_l );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x0e, mixer_vth_l );
	if ( rc < 0 )
		return rc;

	m_input = air_cable1_in;

	/* Air-IN only for Astrometa */
	rc = r82xx_write_reg_mask( 0x05, air_cable1_in, 0x60 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg_mask( 0x06, cable2_in, 0x08 );
	if ( rc < 0 )
		return rc;

	rc = r82xx_write_reg_mask( 0x11, cp_cur, 0x38 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg_mask( 0x17, div_buf_cur, 0x30 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg_mask( 0x0a, filter_cur, 0x60 );
	if ( rc < 0 )
		return rc;

	/*
	 * Set LNA
	 */

	/* LNA TOP: lowest */
	rc = r82xx_write_reg_mask( 0x1d, 0, 0x38 );
	if ( rc < 0 )
		return rc;

	/* 0: normal mode */
	rc = r82xx_write_reg_mask( 0x1c, 0, 0x04);
	if ( rc < 0 )
		return rc;

	/* 0: PRE_DECT off */
	rc = r82xx_write_reg_mask( 0x06, 0, 0x40 );
	if ( rc < 0 )
		return rc;

	/* agc clk 250hz */
	rc = r82xx_write_reg_mask( 0x1a, 0x30, 0x30 );
	if ( rc < 0 )
		return rc;

	/* write LNA TOP = 3 */
	rc = r82xx_write_reg_mask( 0x1d, 0x18, 0x38);
	if ( rc < 0 )
		return rc;

	/*
		* write discharge mode
		* FIXME: IMHO, the mask here is wrong, but it matches
		* what's there at the original driver
		*/
	rc = r82xx_write_reg_mask( 0x1c, mixer_top, 0x04 );
	if ( rc < 0 )
		return rc;

	/* LNA discharge current */
	rc = r82xx_write_reg_mask( 0x1e, lna_discharge, 0x1f );
	if ( rc < 0 )
		return rc;

	/* agc clk 60hz */
	rc = r82xx_write_reg_mask( 0x1a, 0x20, 0x30 );
	if ( rc < 0 )
		return rc;
	 return 0;
}

int r82xxTuner::r82xx_init_tv_standard( void )
{
	/* everything that was previously done in r82xx_init_tv_standard
	 * and doesn't need to be changed when filter settings change */
	int rc;
	uint32_t if_khz;
	uint32_t filt_cal_lo;
	uint8_t filt_gain;
	uint8_t img_r;
	uint8_t filt_q;
	uint8_t hp_cor;
	uint8_t ext_enable;
	uint8_t loop_through;
	uint8_t lt_att;
	uint8_t flt_ext_widest;
	uint8_t polyfil_cur;

	if_khz         = R82XX_DEFAULT_IF_FREQ / 1000;
	filt_cal_lo    = 56000;		/* 52000->56000 */
	filt_gain      = 0x10;		/* +3db, 6mhz on */
	img_r          = 0x00;		/* image negative */
	filt_q         = 0x10;
	hp_cor         = 0x6b;		/* high pass corner frequency */
	ext_enable     = 0x60;		/* r30[6]=1 ext enable; r30[5]:1 ext at lna max-1 */
	loop_through   = 0x00;		/* r5[7], lt on */
	lt_att         = 0x00;		/* r31[7], lt att enable */
	flt_ext_widest = 0x00;		/* r15[7]: flt_ext_wide off */
	polyfil_cur    = 0x60;		/* r25[6:5]:min */

	/* Initialize the shadow registers */
	memcpy( m_regs, r82xx_init_array, sizeof( r82xx_init_array ));

	/* Init Flag & Xtal_check Result (inits VGA gain, needed?)*/
	rc = r82xx_write_reg_mask( 0x0c, 0x00, 0x0f );
	if ( rc < 0 )
		return rc;

	/* version */
	rc = r82xx_write_reg_mask( 0x13, VER_NUM, 0x3f );
	if ( rc < 0 )
		return rc;

	m_nominal_int_freq = if_khz * 1000;

	rc = r82xxTuner::r82xx_calibrate( filt_cal_lo, hp_cor );	// Should this be 60000?

	rc = r82xx_write_reg_mask( 0x0a, filt_q | m_fil_cal_code, 0x1f);
	if (rc < 0)
		return rc;

	/* Set BW, Filter_gain, & HP corner */
	rc = r82xx_write_reg_mask( 0x0b, hp_cor, 0xef );
	if (rc < 0)
		return rc;

	/* Set Img_R */
	rc = r82xx_write_reg_mask( 0x07, img_r, 0x80 );
	if ( rc < 0 )
		return rc;

	/* Set filt_3dB, V6MHz */
	rc = r82xx_write_reg_mask( 0x06, filt_gain, 0x30 );
	if ( rc < 0 )
		return rc;

	/* channel filter extension */
	rc = r82xx_write_reg_mask( 0x1e, ext_enable, 0x60 );
	if ( rc < 0 )
		return rc;

	/* Loop through */
	rc = r82xx_write_reg_mask( 0x05, loop_through, 0x80 );
	if ( rc < 0 )
		return rc;

	/* Loop through attenuation */
	rc = r82xx_write_reg_mask( 0x1f, lt_att, 0x80 );
	if ( rc < 0 )
		return rc;

	/* filter extension widest */
	rc = r82xx_write_reg_mask( 0x0f, flt_ext_widest, 0x80 );
	if ( rc < 0 )
		return rc;

	/* RF poly filter current */
	rc = r82xx_write_reg_mask( 0x19, polyfil_cur, 0x60 );
	if ( rc < 0 )
		return rc;

	return 0;
}
int r82xxTuner::update_if_filter( uint32_t bw )
{
	uint8_t val;
	int rc;
	int i;
	/* find a filter setting that is close to the required bandwidth */
	if ( bw > 100 )
	{
		m_bw = bw;
		TRACE( "Find closest bw to %d\n", m_bw );
		int size = m_FilterSetCount;
		for( i = 0; i < size - 2; i++ )
		{
			/* bandwidth is compared to median of the current and next available bandwidth in the table */
			if ( bw > ( m_FilterInfo[ i ].Bandwidth
					  + m_FilterInfo[ i + 1 ].Bandwidth
					  ) / 2 )
				break;
		}
		if ( i >= size )
			i = size - 1;
	}
	else
		i = bw;

	m_bw = m_FilterInfo[ i ].Bandwidth;	/* Set the actual IF BW we get. */
	TRACE( "Found bw = %d entry %d\n", m_bw, i );

	val = m_FilterInfo[ i ].Reg0A;
	rc = r82xx_write( 0x0a, &val, 1);
	if ( rc < 0 )
		return rc;

	val = m_FilterInfo[ i ].Reg0B;
	rc = r82xx_write( 0x0b, &val, 1 );
	if ( rc < 0 )
		return rc;

	/* Save the nominal IF frequency that matches this bandwidth. */
	m_nominal_int_freq = m_FilterInfo[ i ].IfFreq;

	TRACE( "With an intermediate frequency of %d\n", m_nominal_int_freq );
	return m_nominal_int_freq;
}


int r82xxTuner::r82xx_set_bw( uint32_t bw )
{
	return update_if_filter( bw );
}

int r82xxTuner::r82xx_read_gain( void )
{
	uint8_t data[4];
	int rc;

	rc = r82xx_read( 0x00, data, sizeof( data ));
	if ( rc < 0 )
		return rc;

	//TODO: looks wrong... <<1 >>3?
	return (( data[ 3 ] & 0x0f ) << 1 ) + (( data[ 3 ] & 0xf0 ) >> 4 );
}

int r82xxTuner::r82xx_set_gain( int gain )
{
	int rc;

	if ( gain_mode )	/* Gain mode 0 == AGC. All others manual */
	{
		int i;
		int total_gain = 0;
		uint8_t mix_index = 0;
		uint8_t lna_index = 0;
		uint8_t vga_index = 0;
		uint8_t data[ 4 ];

		rc = r82xx_read( 0x00, data, sizeof( data ));
		if ( rc < 0 )
			return rc;

		switch ( gain_mode )
		{
		case GAIN_MODE_MANUAL: /* original algorithm */
			/* set fixed VGA gain for now (16.3 dB) */
			vga_index = 0x08;

			//	TODOTODO: Can I do this better?
			for ( i = 0; i < 15; i++ )
			{
				if ( total_gain >= gain )
					break;

				total_gain += r82xx_lna_gain_steps[ ++lna_index ];;

				if ( total_gain >= gain )
					break;
				total_gain += r82xx_mixer_gain_steps[ ++mix_index ];;
			}
			break;
		case GAIN_MODE_LINEARITY:
		case GAIN_MODE_SENSITIVITY:
			{
				const struct gain_index_table *t;
				t = &r82xx_gain_index_table[ gain_mode == GAIN_MODE_LINEARITY ? 0 : 1 ];

				for ( i = r82xx_gain_table_len - 1; i >= 0; i-- )
					if ( gain >= r82xx_gain_table[ i ])
						break;

				lna_index = t->lna_gain_index[ SIZE_GAIN_TABLE - i - 1 ];
				mix_index = t->mix_gain_index[ SIZE_GAIN_TABLE - i - 1 ];
				vga_index = t->vga_gain_index[ SIZE_GAIN_TABLE - i - 1 ];

				break;
			}
		}
		/* set VGA gain */
		rc = r82xx_write_reg_mask( 0x0c, vga_index, 0x9f );
		if (rc < 0)
			return rc;

		/* set LNA gain */
		rc = r82xx_write_reg_mask( 0x05, lna_index, 0x0f );
		if (rc < 0)
			return rc;

		/* set Mixer gain */
		rc = r82xx_write_reg_mask( 0x07, mix_index, 0x0f );
		if (rc < 0)
			return rc;

		stagegains[ 0 ] = LNA_stage[ lna_index ];
		stagegains[ 1 ] = Mixer_stage[ mix_index ];
		stagegains[ 2 ] = IF_stage[ vga_index ];
	}

	return 0;
}

int r82xxTuner::r82xx_enable_manual_gain( uint8_t in_gain_mode )
{
	int rc;
	uint8_t data[4];

	if ( in_gain_mode >= MAX_TUNER_GAIN_MODES )
		in_gain_mode = GAIN_MODE_MANUAL;

	rc = r82xx_read( 0x00, data, sizeof( data ));
	if ( rc < 0 )
		return rc;

	if ( in_gain_mode )
	{
		/* LNA auto off */
		rc = r82xx_write_reg_mask( 0x05, 0x10, 0x10 );
		if ( rc < 0 )
			return rc;

			/* Mixer auto off */
		rc = r82xx_write_reg_mask( 0x07, 0x00, 0x10 );
		if ( rc < 0 )
			return rc;

	}
	else
	{
		/* LNA */
		rc = r82xx_write_reg_mask( 0x05, 0, 0x10 );
		if ( rc < 0 )
			return rc;

		/* Mixer */
		rc = r82xx_write_reg_mask( 0x07, 0x10, 0x10 );
		if ( rc < 0 )
			return rc;

		/* set fixed VGA gain for now (26.5 dB) */
		rc = r82xx_write_reg_mask( 0x0c, 0x0b, 0x9f );
		if ( rc < 0 )
			return rc;
	}
	rc = r82xx_read( 0x00, data, sizeof( data) );
	if ( rc < 0 )
		return rc;

	gain_mode = in_gain_mode;
	r82xx_compute_gain_table();

	if ( gain_mode == GAIN_MODE_MANUAL )
		return 0; /* compatibility to old mode API */
	return gain_mode;
}

void r82xxTuner::r82xx_calculate_stage_gains(void)
{
		int i;
		LNA_stage[ 0 ] = r82xx_lna_gain_steps[ 0 ];
		for ( i = 1; i < ARRAY_SIZE( r82xx_lna_gain_steps ); i++ )
			LNA_stage[ i ] = LNA_stage[ i - 1 ] + r82xx_lna_gain_steps[ i ];

		Mixer_stage[ 0 ] = r82xx_mixer_gain_steps[ 0 ];
		for ( i = 1; i < ARRAY_SIZE( r82xx_mixer_gain_steps ); i++ )
			Mixer_stage[ i ] = Mixer_stage[ i - 1 ] + r82xx_mixer_gain_steps[ i ];

		IF_stage[ 0 ] = VGA_BASE_GAIN;
		for ( i = 1; i < ARRAY_SIZE( r82xx_vga_gain_steps ); i++ )
			IF_stage[ i ] = IF_stage[ i - 1 ] + r82xx_vga_gain_steps[ i ];
}

void r82xxTuner::r82xx_compute_gain_table( void )
{
	switch ( gain_mode )
	{
		case GAIN_MODE_AGC: 
		case GAIN_MODE_MANUAL: 
		{
			int lna_index = 0;
			int mixer_index = 0;
			int len = 0;
			//	This is correct even if it looks very odd.
			//	In the finished table of 28 entries you get
			//	lna gain increasing first then mixer gain.
			//	vga gain is constant at 7.7 dB.
			for ( int i = 0; i < 15; i++ )
			{
				r82xx_gain_table[ len++ ] = LNA_stage[ lna_index++ ] + Mixer_stage[ mixer_index ];
				r82xx_gain_table[ len++ ] = LNA_stage[ lna_index ] + Mixer_stage[ mixer_index++ ];
			}
			r82xx_gain_table_len = len;
			break;
		}
		case GAIN_MODE_LINEARITY:
		case GAIN_MODE_SENSITIVITY: 
		{
			const struct gain_index_table *t;
			t = &r82xx_gain_index_table[ gain_mode == GAIN_MODE_LINEARITY ? 0 : 1 ];
			for ( int i = 0; i < SIZE_GAIN_TABLE; i++ )
			{
				r82xx_gain_table[ i ] = -163 // normalize to same VGA gain as GAIN_MODE_MANUAL
						+ LNA_stage[ t->lna_gain_index[ SIZE_GAIN_TABLE - i - 1 ]]
						+ Mixer_stage[ t->mix_gain_index[ SIZE_GAIN_TABLE - i - 1 ]]
						+ IF_stage[ t->vga_gain_index[ SIZE_GAIN_TABLE - i - 1 ]];
			}
			r82xx_gain_table_len = SIZE_GAIN_TABLE;
			break;
		}
	}
}

int r82xxTuner::r82xx_set_lna_gain( int32_t gain )
{
	uint32_t lna_index;
	for ( lna_index = 0; lna_index < ARRAY_SIZE( LNA_stage ); ++lna_index )
	{
		if ( LNA_stage[ lna_index ] == gain )
		{
			int rc;
			uint8_t data[ 4 ];
			rc = r82xx_read( 0x00, data, sizeof( data ));
			if ( rc < 0 )
				return rc;
			/* set LNA gain */
			rc = r82xx_write_reg_mask( 0x05, lna_index, 0x0f );
			if ( rc < 0 )
				return rc;

			stagegains[ 0 ] = gain;
			return 0;
		}
	}
	return -EINVAL;
}

int r82xxTuner::r82xx_set_mixer_gain( int32_t gain )
{
	uint32_t mixer_index;
	for ( mixer_index = 0; mixer_index < ARRAY_SIZE( Mixer_stage ); ++mixer_index )
	{
		if ( Mixer_stage[ mixer_index ] == gain )
		{
			uint8_t data[ 4 ];
			int rc;
			rc = r82xx_read( 0x00, data, sizeof( data ));
			if ( rc < 0 )
				return rc;

			/* set Mixer gain */
			rc = r82xx_write_reg_mask( 0x07, mixer_index, 0x0f );
			if ( rc < 0 )
				return rc;

			stagegains[ 1 ] = gain;

			return 0;
		}
	}
	return -EINVAL;
}

int r82xxTuner::r82xx_set_VGA_gain( int32_t gain )
{
	uint32_t IF_index;
	for( IF_index = 0; IF_index < ARRAY_SIZE( IF_stage ); IF_index++ )
	{
		if ( IF_stage[ IF_index ] == gain )
		{
			uint8_t data[ 4 ];
			int rc;
			rc = r82xx_read( 0x00, data, sizeof( data ));
			if ( rc < 0 )
				return rc;
			/* set VGA gain */
			rc = r82xx_write_reg_mask( 0x0c, IF_index, 0x9f ); // TODO 0x0F or 0x9F?
			if ( rc < 0 )
				return rc;

			stagegains[ 2 ] = gain;

			return 0;
		}
	}
	return -EINVAL;
}

int r82xxTuner::r82xx_set_freq( uint32_t freq, uint32_t *lo_freq_out )
{
	int rc = -1;
	uint32_t lo_freq = freq + m_nominal_int_freq;	//	Dongle Synthesizer Freq.
	uint32_t margin = (uint32_t) ( 1e6 + m_bw / 2 );	//	TODOTODO: Possibly cause error!
	uint8_t air_cable1_in;

	int changed_pll_limits = 0;

	r82xx_write_batch_init();

	/* RF input settings */

	rc = r82xx_set_mux( freq );

	/* switch between 'Cable1' and 'Air-In' inputs on sticks with
	 * R828D tuner. We switch at 345 MHz, because that's where the
	 * noise-floor has about the same level with identical LNA
	 * settings. The original driver used 320 MHz. */
	air_cable1_in = ( freq > MHZ( 345 )) ? 0x00 : 0x60;

	if (( m_rafael_chip == CHIP_R828D )
	&&	( air_cable1_in != m_input ))
	{
		m_input = air_cable1_in;
		rc |= r82xx_write_reg_mask( 0x05, air_cable1_in, 0x60 );
	}


	/* IF generation settings */

 retune:
	if (( freq < 14400000 ) && ( freq < ( m_pll_low_limit - 14400000 )))
	{
		/* Previously "no-mod direct sampling" - confuse the VCO/PLL
		 * sufficiently that we get the HF signal leaking through
		 * the tuner, then sample that directly.
		 *
		 * Disable the VCO, as far as we can.
		 * This throws a big spike of noise into the signal,
		 * so only do it once when crossing the 14.4MHz boundary,
		 * not on every retune.
		 */
		if ( !m_pll_off )
		{
			rc |= r82xx_set_pll( 50000000, NULL );			/* Might influence the noise floor? */
			rc |= r82xx_write_reg_mask( 0x10, 0xd0, 0xe0 );	/* impossible mix_div setting */
			rc |= r82xx_write_reg_mask( 0x12, 0xe0, 0xe0 );	/* VCO current = 0 */
			m_pll_off = 1;
		}

		/* We are effectively tuned to 0Hz - the downconverter must do all the heavy lifting now */
		lo_freq = 0;
		if ( lo_freq_out )
			*lo_freq_out = 0;
	}
	else
	{
		/* Normal tuning case */
		int pll_error = 0;

		if ( m_pll_off )
		{
			/* Crossed the 14.4MHz boundary, power the VCO back on */
			rc |= r82xx_write_reg_mask( 0x12, 0x80, 0xe0 );
			m_pll_off = 0;
		}

		/*
		 * Keep PLL within empirically stable bounds; outside those bounds,
		 * we prefer to tune to the "wrong" frequency; the difference will be
		 * mopped up by the 2832 downconverter.
		 *
		 * Beware that outside the stable range, the PLL can claim to be locked
		 * while it is actually stuck at a different frequency (e.g. sometimes
		 * it can claim to get PLL lock when configured anywhere between 24 and
		 * 26MHz, but it actually always locks to 26.6-ish).
		 *
		 * Make sure to keep the LO away from tuned frequency as there seems
		 * to be a ~600kHz high-pass filter in the IF path, so you don't want
		 * any interesting frequencies to land near the IF.
		 */

		if ( lo_freq < m_pll_low_limit )
		{
			if (( freq > ( m_pll_low_limit - margin ))
			&&	( freq < ( m_pll_low_limit + margin )))
			{
				lo_freq = freq + margin;
			}
			else
			{
				lo_freq = m_pll_low_limit;
			}
		}
		else
		if ( lo_freq > m_pll_high_limit )
		{
			if (( freq > ( m_pll_high_limit - margin ))
			&&	( freq < ( m_pll_high_limit + margin )))
			{
				lo_freq = freq - margin;
			}
			else
			{
				lo_freq = m_pll_high_limit;
			}
		}

		pll_error = r82xx_set_pll( lo_freq, lo_freq_out );
		if ( pll_error == -42 )
		{
			/* Magic return value to say that the PLL didn't lock.
			 * If we are close to the edge of the PLL range, shift the range and try again.
			 */
			if ( *lo_freq_out < PLL_SAFE_LOW )
			{
				m_pll_low_limit = *lo_freq_out + PLL_STEP_LOW;
				if ( m_pll_low_limit > PLL_SAFE_LOW )
					m_pll_low_limit = PLL_SAFE_LOW;
				changed_pll_limits = 1;
				goto retune;
			}
			else
			if ( *lo_freq_out > PLL_SAFE_HIGH )
			{
				m_pll_high_limit = *lo_freq_out - PLL_STEP_HIGH;
				if ( m_pll_high_limit < PLL_SAFE_HIGH )
					m_pll_high_limit = PLL_SAFE_HIGH;
				changed_pll_limits = 1;
				goto retune;
			}
			else
			{
				fprintf(stderr, "[r82xx] Failed to get PLL lock at %u Hz\n", lo_freq);
				TRACE( "[r82xx] Failed to get PLL lock at %u Hz\n", lo_freq);
			}
		}

		rc = pll_error;
	}

	if ( changed_pll_limits )
	{
		fprintf( stderr, "[r82xx] Updated PLL limits to %u .. %u Hz\n", m_pll_low_limit, m_pll_high_limit);
		TRACE( "[r82xx] Updated PLL limits to %u .. %u Hz\n", m_pll_low_limit, m_pll_high_limit);
	}

	/* IF filter / image rejection settings */

	if ( *lo_freq_out > freq )
	{
		/* high-side mixing, image negative */
		rc |= r82xx_write_reg_mask(  0x07, 0x00, 0x80 );
		m_if_filter_freq = *lo_freq_out - freq;
	}
	else
	{
		/* low-side mixing, image positive */
		rc |= r82xx_write_reg_mask( 0x07, 0x80, 0x80 );
		m_if_filter_freq = freq - *lo_freq_out;
	}

	if ( m_reg_batch )
	{
		rc |= r82xx_write_batch_sync();
	}

	if ( rc < 0 )
	{
		fprintf(stderr, "%s: failed=%d\n", __FUNCTION__, rc);
		TRACE( "%s: failed=%d\n", __FUNCTION__, rc);
	}

	m_asked_for_freq = freq;

	return rc;
}

#if 0	//	NOT USED
int r82xxTuner::r82xx_set_nomod( void )
{
	int rc = -1;

	fprintf(stderr, "Using R820T no-mod direct sampling mode\n");

	/*rc = r82xx_set_bw( 1000000 );
	if (rc < 0)
		goto err;*/

	/* experimentally determined magic numbers
	 * needs more experimenting with all the registers */
	rc = r82xx_set_mux( 300000000 );
	if ( rc < 0 )
		goto err;

	r82xx_set_pll( 25000000, NULL );

err:
	if ( rc < 0 )
	{
		fprintf(stderr, "%s: failed=%d\n", __FUNCTION__, rc);
		TRACE( "%s: failed=%d\n", __FUNCTION__, rc);
	}
	return rc;
}
#endif

int r82xxTuner::r82xx_get_tuner_bandwidths( const uint32_t **bandwidths, int *len )
{
	if ( !len & !bandwidths )
		return -1;

#if defined( LEIF_TABLES )
	*len = r82xx_bandwidth_table_len * sizeof( int );
	if ( bandwidths != NULL )
		*bandwidths = r82xx_bandwidth_table;
	else
		return *len;
#else
	*len = m_FilterSetCount;
	if ( bandwidths != NULL )
		*bandwidths = m_bandwidths;
	else
		return *len;
#endif
	return 0;
}


int r82xxTuner::r82xx_get_bandwidth_set_name
										( int nSet
										, LPSTR pString
										)
{
	if ( pString == NULL )
		return -1;
	if ((DWORD) nSet >= (DWORD) BWSNcount )
	{
		pString[ 0 ] = 0;
		return -1;
	}
	else
	{
		int size = BandwidthSetNames[ nSet ].GetLength();
		memcpy( pString
			  , (LPCSTR) BandwidthSetNames[ nSet ]
			  , size
			  );
		pString[ size ] = 0;
		return 0;
	}
}


int r82xxTuner::r82xx_set_bandwidth_set( int nSet )
{
	if ((DWORD) nSet >= (DWORD) BWSNcount )
		return -1;
	else
	{
		//	Set it somehow.
		r82xx_SetBWValues( nSet );
		return 0;
	}
}


int r82xxTuner::r82xx_set_dither( int dither )
{
	m_disable_dither = !dither;
	return 0;
}

/*
 * r82xx standby logic
 */

int r82xxTuner::r82xx_standby( void )
{
	int rc;

	/* If device was not initialized yet, don't need to standby */
	if ( !m_init_done )
		return 0;

	m_reg_cache = 0;
	rc = r82xx_write_reg( 0x06, 0xb1 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x05, 0x03 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x07, 0x3a );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x08, 0x40 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x09, 0xc0 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x0a, 0x36 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x0c, 0x35 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x0f, 0x68 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x11, 0x03 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x17, 0xf4 );
	if ( rc < 0 )
		return rc;
	rc = r82xx_write_reg( 0x19, 0x0c );

	/* Force initial calibration */
	m_reg_cache = 1;
	return rc;
}

/*
 * r82xx device init logic
 */

/* Done in r82xx_set_mux these days. */
#if 0
int r82xxTuner::r82xx_xtal_check( void )
{
	int rc;
	unsigned int i;
	uint8_t data[ 3 ];
	uint8_t val;

	/* Initialize the shadow registers */
	memcpy( m_regs, r82xx_init_array, sizeof( r82xx_init_array ));

	/* cap 30pF & Drive Low */
	rc = r82xx_write_reg_mask( 0x10, 0x0b, 0x0b );
	if ( rc < 0 )
		return rc;

	/* set pll autotune = 128kHz */
	rc = r82xx_write_reg_mask( 0x1a, 0x00, 0x0c );
	if ( rc < 0 )
		return rc;

	/* set manual initial reg = 111111;  */
	rc = r82xx_write_reg_mask( 0x13, 0x7f, 0x7f );
	if ( rc < 0 )
		return rc;

	/* set auto */
	rc = r82xx_write_reg_mask( 0x13, 0x00, 0x40 );
	if ( rc < 0 )
		return rc;

	/* Try several xtal capacitor alternatives */
	for ( i = 0; i < ARRAY_SIZE( r82xx_xtal_capacitor ); i++ )
	{
		rc = r82xx_write_reg_mask( 0x10, r82xx_xtal_capacitor[ i ][ 0 ], 0x1b );
		if ( rc < 0 )
			return rc;

//		usleep_range(5000, 6000);

		rc = r82xx_read( 0x00, data, sizeof( data ));
		if ( rc < 0 )
			return rc;
		if (( data[ 2 ] & 0x40 ) != 0 )
			continue;

		val = data[ 2 ] & 0x3f;

		if (( m_xtal == 16000000 ) && (( val > 29 ) || ( val < 23 )))
			break;

		if ( val != 0x3f )
			break;
	}

	if ( i == ARRAY_SIZE( r82xx_xtal_capacitor ))
		return -1;

	return r82xx_xtal_capacitor[ i ][ 1 ];
}
#endif

int r82xxTuner::r82xx_init( void )
{
	int rc;

	/* TODO: R828D might need r82xx_xtal_check() */
	m_xtal_cap_sel = XTAL_HIGH_CAP_0P;

	/* Initialize registers */
	m_reg_cache = 0;
	rc = r82xx_write( 0x05, r82xx_init_array, sizeof( r82xx_init_array ));

	rc |= r82xx_init_tv_standard();

	m_bw       = R82XX_DEFAULT_IF_BW;
	m_nominal_int_freq = R82XX_DEFAULT_IF_FREQ;
	/* r82xx_set_bw will always be called by rtlsdr_set_sample_rate,
	   so there's no need to call r82xx_set_if_filter here */

	rc |= r82xx_sysfreq_sel();
	
	/* set VCO current = 100 */
	rc = r82xx_write_reg_mask( 0x12, 0x00, 0xe0 );

	m_pll_low_limit = PLL_INITIAL_LOW;
	m_pll_high_limit = PLL_INITIAL_HIGH;

	r82xx_calculate_stage_gains();
	r82xx_compute_gain_table();

	m_init_done = 1;
	m_reg_cache = 1;

	if ( rc < 0 )
	{
		fprintf(stderr, "%s: failed=%d\n", __FUNCTION__, rc);
		TRACE( "%s: failed=%d\n", __FUNCTION__, rc);
	}
	return rc;
}

/* 
"inspired by Mauro Carvalho Chehab calibration technique"
https://stuff.mit.edu/afs/sipb/contrib/linux/drivers/media/tuners/r820t.c
part of r820t_set_tv_standard()
*/
#define CALIBRATION_LO 88000	// Airspy
int r82xxTuner::r82xx_calibrate( int calibration_lo, uint8_t hp_cor )
{
	int i;
	int rc;
	uint8_t data[ 5 ];

	for ( i = 0; i < 5; i++ )
	{
		/* Set filt_cap */
		rc = r82xx_write_reg_mask( 0x0b, hp_cor /*0x08*/, 0x60 );	//	jd: Leif
		if ( rc < 0 )
			return rc;

		/* set cali clk =on */
		rc = r82xx_write_reg_mask( 0x0f, 0x04, 0x04 );
		if ( rc < 0 )
			return rc;

		/* X'tal cap 0pF for PLL */
		rc = r82xx_write_reg_mask( 0x10, 0x00, 0x03);
		if ( rc < 0 )
			return rc;

		rc = r82xx_set_pll( calibration_lo * 1000, NULL );
		if ( rc < 0 )
			return rc;

		/* Start Trigger */
		rc = r82xx_write_reg_mask( 0x0b, 0x00, 0x10);
		if ( rc < 0 )
			return rc;

		Sleep( 1 );

		/* Stop Trigger */
		rc = r82xx_write_reg_mask( 0x0b, 0x00, 0x10 );
		if ( rc < 0 )
			return rc;

		/* set cali clk =off */
		rc = r82xx_write_reg_mask( 0x0f, 0x00, 0x04 );
		if ( rc < 0 )
			return rc;

		/* Check if calibration worked */
		rc = r82xx_read( 0x00, data, sizeof( data ));
		if ( rc < 0 )
			return rc;

		m_fil_cal_code = data[ 4 ] & 0x0f;
		if ( m_fil_cal_code && ( m_fil_cal_code != 0x0f ))
			break;
	}

	if ( m_fil_cal_code == 0x0f )
		m_fil_cal_code = 0;

	return i >= 5 ? -1 : 0;
}

#if 0
/* Not used, for now */
static int r82xx_gpio( int enable)
{
	return r82xx_write_reg_mask( 0x0f, enable ? 1 : 0, 0x01);
}
#endif

#if defined SET_SPECIAL_FILTER_VALUES
int r82xxTuner::SetFilterValuesDirect( BYTE regA
									 , BYTE regB
									 , DWORD ifFreq
									 )
{
	int rc;
	rc = r82xx_write_reg_mask( 0x0a, regA, 0xff );
	if ( rc < 0 )
		return rc;

	rc = r82xx_write_reg_mask( 0x0b, regB, 0xff );
	if ( rc < 0 )
		return rc;

	m_nominal_int_freq = ifFreq;
	return rc;
}
#endif


void r82xxTuner::r82xx_SetBWValues( int nSet )
{
	int size = 0;
	m_FilterSetCount = 0;
	switch( nSet )
	{
	case 0:
		m_FilterInfo = m_FilterInfo0;
		size = sizeof( m_FilterInfo0 ) / sizeof( tFilterInfo );
		break;
	case 1:
		m_FilterInfo = m_FilterInfo1;
		size = sizeof( m_FilterInfo1 ) / sizeof( tFilterInfo );
	}

	if ( m_bandwidths )
		delete m_bandwidths;
	m_bandwidths = new uint32_t[ size ];
	if ( m_bandwidths )
	{
		for( int i = 0; i < size; i++ )
		{
			m_bandwidths[ i ] = m_FilterInfo[ i ].Bandwidth;
		}
		m_FilterSetCount = size;
	}
}
