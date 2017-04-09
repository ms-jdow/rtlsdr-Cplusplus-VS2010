// librtlsdr_dongle_comms.cpp

//	This file is the basic rtlsdr low level dongle USB communications
//	portion of the rtlsdr class in the rtlsdr.dll project.
//


#include "StdAfx.h"
#include "libusb/libusb.h"
#include "librtlsdr.h"

enum usb_reg
{
	USB_SYSCTL		    = 0x2000,
	USB_CTRL		    = 0x2010,
	USB_STAT		    = 0x2014,
	USB_EPA_CFG		    = 0x2144,
	USB_EPA_CTL		    = 0x2148,
	USB_EPA_MAXPKT		= 0x2158,
	USB_EPA_MAXPKT_2	= 0x215a,
	USB_EPA_FIFO_CFG	= 0x2160,
};

enum sys_reg {
	DEMOD_CTL		= 0x3000,
	GPO			    = 0x3001,
	GPI			    = 0x3002,
	GPOE			= 0x3003,
	GPD			    = 0x3004,
	SYSINTE			= 0x3005,
	SYSINTS			= 0x3006,
	GP_CFG0			= 0x3007,
	GP_CFG1			= 0x3008,
	SYSINTE_1		= 0x3009,
	SYSINTS_1		= 0x300a,
	DEMOD_CTL_1		= 0x300b,
	IR_SUSPEND		= 0x300c,
};

enum blocks {
	DEMODB			= 0,
	USBB			= 1,
	SYSB			= 2,
	TUNB			= 3,
	ROMB			= 4,
	IRB			= 5,
	IICB			= 6,
};

#define CTRL_IN		(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT	(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)
#define CTRL_TIMEOUT	300

#define DEFAULT_BUF_NUMBER	15
#define DEFAULT_BUF_LENGTH	(16 * 32 * 512)

#define EEPROM_ADDR	0xa0

int rtlsdr::rtlsdr_read_array( uint8_t block
							   , uint16_t addr
							   , uint8_t *data
							   , uint8_t len
							   )
{
	if ( !devh )
		return -1;

	int r;
	uint16_t index = ( block << 8 );

	r = libusb_control_transfer( devh
							   , CTRL_IN
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );
#if 0
	if ( r < 0 )
	{
		fprintf(stderr, "%s failed with %d\n", __FUNCTION__, r);
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}
#endif
	return r;
}


int rtlsdr::rtlsdr_write_array( uint8_t block
							  , uint16_t addr
							  , uint8_t *data
							  , uint8_t len
							  )
{
	if ( !devh )
		return -1;

	int r;
	uint16_t index = ( block << 8 ) | 0x10;

	r = libusb_control_transfer( devh
							   , CTRL_OUT
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );
#if 0
	if ( r < 0 )
	{
		fprintf(stderr, "%s failed with %d\n", __FUNCTION__, r);
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}
#endif
	return r;
}


int rtlsdr::rtlsdr_i2c_write_reg( uint8_t i2c_addr
								, uint8_t reg
								, uint8_t val
								)
{
	if ( !devh )
		return -1;

	uint16_t addr = i2c_addr;
	uint8_t data[2];

	data[ 0 ] = reg;
	data[ 1 ] = val;
	return rtlsdr_write_array( IICB, addr, (uint8_t *) &data, 2 );
}


uint8_t rtlsdr::rtlsdr_i2c_read_reg( uint8_t i2c_addr, uint8_t reg )
{
	if ( !devh )
		return (uint8_t) -1;

	uint16_t addr = i2c_addr;
	uint8_t data = 0;

	rtlsdr_write_array( IICB, addr, &reg, 1 );
	rtlsdr_read_array( IICB, addr, &data, 1 );

	return data;
}


int rtlsdr::rtlsdr_i2c_write( uint8_t i2c_addr, uint8_t *buffer, int len )
{
	uint16_t addr = i2c_addr;

	if ( !devh )
		return -1;

	return rtlsdr_write_array(  IICB, addr, buffer, len );
}


int rtlsdr::rtlsdr_i2c_read( uint8_t i2c_addr, uint8_t *buffer, int len )
{
	uint16_t addr = i2c_addr;

	if ( !devh )
		return -1;

	return rtlsdr_read_array( IICB, addr, buffer, len );
}


uint16_t rtlsdr::rtlsdr_read_reg( uint8_t block, uint16_t addr, uint8_t len )
{
	int r;
	unsigned char data[ 2 ] = { 0, 0 };
	uint16_t index = ( block << 8 );
	uint16_t reg;

	r = libusb_control_transfer( devh
							   , CTRL_IN
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		fprintf(stderr, "%s failed with %d\n", __FUNCTION__, r);
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	reg = (data[1] << 8) | data[0];

	return reg;
}


int rtlsdr::rtlsdr_write_reg( uint8_t block
							, uint16_t addr
							, uint16_t val
							, uint8_t len
							)
{
	int r;
	unsigned char data[ 2 ];

	uint16_t index = ( block << 8 ) | 0x10;

	if ( len == 1 )
		data[ 0 ] = val & 0xff;
	else
		data[ 0 ] = val >> 8;

	data[ 1 ] = val & 0xff;

	r = libusb_control_transfer( devh
							   , CTRL_OUT
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		fprintf(stderr, "%s failed with %d\n", __FUNCTION__, r);
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	return r;
}


uint16_t rtlsdr::rtlsdr_demod_read_reg( uint8_t page
									  , uint16_t addr
									  , uint8_t len
									  )
{
	int r;
	unsigned char data[ 2 ];

	uint16_t index = page;
	uint16_t reg;
	addr = ( addr << 8 ) | 0x20;

	r = libusb_control_transfer( devh
							   , CTRL_IN
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		fprintf(stderr, "%s failed with %d\n", __FUNCTION__, r);
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	reg = ( data[ 1 ] << 8 ) | data[ 0 ];

	return reg;
}


int rtlsdr::rtlsdr_demod_write_reg( uint8_t page
								  , uint16_t addr
								  , uint16_t val
								  , uint8_t len
								  )
{
	if ( devh == NULL )
		return -1;

	int r;
	unsigned char data[ 2 ];
	uint16_t index = 0x10 | page;
	addr = ( addr << 8 ) | 0x20;

	if ( len == 1 )
		data[ 0 ] = val & 0xff;
	else
		data[ 0 ] = val >> 8;

	data[ 1 ] = val & 0xff;

	r = libusb_control_transfer( devh
							   , CTRL_OUT
							   , 0
							   , addr
							   , index
							   , data
							   , len
							   , CTRL_TIMEOUT
							   );

	if ( r < 0 )
	{
		fprintf(stderr, "%s failed with %d\n", __FUNCTION__, r);
		TRACE( "%s failed with %d\n", __FUNCTION__, r);
	}

	rtlsdr_demod_read_reg( 0x0a, 0x01, 1 );

	return ( r == len ) ? 0 : -1;
}


void rtlsdr::_rtlsdr_set_gpio_bit( uint8_t gpio, int val )
{
	uint16_t r;

	gpio = 1 << gpio;
	r = rtlsdr_read_reg( SYSB, GPO, 1 );
	r = val ?  ( r | gpio ) : ( r & ~gpio );
	rtlsdr_write_reg( SYSB, GPO, r, 1 );
}

int rtlsdr::_rtlsdr_get_gpio_bit( uint8_t gpio )
{
	gpio = 1 << gpio;
	uint8_t bit = (uint8_t) rtlsdr_read_reg( SYSB, GPO, 1 );
	bit &= gpio;
	return bit != 0;
}


void rtlsdr::rtlsdr_set_gpio_output( uint8_t gpio )
{
	int r;
	gpio = 1 << gpio;

	//	Set data value to 0?
	r = rtlsdr_read_reg( SYSB, GPD, 1 );
	r = rtlsdr_write_reg( SYSB, GPD, r & ~gpio, 1 ); // CARL: Changed from rtlsdr_write_reg(dev, SYSB, GPO, r & ~gpio, 1); must be a bug in the old code
	//	Now enable output.
	r = rtlsdr_read_reg( SYSB, GPOE, 1 );
	r = rtlsdr_write_reg( SYSB, GPOE, r | gpio, 1 );
}


void rtlsdr::rtlsdr_set_gpio_input( uint8_t gpio )
{
	int r;
	gpio = 1 << gpio;

	//	Set data value to 0?
	r = rtlsdr_read_reg( SYSB, GPD, 1 );
	rtlsdr_write_reg( SYSB, GPD, r & ~gpio, 1 ); // CARL: Changed from rtlsdr_write_reg(dev, SYSB, GPO, r & ~gpio, 1); must be a bug in the old code
	// Now make pin an input.
	r = rtlsdr_read_reg( SYSB, GPOE, 1 );
	rtlsdr_write_reg( SYSB, GPOE, r & ~gpio, 1 );
}


int rtlsdr::rtlsdr_get_gpio_output( uint8_t gpio )
{
	gpio = 1 << gpio;

	uint8_t bit = (uint8_t) rtlsdr_read_reg( SYSB, GPOE, 1 );
	bit &= gpio;

	return ( bit ) != 0;
}


void rtlsdr::rtlsdr_set_i2c_repeater( int on )
{
	if ( on == i2c_repeater_on )
		return;
	if ( on == -1 )
		on = 0;
	else
		on = !!on; /* values +2 to force on */
	i2c_repeater_on = on;
	int ret = rtlsdr_demod_write_reg( 1, 0x01, on ? 0x18 : 0x10, 1 );
	ASSERT( ret == 0 );
}


int rtlsdr::rtlsdr_i2c_write_fn( uint8_t addr, uint8_t *buf, int len )
{
	int r;
	int retries = 2;
	if ( !devh )
		return -1;
	do
	{
		r = rtlsdr_i2c_write( addr, buf, len );
		if ( r >= 0 )
		    return r;
		rtlsdr_set_i2c_repeater( 2 );
		retries--;
	}
	while ( retries > 0 );

	return -1;
}


int rtlsdr::rtlsdr_i2c_read_fn( uint8_t addr, uint8_t *buf, int len )
{
	int r;
	int retries = 2;
	if ( !devh )
		return -1;
	do
	{
		r = rtlsdr_i2c_read( addr, buf, len );
		if (r >= 0)
		    return r;
		rtlsdr_set_i2c_repeater( 2 );
		retries--;
	} while (retries > 0);
	return -1;
}


//	Note about formatting for writing to the EEPROM. It seems that
//	the rtl_eeprom.exe file writes a zero to the byte at the index
//	78, 0x4e. This is the "length of ir config". It coincidentally
//	is zero already when using Unicoded ASCII.
//	This observation came from me musing about adding more strings
//	to the string portion of the eeprom for whatever reasons we
//	might find worthwhile.
int rtlsdr::rtlsdr_write_eeprom_raw( eepromdata& data )
{
	if ( !devh )
		return -1;

	int r = 0;
	int i;
	int len = sizeof( eepromdata );

	uint8_t cmd[ 2 ];

	rtlsdr_set_i2c_repeater( 0 );

	for ( i = 0; i < len; i++ )
	{
		cmd[ 0 ] = i;
		r = rtlsdr_write_array( IICB, EEPROM_ADDR, cmd, 1 );
		r = rtlsdr_read_array( IICB, EEPROM_ADDR, &cmd[ 1 ], 1 );

		/* only write the byte if it differs */
		if (cmd[ 1 ] == data[ i ])
			continue;

		cmd[ 1 ] = data[ i ];
		r = rtlsdr_write_array( IICB, EEPROM_ADDR, cmd, 2 );
		if ( r != sizeof( cmd ))
			return -3;

		/* for some EEPROMs (e.g. ATC 240LC02) we need a delay
		 * between write operations, otherwise they will fail */
#ifdef _WIN32
		Sleep( 5 );
#else
		usleep(5000);
#endif
	}

	return 0;
}


#define EEPROM_READ_SIZE 1		//	Maximum safe read it appears.

int rtlsdr::rtlsdr_read_eeprom_raw( eepromdata& data )
{
	int r = 0;
	int i;
	int len;
	uint8_t offset = 0;

	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( -1 );

	r = rtlsdr_write_array(  IICB, EEPROM_ADDR, &offset, 1 );
	if ( r < 0 )
		return -3;

#if 1
	len = sizeof( eepromdata );;
	for ( i = 0; i < len; i++ )
	{
		r = rtlsdr_read_array( IICB, EEPROM_ADDR, data + i, 1 );

		if ( r < 0 )
			return -3;
	}
#else
	//	Let's perform some larger reads. 16 is safe. 32 is not, 17 is not.
	//	16 appears to work on all the dongles I have.
	//	16 quite fast compared to 1.
	int readsize = EEPROM_READ_SIZE;
	len = sizeof( eepromdata );;
	memset( data, 0xff, len );
	for ( i = 0; i < len; i += EEPROM_READ_SIZE )
	{
		r = rtlsdr_read_array( IICB
							 , readsize
							 , data + i
							 , (( len - i ) > readsize )
							  ? readsize
							  : len
							 );
		if ( r < 0 )
		{
			r = r;
			break;
		}
		len -= r;
	}
#endif

	return r;
}


int rtlsdr::rtlsdr_reset_buffer( void )
{
	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	rtlsdr_write_reg( USBB, USB_EPA_CTL, 0x1002, 2 );
	rtlsdr_write_reg( USBB, USB_EPA_CTL, 0x0000, 2 );

	return 0;
}


int rtlsdr::rtlsdr_read_sync( BYTE *buf, int len, int *n_read)
{
	if (!devh)
		return -1;

	return libusb_bulk_transfer( devh, 0x81, buf, len, n_read, BULK_TIMEOUT );
}


static void LIBUSB_CALL _libusb_callback( struct libusb_transfer *xfer )
{
	rtlsdr *dev = (rtlsdr *)xfer->user_data;
	dev->libusb_callback( xfer );
	
}


void rtlsdr::libusb_callback( struct libusb_transfer *xfer )
{
	if ( LIBUSB_TRANSFER_COMPLETED == xfer->status )
	{
		if ( cb )
			 cb( xfer->buffer, xfer->actual_length, cb_ctx );

		libusb_submit_transfer( xfer ); /* resubmit transfer */
		xfer_errors = 0;
	}
	else
	if ( LIBUSB_TRANSFER_CANCELLED != xfer->status )
	{
#ifndef _WIN32
		if (LIBUSB_TRANSFER_ERROR == xfer->status)
			dev->xfer_errors++;

		if (dev->xfer_errors >= dev->xfer_buf_num ||
		    LIBUSB_TRANSFER_NO_DEVICE == xfer->status) {
#endif
			dev_lost = 1;
			rtlsdr_cancel_async();
			fprintf(stderr, "cb transfer status: %d, "
							"canceling...\n", xfer->status);
			TRACE(  "cb transfer status: %d, canceling...\n", xfer->status);
#ifndef _WIN32
		}
#endif
	}
}


int rtlsdr::rtlsdr_wait_async( rtlsdr_read_async_cb_t cb, void *ctx )
{
	return rtlsdr_read_async( cb, ctx, 0, 0 );
}


int rtlsdr::rtlsdr_alloc_async_buffers( void )
{
	int i;
	int j;

	if ( !devh )
		return -1;

	//	Start clean
	rtlsdr_free_async_buffers();

	//	Don't reallocate them....
	if ( xfer == NULL )
	{
		xfer = (struct libusb_transfer**) malloc( xfer_buf_num
												* sizeof(struct libusb_transfer *)
												);
		if ( xfer == NULL )
			return -1;

		for( i = 0; i < (int) xfer_buf_num; ++i )
		{
			xfer[ i ] = libusb_alloc_transfer( 0 );
			if ( xfer[ i ] == NULL )
				goto unwind;
		}
	}

	if ( xfer_buf == NULL )
	{
		xfer_buf = (BYTE**) malloc( xfer_buf_num * sizeof(unsigned char *));
		if ( xfer_buf == NULL )
			goto unwind;

		for( j = 0; j < (int) xfer_buf_num; ++j )
		{
			xfer_buf[ j ] = (BYTE*) malloc( xfer_buf_len );
			if ( xfer_buf[ j ] == NULL )
				goto unwind;
		}
	}

	return 0;

unwind:
	rtlsdr_free_async_buffers();

	return -1;
}


int rtlsdr::rtlsdr_free_async_buffers( void )
{
	unsigned int i;

	if ( !devh )
		return -1;

	if ( xfer != NULL )
	{
		for( i = 0; i < xfer_buf_num; ++i )
		{
			if ( xfer[ i ] != NULL )
				libusb_free_transfer( xfer[ i ]);
			xfer[ i ] = NULL;
		}

		free( xfer );
		 xfer = NULL;
	}

	if ( xfer_buf != NULL )
	{
		for( i = 0; i < xfer_buf_num; ++i )
		{
			if ( xfer_buf[ i ] != NULL )
				free( xfer_buf[ i ]);
			xfer_buf[ i ] = NULL;
		}

		free( xfer_buf );
		xfer_buf = NULL;
	}

	return 0;
}


int rtlsdr::rtlsdr_read_async( rtlsdr_read_async_cb_t in_cb
							 , void *in_ctx
							 , uint32_t buf_num
							 , uint32_t buf_len
							 )
{
	unsigned int i;
	int r = 0;
	struct timeval tv = { 1, 0 };
	struct timeval zerotv = { 0, 0 };
	enum rtlsdr_async_status next_status = RTLSDR_INACTIVE;

	if ( !devh )
		return -1;

	if ( RTLSDR_INACTIVE != async_status )
		return -2;

	async_status = RTLSDR_RUNNING;
	async_cancel = 0;

	cb = in_cb;
	cb_ctx = in_ctx;

	//	Start clean (before we potentially change
	//	the number of buffers or their length.)
	rtlsdr_free_async_buffers();

	if ( buf_num > 0 )
		xfer_buf_num = buf_num;
	else
		xfer_buf_num = DEFAULT_BUF_NUMBER;

	if (( buf_len > 0 )
	&&	( buf_len % 512 == 0 )) /* len must be multiple of 512 */
		xfer_buf_len = buf_len;
	else
		xfer_buf_len = DEFAULT_BUF_LENGTH;

	r = rtlsdr_alloc_async_buffers();
	if ( r < 0 )
	{
		fprintf(stderr, "Failed to allocate buffers!\n");
		TRACE( "Failed to allocate buffers!\n");
		async_status = RTLSDR_CANCELING;
		return r;
	}

	for( i = 0; i < xfer_buf_num; ++i )
	{
		/* Sleep a little here. 
		   Will avoid crash in case there are many small buffers. */
		// jd = I don't believe it but this costs little.
#ifdef _WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
		libusb_fill_bulk_transfer( xfer[ i ]
								 , devh
								 , 0x81
								 , xfer_buf[ i ]
								 , xfer_buf_len
								 , _libusb_callback
								 , (void *) this
								 , BULK_TIMEOUT
								 );

		r = libusb_submit_transfer( xfer[ i ]);
		if ( r < 0 )
		{
			fprintf(stderr, "Failed to submit transfer %i error %d!\n", i, r);
			TRACE( "Failed to submit transfer %i error %d!\n", i, r);
			async_status = RTLSDR_CANCELING;
			break;
		}
	}

	while ( RTLSDR_INACTIVE != async_status )
	{
		r = libusb_handle_events_timeout_completed( ctx, &tv, &async_cancel );
		if ( r < 0 )
		{
			/*fprintf(stderr, "handle_events returned: %d\n", r);*/
			/*TRACE( "handle_events returned: %d\n", r);*/
			if (( r == LIBUSB_ERROR_INTERRUPTED ) || ( r == -1 )) /* stray signal */
				continue;
			break;
		}

		if ( RTLSDR_CANCELING == async_status )
		{
			next_status = RTLSDR_INACTIVE;

			if ( xfer == NULL )
				break;

			for( i = 0; i < xfer_buf_num; ++i )
			{
				if ( xfer[ i ] == NULL )
					continue;

				if ( LIBUSB_TRANSFER_CANCELLED != xfer[ i ]->status )
				{
					r = libusb_cancel_transfer( xfer[ i ]);
					/* handle events after canceling
					 * to allow transfer status to
					 * propagate */
					libusb_handle_events_timeout_completed( ctx, &zerotv, NULL);
					if ( r < 0 )
						continue;

					next_status = RTLSDR_CANCELING;
				}
			}

			if (( dev_lost )
			||	( RTLSDR_INACTIVE == next_status )) 
			{
				/* handle any events that still need to
				 * be handled before exiting after we
				 * just cancelled all transfers */
				libusb_handle_events_timeout_completed( ctx, &zerotv, NULL );
				break;
			}
		}
	}

	rtlsdr_free_async_buffers();

	async_status = next_status;

	return r;
}


int rtlsdr::rtlsdr_cancel_async( void )
{
	if ( !devh )
		return -1;

	/* if streaming, try to cancel gracefully */
	if ( RTLSDR_RUNNING == async_status )
	{
		async_status = RTLSDR_CANCELING;
		async_cancel = 1;
		return 0;
	}

	/* if called while in pending state, change the state forcefully */
#if 0
	if (RTLSDR_INACTIVE != dev->async_status) {
		dev->async_status = RTLSDR_INACTIVE;
		return 0;
	}
#endif
	return -2;
}


void rtlsdr::rtlsdr_init_baseband( void )
{
	unsigned int i;

	/* initialize USB */
	rtlsdr_write_reg( USBB, USB_SYSCTL, 0x09, 1 );
	rtlsdr_write_reg( USBB, USB_EPA_MAXPKT, 0x0002, 2 );
	rtlsdr_write_reg( USBB, USB_EPA_CTL, 0x1002, 2 );

	/* poweron demod */
	rtlsdr_write_reg( SYSB, DEMOD_CTL_1, 0x22, 1 );
	rtlsdr_write_reg( SYSB, DEMOD_CTL, 0xe8, 1 );

	/* reset demod (bit 3, soft_rst) */
	rtlsdr_demod_write_reg( 1, 0x01, 0x14, 1 );
	rtlsdr_demod_write_reg( 1, 0x01, 0x10, 1 );

	/* disable spectrum inversion and adjacent channel rejection */
	rtlsdr_demod_write_reg( 1, 0x15, 0x00, 1 );
//	spectrum_inversion = 0;		Performed in ClearVars.
	rtlsdr_demod_write_reg( 1, 0x16, 0x0000, 2 );

	/* clear both DDC shift and IF frequency registers  */
	for ( i = 0; i < 6; i++ )
		rtlsdr_demod_write_reg( 1, 0x16 + i, 0x00, 1 );

	rtlsdr_set_fir();

	/* enable SDR mode, disable DAGC (bit 5) */
	/* Also Disable test mode */
	rtlsdr_demod_write_reg( 0, 0x19, 0x05, 1 );

	/* init FSM state-holding register */
	rtlsdr_demod_write_reg( 1, 0x93, 0xf0, 1 );
	rtlsdr_demod_write_reg( 1, 0x94, 0x0f, 1 );

	/* disable AGC (en_dagc, bit 0) (this seems to have no effect) */
	rtlsdr_demod_write_reg( 1, 0x11, 0x00, 1 );

	/* disable RF and IF AGC loop */
	rtlsdr_demod_write_reg( 1, 0x04, 0x00, 1 );

	/* disable PID filter (enable_PID = 0) */
	rtlsdr_demod_write_reg( 0, 0x61, 0x60, 1 );

	/* opt_adc_iq = 0, default ADC_I/ADC_Q datapath */
	rtlsdr_demod_write_reg( 0, 0x06, 0x80, 1 );

	/* Enable Zero-IF mode (en_bbin bit), DC cancellation (en_dc_est),
	 * IQ estimation/compensation (en_iq_comp, en_iq_est) */
	rtlsdr_demod_write_reg( 1, 0xb1, 0x1b, 1 );

	/* disable 4.096 MHz clock output on pin TP_CK0 */
	rtlsdr_demod_write_reg( 0, 0x0d, 0x83, 1 );
}


int rtlsdr::rtlsdr_deinit_baseband( void )
{
	int r = 0;

	if ( !devh )
		return -1;

	rtlsdr_set_i2c_repeater( 0 );

	if ( tuner != NULL )
	{
		rtlsdr_set_i2c_repeater( 1 );
		r = tuner->exit();			/* deinitialize tuner */
		rtlsdr_set_i2c_repeater( 0 );
	}

	/* poweroff demodulator and ADCs */
	rtlsdr_write_reg( SYSB, DEMOD_CTL, 0x20, 1 );

	return r;
}


bool rtlsdr::dummy_write( void )
{
	return  rtlsdr_write_reg( USBB, USB_SYSCTL, 0x09, 1 ) < 0;
}


//	Static // Compare sparse eeprom image with image built from Dongle entry.
bool rtlsdr::CompareSparseRegAndData( Dongle*		dng
									, eepromdata&	data
									)
{
	eepromdata dngd;
	int err = srtlsdr_eep_img_from_Dongle( dngd, dng );
	if ( err < 0 )
		return 0;

	int r = 0;
	int i;
	uint8_t pos = 0;

	for( i = 0; i < 6; i++ )
	{
		if ( data[ i ] != dngd[ i ])
			return false;
	}

	pos = STR_OFFSET;
	//	Match manf string's size and type
	if ( memcmp( &data[ pos ], &dngd[ pos ], data[ pos ] ) != 0 )
		return false;
	pos += data[ pos ];

	//	Match last char of manf string and prod string size and type
	if ( memcmp( &data[ pos ], &dngd[ pos ], data[ pos ] ) != 0 )
		return false;
	pos += data[ pos ];

	//	Match last char of prod string and sern string size and type
	if ( memcmp( &data[ pos ], &dngd[ pos ], data[ pos ] ) != 0 )
		return false;

	return true;
}
