//	librtlsdr.h	// Defines the rtlsdr_dev class.

#pragma once

#define _DUMMY_	{ return -1; }

#include "libusb/libusb.h"
#include "ITuner.h"
#include "rtl-sdr.h"
#include "DongleArrays.h"

typedef struct rtlsdr_dongle rtlsdr_dongle_t;
typedef void(*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

#define FIR_LEN 16
class e4kTuner;
class fc0012Tuner;
class fc0013Tuner;
class fc2580Tuner;
class r82xxTuner;

class SDRDAPI rtlsdr
{
public:
	rtlsdr								();
	~rtlsdr								();	// Unwind tuners.

protected:
	friend e4kTuner;
	friend fc0012Tuner;
	friend fc0013Tuner;
	friend fc2580Tuner;
	friend r82xxTuner;

	//	Dongle communications
	int			rtlsdr_read_array		( uint8_t block
										, uint16_t addr
										, uint8_t *data
										, uint8_t len
										);
	int			rtlsdr_write_array		( uint8_t block
										, uint16_t addr
										, uint8_t *data
										, uint8_t len
										);
	int			rtlsdr_i2c_write_reg	( uint8_t i2c_addr
										, uint8_t reg
										, uint8_t val
										);
	uint8_t		rtlsdr_i2c_read_reg		( uint8_t i2c_addr
										, uint8_t reg
										);
	int			rtlsdr_i2c_write		( uint8_t i2c_addr
										, uint8_t *buffer
										, int len
										);
	int			rtlsdr_i2c_read			( uint8_t i2c_addr
										, uint8_t *buffer
										, int len
										);
	uint16_t	rtlsdr_read_reg			( uint8_t block
										, uint16_t addr
										, uint8_t len
										);
	int			rtlsdr_write_reg		( uint8_t block
										, uint16_t addr
										, uint16_t val
										, uint8_t len
										);
	uint16_t	rtlsdr_demod_read_reg	( uint8_t page
										, uint16_t addr
										, uint8_t len);
	int			rtlsdr_demod_write_reg	( uint8_t page
										, uint16_t addr
										, uint16_t val
										, uint8_t len
										);
	void		rtlsdr_set_gpio_bit		( uint8_t gpio
										, int val
										);
	void		rtlsdr_set_gpio_output	( uint8_t gpio );
	void		rtlsdr_set_i2c_repeater	( int on );
	int			rtlsdr_i2c_write_fn		( uint8_t addr
										, uint8_t *buf
										, int len
										);
	int			rtlsdr_i2c_read_fn		( uint8_t addr
										, uint8_t *buf
										, int len
										);


protected:	// The real work.
	int			rtlsdr_set_fir			( void );
	void		rtlsdr_init_baseband	( void );
	int			rtlsdr_deinit_baseband	( void );
	int			rtl2832_set_if_freq		( uint32_t freq
										, uint32_t *freq_out
										);
	int			set_spectrum_inversion	( int inverted );
	int			rtlsdr_set_sample_freq_correction
										( int ppm );
	
public:
	libusb_device_handle*
				GetDeviceHandle			( void )	{ return devh; }
	int			rtlsdr_set_xtal_freq	( uint32_t rtl_freq
										, uint32_t tuner_freq
										);
	int			rtlsdr_get_xtal_freq	( uint32_t *rtl_freq
										, uint32_t *tuner_freq
										);
	int			rtlsdr_write_eeprom		( uint8_t *data
										, uint8_t offset
										, uint16_t len
										);
	int			rtlsdr_read_eeprom		( uint8_t *data
										, uint8_t offset
										, uint16_t len
										);
	int			rtlsdr_set_center_freq	( uint32_t in_freq );
	uint32_t	rtlsdr_get_center_freq	( void );
	int			rtlsdr_set_freq_correction
										( int ppm );
	int			rtlsdr_get_freq_correction
										( void );
	int			rtlsdr_get_tuner_type	( void );
	int			rtlsdr_get_tuner_gains	( int *gains );
	int			rtlsdr_set_tuner_gain	( int gain );
	int			rtlsdr_get_tuner_gain	( void );
	int			rtlsdr_set_tuner_if_gain( int stage
										, int gain
										);
	int			rtlsdr_set_tuner_gain_mode
										( int mode );
	int			rtlsdr_set_sample_rate	( uint32_t samp_rate );
	uint32_t	rtlsdr_get_sample_rate	( void );
	uint32_t	rtlsdr_get_corr_sample_rate
										( void );
	int			rtlsdr_set_testmode		( int on );
	int			rtlsdr_set_agc_mode		( int on );
	int			rtlsdr_set_direct_sampling
										( int on );
	int			rtlsdr_get_direct_sampling
										( void );
	int			rtlsdr_set_offset_tuning( int on );
	int			rtlsdr_get_offset_tuning( void );
	int			rtlsdr_set_dithering	( int dither );
	int			rtlsdr_get_tuner_type	( int index );

public:
	// Static functions
	static int		rtlsdr_get_usb_strings	( libusb_device_handle* in_devh
											, char *manufact
											, char *product
											, char *serial
											);
	static uint32_t	rtlsdr_get_device_count	( void );
	static const char *rtlsdr_get_device_name
											( uint32_t index );
	static int		rtlsdr_get_device_usb_strings
											( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											);
	static int		rtlsdr_get_index_by_serial
											( const char *serial );
	// Non-static public functions
	int			rtlsdr_open				( uint32_t index );
	int			rtlsdr_close			( void );
	int			rtlsdr_reset_buffer		( void );
	int			rtlsdr_read_sync		( BYTE *buf
										, int len
										, int *n_read
										);
	void LIBUSB_CALL
				libusb_callback			( struct libusb_transfer *xfer );
	int			rtlsdr_wait_async		( rtlsdr_read_async_cb_t cb
										, void *ctx
										);
	int			rtlsdr_read_async		( rtlsdr_read_async_cb_t in_cb
										, void *in_ctx
										, uint32_t buf_num
										, uint32_t buf_len
										);
	int			rtlsdr_cancel_async		( void );
	uint32_t	rtlsdr_get_tuner_clock	( void );
	static int	rtlsdr_static_get_tuner_type
										( int index );


	// SPECIALS
	static void	GetCatalog				( void );
	static void	FillDongleArraysAndComboBox
										( CDongleArray * da
										, CComboBox * combo
										, LPCTSTR selected
										);

	static bool	GetDongleIdString		( CString& string
										, int devindex
										);
	static int	FindDongleByIdString	( LPCTSTR source );
	int			GetOpenedDeviceIndex	( void ) { return deviceOpened; }

protected:
	static void	WriteRegistry			( void );
	static void	ReadRegistry			( void );
	static void mergeToMaster			( Dongle& tempd
										, int index
										);
	static bool	reinitDongles			( void );

private:
	static rtlsdr_dongle_t *
				find_known_device		( uint16_t vid
										, uint16_t pid
										);
	void		ClearVars				( void );
	int			_rtlsdr_alloc_async_buffers
										( void );
	int			_rtlsdr_free_async_buffers
										( void );

private:
	libusb_context *					ctx;
	struct libusb_device_handle *		devh;
	uint32_t							xfer_buf_num;
	uint32_t							xfer_buf_len;
	struct libusb_transfer **			xfer;
	unsigned char **					xfer_buf;
	rtlsdr_read_async_cb_t				cb;
	void *								cb_ctx;
	enum rtlsdr_async_status			async_status;
	int									async_cancel;
	/* rtl demod context */
	uint32_t							rate; /* Hz */
	uint32_t							rtl_xtal; /* Hz */
	int									fir[ FIR_LEN ];
	int									direct_sampling;
	/* tuner context */
	enum rtlsdr_tuner					tuner_type;
	ITuner *							tuner;
	uint32_t							tun_xtal; /* Hz */
	uint32_t							freq; /* Hz */
	uint32_t							offs_freq; /* Hz */
	uint32_t							effective_freq; /* Hz */
	int									corr; /* ppm */
	int									gain; /* tenth dB */
//	struct e4k_state e4k_s;
//	struct r82xx_config r82xx_c;
//	struct r82xx_priv r82xx_p;
	/* status */
	int									dev_lost;
	int									driver_active;
	unsigned int						xfer_errors;
	ITuner*								tunerset[ RTLSDR_TUNER_R828D + 1 ];
	int									deviceOpened;

	static CDongleArray					Dongles;
	static time_t						lastCatalog;
	static bool							noCatalog;
	static bool							goodCatalog;

	int									tuner_initialized;
	int									i2c_repeater_on;
	int									spectrum_inversion;
};
