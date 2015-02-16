//	ITuner.h
#pragma once

#define PURE		= 0

class rtlsdr;

class ITuner
{
public:
	//	ITuner interface
	virtual int init					( rtlsdr* base ) PURE;	//	new?
	virtual int exit					( void ) PURE;	//	delete?
	virtual int set_freq				( uint32_t freq /* Hz */
										, uint32_t *lo_freq_out
										) PURE;
	virtual int set_bw					( int bw /* Hz */) PURE;
	virtual	int get_gain				( void ) PURE;	/* tenth dB. Computerd gain if necessary. */
	virtual int set_gain				( int gain /* tenth dB */) PURE;
	virtual int set_if_gain				( int stage
										, int gain /* tenth dB */
										) PURE;
	virtual int get_tuner_stage_gains	( uint8_t stage
										, const int32_t **gains
										, const char **description
										) PURE;
	virtual int get_tuner_stage_count	( void ) PURE;
	virtual int get_tuner_stage_gain	( uint8_t stage ) PURE;
	virtual int set_tuner_stage_gain	( uint8_t stage
										, int gain
										) PURE;
	virtual int set_gain_mode			( int manual ) PURE;
	virtual int set_dither				( int dither ) PURE;

	virtual int	get_xtal_frequency		( uint32_t& xtalfreq ) PURE;
	virtual int	set_xtal_frequency		( uint32_t xtalfreq ) PURE;

	virtual int	get_tuner_gains			( const int **ptr
										, int *len
										) PURE;
};
