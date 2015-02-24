#pragma once

#if !defined SDRDAPI
#define SDRDAPI
#endif

#define EEPROM_SIZE		256
typedef uint8_t eepromdata[ EEPROM_SIZE ];

class /*SDRDAPI*/ IRtlSdr
{
public:
	virtual
	int			rtlsdr_set_xtal_freq		( uint32_t rtl_freq
											, uint32_t tuner_freq
											) PURE;
	virtual
	int			rtlsdr_get_xtal_freq		( uint32_t *rtl_freq
											, uint32_t *tuner_freq
											) PURE;
	virtual
	int			rtlsdr_write_eeprom			( uint8_t *data
											, uint8_t offset
											, uint16_t len
											) PURE;
	virtual
	int			rtlsdr_read_eeprom			( uint8_t *data
											, uint8_t offset
											, uint16_t len
											) PURE;
	virtual
	int			rtlsdr_read_eeprom			( eepromdata& data ) PURE;
	virtual
	int			rtlsdr_set_center_freq		( uint32_t in_freq ) PURE;
	virtual
	uint32_t	rtlsdr_get_center_freq		( void ) PURE;
	virtual
	int			rtlsdr_set_freq_correction	( int ppm ) PURE;
	virtual
	int			rtlsdr_get_freq_correction
											( void ) PURE;
	virtual
	int			rtlsdr_get_tuner_type		( void ) PURE;
	virtual
	int			rtlsdr_get_tuner_gains		( int *gains ) PURE;
	virtual
	int			rtlsdr_set_tuner_gain		( int gain ) PURE;
	virtual
	int			rtlsdr_get_tuner_gain		( void ) PURE;
	virtual
	int			rtlsdr_set_tuner_if_gain	( int stage
											, int gain
											) PURE;
	virtual
	int			rtlsdr_get_tuner_stage_gains( uint8_t stage
											, int32_t *gains
											, char *description
											) PURE;
	virtual
	int			rtlsdr_get_tuner_stage_count( void ) PURE;
	virtual
	int			rtlsdr_set_tuner_stage_gain	( uint8_t stage
											, int32_t gain
											) PURE;
	virtual
	int			rtlsdr_get_tuner_stage_gain	( uint8_t stage ) PURE;
	virtual
	int			rtlsdr_set_tuner_gain_mode	( int mode ) PURE;
	virtual
	int			rtlsdr_set_sample_rate		( uint32_t samp_rate ) PURE;
	virtual
	uint32_t	rtlsdr_get_sample_rate		( void ) PURE;
	virtual
	uint32_t	rtlsdr_get_corr_sample_rate	( void ) PURE;
	virtual
	int			rtlsdr_set_testmode			( int on ) PURE;
	virtual
	int			rtlsdr_set_agc_mode			( int on ) PURE;
	virtual
	int			rtlsdr_set_direct_sampling	( int on ) PURE;
	virtual
	int			rtlsdr_get_direct_sampling
											( void ) PURE;
	virtual
	int			rtlsdr_set_offset_tuning	( int on ) PURE;
	virtual
	int			rtlsdr_get_offset_tuning	( void ) PURE;
	virtual
	int			rtlsdr_set_dithering		( int dither ) PURE;
	virtual
//	int			rtlsdr_get_tuner_type		( int index ) PURE;
//	virtual
	int			rtlsdr_open					( uint32_t index ) PURE;
	virtual
	int			rtlsdr_close				( void ) PURE;
	virtual
	uint32_t	rtlsdr_get_tuner_clock		( void ) PURE;
	virtual
	int			rtlsdr_get_usb_strings		( char *manufact
											, char *product
											, char *serial
											) PURE;
	virtual
	int			rtlsdr_get_usb_strings		( CString& manufact
											, CString& product
											, CString& serial
											) PURE;
	virtual
	int			rtlsdr_reset_buffer			( void ) PURE;
	virtual
	int			rtlsdr_read_sync			( BYTE *buf
											, int len
											, int *n_read
											) PURE;
	virtual
	int			rtlsdr_wait_async			( rtlsdr_read_async_cb_t cb
											, void *ctx
											) PURE;
	virtual
	int			rtlsdr_read_async			( rtlsdr_read_async_cb_t in_cb
											, void *in_ctx
											, uint32_t buf_num
											, uint32_t buf_len
											) PURE;
	virtual
	int			rtlsdr_cancel_async			( void ) PURE;
	virtual
	const char* rtlsdr_get_version			( void ) PURE;
	virtual
	unsigned __int64 
				rtlsdr_get_version_int64	( void ) PURE;
	virtual
	uint32_t	rtlsdr_get_device_count		( void ) PURE;
	virtual
	const char *rtlsdr_get_device_name		( uint32_t index ) PURE;
	virtual
	int			rtlsdr_get_device_usb_strings
											( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											) PURE;
	virtual
	int			rtlsdr_get_index_by_serial	( const char *serial ) PURE;
};
