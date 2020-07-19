#pragma once

//	NB. This is meant to be included in librtlsdr.h as part of the
//	rtlsdr class. It's broken out this way for over all clarity.

enum rtlsdr_async_status
{
	RTLSDR_INACTIVE = 0,
	RTLSDR_CANCELING,
	RTLSDR_RUNNING
};

#define BULK_TIMEOUT	0

typedef void(*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

#if 0
class rtlsdr_dongle_comms
{
public:
	rtlsdr_dongle_comms(void);
	~rtlsdr_dongle_comms(void);
#endif

public:		//	Basic comms
	libusb_device_handle*
				GetDeviceHandle			( void )	{ return devh; }

protected:	//	Basic comms
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
	void		_rtlsdr_set_gpio_bit	( uint8_t gpio
										, int val
										);
	int			_rtlsdr_get_gpio_bit	( uint8_t gpio );
	void		rtlsdr_set_gpio_output	( uint8_t gpio );
	void		rtlsdr_set_gpio_input	( uint8_t gpio );
	int			rtlsdr_get_gpio_output	( uint8_t gpio );
	void		rtlsdr_set_i2c_repeater	( int on );
	int			rtlsdr_i2c_write_fn		( uint8_t addr
										, uint8_t *buf
										, int len
										);
	int			rtlsdr_i2c_read_fn		( uint8_t addr
										, uint8_t *buf
										, int len
										);
	int			rtlsdr_write_eeprom_raw	( eepromdata& data );
	int			rtlsdr_read_eeprom_raw	( eepromdata& data );

public:
	int			GetOpenedDeviceIndex	( void )
										{
											return GetDongleIndexFromDongle( m_dongle );
										}

public:		//	Read and write bulk transfers
	int			rtlsdr_reset_buffer		( void );
	int			rtlsdr_read_sync		( BYTE *buf
										, int len
										, int *n_read
										);
	int			rtlsdr_wait_async		( rtlsdr_read_async_cb_t cb
										, void *ctx
										);
	int			rtlsdr_read_async		( rtlsdr_read_async_cb_t in_cb
										, void *in_ctx
										, uint32_t buf_num
										, uint32_t buf_len
										);
	int			rtlsdr_cancel_async		( void );

protected:	//	Read and write bulk transfers
	void		rtlsdr_init_baseband	( void );
	int			rtlsdr_deinit_baseband	( void );
	int			rtlsdr_alloc_async_buffers
										( void );
	int			rtlsdr_free_async_buffers
										( void );

protected:	// Support functions
	bool		dummy_write				( void );
	STATIC bool	CompareSparseRegAndData	( Dongle*		dng
										, eepromdata&	data
										);

public:		//	Read and write bulk transfers callback
	void LIBUSB_CALL
				libusb_callback			( struct libusb_transfer *xfer );

protected:	//	Basic variables.
	Dongle								m_dongle;
	libusb_context *					ctx;
	struct libusb_device_handle *		devh;
	int									i2c_repeater_on;
	uint32_t							xfer_buf_num;
	uint32_t							xfer_buf_len;
	struct libusb_transfer **			xfer;
	unsigned char **					xfer_buf;
	rtlsdr_read_async_cb_t				cb;
	void *								cb_ctx;
	enum rtlsdr_async_status			async_status;
	int									async_cancel;
	unsigned int						xfer_errors;
	int									dev_lost;

//	static	CMyMutex					registry_mutex;	//	Protect registry accesses
//	static	CMyMutex					dongle_mutex;	//	Protect shared memory accesses
	static	CMyMutex					rtlsdr_mutex;	//	Protect shared memory accesses
