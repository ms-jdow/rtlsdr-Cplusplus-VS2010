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

#pragma once
#include "librtlsdr.h"

#define R820T_I2C_ADDR		0x34
#define R828D_I2C_ADDR		0x74
#define R828D_XTAL_FREQ		16000000

#define R82XX_CHECK_ADDR	0x00
#define R82XX_CHECK_VAL		0x69

#define R82XX_DEFAULT_IF_FREQ   6000000
#define R82XX_DEFAULT_IF_BW     2000000

#define REG_SHADOW_START	5
#define NUM_REGS			30
#define NUM_IMR				5
#define IMR_TRIAL			9

#define VER_NUM				49

enum r82xx_chip
{
	CHIP_R820T,
	CHIP_R620D,
	CHIP_R828D,
	CHIP_R828,
	CHIP_R828S,
	CHIP_R820C,
};

typedef enum _r82xx_tuner_type
{
	TUNER_UNKNOWN = -1,
	TUNER_RADIO = 1,
	TUNER_ANALOG_TV,
	TUNER_DIGITAL_TV
} r82xx_tuner_type;

typedef enum r82xx_xtal_cap_value
{
	XTAL_LOW_CAP_30P = 0,
	XTAL_LOW_CAP_20P,
	XTAL_LOW_CAP_10P,
	XTAL_LOW_CAP_0P,
	XTAL_HIGH_CAP_0P
} r82xx_xtal_cap_val;


class r82xxTuner : public ITuner
{
public:
	r82xxTuner( rtlsdr* base, enum rtlsdr_tuner rafaelTunerType );
	~r82xxTuner() {}

	//	ITuner interface
	virtual int init					( rtlsdr* base );
	virtual int exit					( void );
	virtual int set_freq				( uint32_t freq /* Hz */
										, uint32_t *lo_freq_out
										);
	virtual int set_bw					( int bw /* Hz */);
	virtual	int get_gain				( void );	/* tenth dB */
	virtual int set_gain				( int gain /* tenth dB */);
	virtual int set_if_gain				( int stage
										, int gain /* tenth dB */
										);
	virtual int get_tuner_stage_count	( void ) { return 3; }
	virtual int get_tuner_stage_gains	( uint8_t stage
										, const int32_t **gains
										, const char **description
										);
	virtual int get_tuner_stage_gain	( uint8_t stage );	// TODOTODO
	virtual int set_tuner_stage_gain	( uint8_t stage
										, int32_t gain
										);
	virtual int set_gain_mode			( int manual );
	virtual int set_dither				( int dither );

	virtual int	get_xtal_frequency		( uint32_t& xtalfreq );
	virtual int	set_xtal_frequency		( uint32_t xtalfreq );
	virtual int	get_tuner_gains			( const int **ptr
										, int *len
										);

protected:
	void		ClearVars				( void );
	void		shadow_store			( uint8_t reg
										, const uint8_t *val
										, int len
										);
	int			r82xx_write				( uint8_t reg
									   , const uint8_t *val
									   , unsigned int len
									   );
	int			r82xx_write_reg			( uint8_t reg
										, uint8_t val
										);
	int			r82xx_read_cache_reg	( int reg );
	int			r82xx_write_reg_mask	( uint8_t reg
										, uint8_t val
										, uint8_t bit_mask
										);
	int			r82xx_write_batch_init	( void );
	int			r82xx_write_batch_sync	( void );
	uint8_t		r82xx_bitrev			( uint8_t byte );
	int			r82xx_read				( uint8_t reg
										, uint8_t *val
										, int len
										);
	int			r82xx_set_mux			( uint32_t freq );
	int			r82xx_set_pll			( uint32_t freq
										, uint32_t *freq_out
										);
	int			r82xx_sysfreq_sel		( uint32_t in_freq
										, r82xx_tuner_type in_type
										, uint32_t in_delsys
										);
	int			r82xx_init_tv_standard	( unsigned in_bw
										, r82xx_tuner_type in_type
										, uint32_t in_delsys
										);
	int			update_if_filter		( void );
	int			r82xx_set_bw			( uint32_t bw );
	int			r82xx_read_gain			( void );
	int			r82xx_set_gain			( int gain );
	int			r82xx_set_freq			( uint32_t freq
										, uint32_t *lo_freq_out
										);
	int			r82xx_xtal_check		( void );
	int			r82xx_standby			( void );
	int			r82xx_init				( void );
	int			r82xx_set_nomod			( void );
	int			r82xx_set_dither		( int dither );

	int			r82xx_enable_manual_gain( uint8_t manual);

	int			r82xx_get_tuner_stage_gains
										( uint8_t stage
										, const int32_t **gains
										, const char **description
										);
	int			r82xx_set_tuner_stage_gain
										( uint8_t stage
										, int32_t gain
										);
	void		r82xx_calculate_stage_gains
										( void );
	void		r82xx_compute_gain_table( void );
	int			r82xx_set_lna_gain		( int32_t gain );
	int			r82xx_set_mixer_gain	( int32_t gain );
	int			r82xx_set_VGA_gain		( int32_t gain );

protected:	//	config variables
	uint8_t				m_i2c_addr;
	uint32_t			m_xtal;
	enum r82xx_chip		m_rafael_chip;
	unsigned int		m_max_i2c_msg_len;
	int					m_use_predetect;

protected:	//	private variables
	uint8_t				m_regs[ NUM_REGS ];
	uint8_t				m_buf[ NUM_REGS + 1 ];
	r82xx_xtal_cap_val	m_xtal_cap_sel;
	uint16_t			m_pll;	/* kHz */
	uint32_t			m_int_freq;
	uint8_t				m_fil_cal_code;
	uint8_t				m_input;
	int					m_init_done;
	int					m_disable_dither;
	int					m_reg_cache;
	int					m_reg_batch;
	int					m_reg_low;
	int					m_reg_high;

	/* Store current mode */
	uint32_t			m_delsys;
	r82xx_tuner_type	m_type;

	uint32_t			m_bw;	/* in MHz */
	uint32_t			m_if_filter_freq;	/* in Hz */

	int					m_pll_off;

	/* current PLL limits */
	uint32_t			m_pll_low_limit;
	uint32_t			m_pll_high_limit;

	//	Stage gains
	int					gain_mode;
	int					stagegains[ 3 ];

	rtlsdr*				rtldev;
};
