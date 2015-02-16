#pragma once

#include "librtlsdr.h"

#define	BORDER_FREQ	2600000	//2.6GHz : The border frequency which determines whether Low VCO or High VCO is used
#define USE_EXT_CLK	0	//0 : Use internal XTAL Oscillator / 1 : Use External Clock input
#define OFS_RSSI 57

#define FC2580_I2C_ADDR		0xac
#define FC2580_CHECK_ADDR	0x01
#define FC2580_CHECK_VAL	0x56

typedef enum
{
	FC2580_UHF_BAND,
	FC2580_L_BAND,
	FC2580_VHF_BAND,
	FC2580_NO_BAND
} fc2580_band_type;

typedef enum
{
	FC2580_FCI_FAIL,
	FC2580_FCI_SUCCESS
} fc2580_fci_result_type;

enum FUNCTION_STATUS
{
	FUNCTION_SUCCESS,
	FUNCTION_ERROR,
};


class fc2580Tuner : public ITuner
{
public:
	fc2580Tuner( rtlsdr* base );
	~fc2580Tuner();

	//	ITuner interface
	virtual int init					( rtlsdr* base );
	virtual int exit					( void ) _DUMMY_
	virtual int set_freq				( uint32_t freq /* Hz */
										, uint32_t *lo_freq_out
										);
	virtual int set_bw					( int bw /* Hz */);
	virtual	int get_gain				( void ) _DUMMY_	/* tenth dB */
	virtual int set_gain				( int gain /* tenth dB */) _DUMMY_
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
	virtual int set_gain_mode			( int manual ) _DUMMY_
	virtual int set_dither				( int dither ) _DUMMY_

	virtual int	get_xtal_frequency		( uint32_t& xtalfreq );
	virtual int	set_xtal_frequency		( uint32_t xtalfreq ) _DUMMY_
	virtual int	get_tuner_gains			( const int **ptr
										, int *len
										);

protected:
	fc2580_fci_result_type
				fc2580_i2c_write		( unsigned char reg
										, unsigned char val
										);
	fc2580_fci_result_type
				fc2580_i2c_read			( unsigned char reg
										, unsigned char *read_data
										);

	/*==============================================================================
		   fc2580 initial setting

	  This function is a generic function which gets called to initialize

	  fc2580 in DVB-H mode or L-Band TDMB mode

	  <input parameter>

	  ifagc_mode
		type : integer
		1 : Internal AGC
		2 : Voltage Control Mode

	==============================================================================*/
	fc2580_fci_result_type
				fc2580_set_init			( int ifagc_mode
										, unsigned int freq_xtal
										);

	/*==============================================================================
		   fc2580 frequency setting

	  This function is a generic function which gets called to change LO Frequency

	  of fc2580 in DVB-H mode or L-Band TDMB mode

	  <input parameter>

	  f_lo
		Value of target LO Frequency in 'kHz' unit
		ex) 2.6GHz = 2600000

	==============================================================================*/
	fc2580_fci_result_type
				fc2580_set_freq			( unsigned int f_lo
										, unsigned int freq_xtal
										);


	/*==============================================================================
		   fc2580 filter BW setting

	  This function is a generic function which gets called to change Bandwidth

	  frequency of fc2580's channel selection filter

	  <input parameter>

	  filter_bw
		1 : 1.53MHz(TDMB)
		6 : 6MHz
		7 : 7MHz
		8 : 7.8MHz


	==============================================================================*/
	fc2580_fci_result_type
				fc2580_set_filter		( unsigned char filter_bw
										, unsigned int freq_xtal
										);


	// Manipulaing functions
	int			fc2580_Initialize		( void );

	int			fc2580_SetRfFreqHz		( unsigned long RfFreqHz );

	// Extra manipulaing functions
	int			fc2580_SetBandwidthMode	( int BandwidthMode );

	void		fc2580_wait_msec		( int a );	// noop for now
protected:
	rtlsdr*		rtldev;
};
