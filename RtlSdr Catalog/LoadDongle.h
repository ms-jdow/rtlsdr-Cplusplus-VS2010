#pragma once
enum rtlsdr_tuner
{
	RTLSDR_TUNER_UNKNOWN = 0,
	RTLSDR_TUNER_E4000,
	RTLSDR_TUNER_FC0012,
	RTLSDR_TUNER_FC0013,
	RTLSDR_TUNER_FC2580,
	RTLSDR_TUNER_R820T,
	RTLSDR_TUNER_R828D
};

enum rtl_sdr_gain_mode
{
	GAIN_MODE_AGC,
	GAIN_MODE_MANUAL,
	GAIN_MODE_LINEARITY,
	GAIN_MODE_SENSITIVITY,
	GAIN_MODE_STAGES,
	GAIN_MODE_COUNT			// Count of gain modes. Not a real gain mode.
};

typedef BYTE uint8_t;
typedef WORD uint16_t;
typedef DWORD uint32_t;
typedef long int32_t;
typedef unsigned long long uint64_t;

typedef struct rtlsdr_dev rtlsdr_dev_t;
typedef void( *rtlsdr_read_async_cb_t )( unsigned char *buf, uint32_t len, void *ctx );

#include "IRtlSdr.h"

class rtlsdr_surrogate;

class LoadDongle
{
public:
	LoadDongle(void);
	virtual ~LoadDongle(void);
	bool		Loaded					( void ) { return m_hModule != NULL; }


protected:
	void			makeSafe();

	HMODULE				m_hModule;
	uint64_t			merror;

public:
	static IRtlSdr*		surr;

protected:

public:
	IRtlSdr* (*CreateRtlSdr) ( void );
	void (*DeleteRtlSdr) ( IRtlSdr* me );

public:		//	rtlsdr.dll entry points.
	uint32_t 	( *rtlsdr_get_device_count )( void );

	const char* ( *rtlsdr_get_device_name ) ( uint32_t index );

	/*!
	 * Get USB device strings.
	 *
	 * NOTE: The string arguments must provide space for up to 256 bytes.
	 *
	 * \param index the device index
	 * \param manufact manufacturer name, may be NULL
	 * \param product product name, may be NULL
	 * \param serial serial number, may be NULL
	 * \return 0 on success
	 */
	int		( *rtlsdr_get_device_usb_strings )
											( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											);
	int		( *rtlsdr_get_device_usb_cstrings )
											( uint32_t index
											, CString& manufact
											, CString& product
											, CString& serial
											);

	/*!
	 * Get device index by USB serial string descriptor.
	 *
	 * \param serial serial string of the device
	 * \return device index of first device where the name matched
	 * \return -1 if name is NULL
	 * \return -2 if no devices were found at all
	 * \return -3 if devices were found, but none with matching name
	 */
	int		( *rtlsdr_get_index_by_serial )	( const char *serial );

	int		( *rtlsdr_open )				( rtlsdr_dev_t **dev
											, uint32_t index
											);

	int		( *rtlsdr_close )				( rtlsdr_dev_t *dev );

	/* configuration functions */

	/*!
	 * Set crystal oscillator frequencies used for the RTL2832 and the tuner IC.
	 *
	 * Usually both ICs use the same clock. Changing the clock may make sense if
	 * you are applying an external clock to the tuner or to compensate the
	 * frequency (and samplerate) error caused by the original (cheap) crystal.
	 *
	 * NOTE: Call this function only if you fully understand the implications.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param rtl_freq frequency value used to clock the RTL2832 in Hz
	 * \param tuner_freq frequency value used to clock the tuner IC in Hz
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_xtal_freq )		( rtlsdr_dev_t *dev
											, uint32_t rtl_freq
											, uint32_t tuner_freq
											);

	/*!
	 * Get crystal oscillator frequencies used for the RTL2832 and the tuner IC.
	 *
	 * Usually both ICs use the same clock.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param rtl_freq frequency value used to clock the RTL2832 in Hz
	 * \param tuner_freq frequency value used to clock the tuner IC in Hz
	 * \return 0 on success
	 */
	int		( *rtlsdr_get_xtal_freq )		( rtlsdr_dev_t *dev
											, uint32_t *rtl_freq
											, uint32_t *tuner_freq
											);

	/*!
	 * Get USB device strings.
	 *
	 * NOTE: The string arguments must provide space for up to 256 bytes.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param manufact manufacturer name, may be NULL
	 * \param product product name, may be NULL
	 * \param serial serial number, may be NULL
	 * \return 0 on success
	 */
	int		( *rtlsdr_get_usb_strings )		( rtlsdr_dev_t *dev
											, char *manufact
											, char *product
											, char *serial
											);

	int		( *rtlsdr_get_usb_cstrings )	( rtlsdr_dev_t *dev
											, CString& manufact
											, CString& product
											, CString& serial
											);

	/*!
	 * Write the device EEPROM
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param data buffer of data to be written
	 * \param offset address where the data should be written
	 * \param len length of the data
	 * \return 0 on success
	 * \return -1 if device handle is invalid
	 * \return -2 if EEPROM size is exceeded
	 * \return -3 if no EEPROM was found
	 */

	int		( *rtlsdr_write_eeprom )		( rtlsdr_dev_t *dev
											, uint8_t *data
											, uint8_t offset
											, uint16_t len
											);

	/*!
	 * Read the device EEPROM
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param data buffer where the data should be written
	 * \param offset address where the data should be read from
	 * \param len length of the data
	 * \return 0 on success
	 * \return -1 if device handle is invalid
	 * \return -2 if EEPROM size is exceeded
	 * \return -3 if no EEPROM was found
	 */

	int		( *rtlsdr_read_eeprom )			( rtlsdr_dev_t *dev
											, uint8_t *data
											, uint8_t offset
											, uint16_t len
											);

	int		( *rtlsdr_set_center_freq )		( rtlsdr_dev_t *dev
											, uint32_t freq
											);

	/*!
	 * Get actual frequency the device is tuned to.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return 0 on error, frequency in Hz otherwise
	 */
	uint32_t	( *rtlsdr_get_center_freq )	( rtlsdr_dev_t *dev );

	/*!
	 * Set the frequency correction value for the device.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param ppm correction value in parts per million (ppm)
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_freq_correction )	( rtlsdr_dev_t *dev
											, int ppm
											);

	/*!
	 * Get actual frequency correction value of the device.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return correction value in parts per million (ppm)
	 */
	int		( *rtlsdr_get_freq_correction )	( rtlsdr_dev_t *dev );

	/*!
	 * Get the tuner type.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return RTLSDR_TUNER_UNKNOWN on error, tuner type otherwise
	 */
	enum rtlsdr_tuner
			( *rtlsdr_get_tuner_type )		( rtlsdr_dev_t *dev );

	/*!
	 * Get a list of gains supported by the tuner.
	 *
	 * NOTE: The gains argument must be preallocated by the caller. If NULL is
	 * being given instead, the number of available gain values will be returned.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param gains array of gain values. In tenths of a dB, 115 means 11.5 dB.
	 * \return <= 0 on error, number of available (returned) gain values otherwise
	 */
	int		( *rtlsdr_get_tuner_gains )		( rtlsdr_dev_t *dev
											, int *gains
											);

	/*!
	 * Set the gain for the device.
	 * Manual gain mode must be enabled for this to work.
	 *
	 * Valid gain values (in tenths of a dB) for the E4000 tuner:
	 * -10, 15, 40, 65, 90, 115, 140, 165, 190,
	 * 215, 240, 290, 340, 420, 430, 450, 470, 490
	 *
	 * Valid gain values may be queried with \ref rtlsdr_get_tuner_gains function.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param gain in tenths of a dB, 115 means 11.5 dB.
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_tuner_gain )		( rtlsdr_dev_t *dev
											, int gain
											);

	/*!
	 * Get actual gain the device is configured to.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return 0 on error, gain in tenths of a dB, 115 means 11.5 dB.
	 */
	int		( *rtlsdr_get_tuner_gain )		( rtlsdr_dev_t *dev );

	/*!
	 * Set the intermediate frequency gain for the device.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param stage intermediate frequency gain stage number (1 to 6 for E4000)
	 * \param gain in tenths of a dB, -30 means -3.0 dB.
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_tuner_if_gain )	( rtlsdr_dev_t *dev
											, int stage
											, int gain
											);

//  ADDED_STAGE_GAIN_MATERIAL
/*!
 * Get a list of gains and description of the gain stages supported by the tuner.
 * NOTE: The gains argument must be preallocated by the caller. If NULL is
 * being given instead, the number of available gain settings will be returned.
 *
 * \param dev the device handle given by rtlsdr_open()
 * \param stage the stage to get the array of gain settings. If no such
 *   stage exists, return error
 * \gains array to hold the different gain settings for this stage
 *   - use NULL to get the size of the array returned by the function
 * \param description the textual description of the respective stage
 *   is copied into this string (description max. 256 chars)
 *   Optional: can be NULL
 * \return <= 0 on error, number of available (returned) gain values otherwise
 * \def DESCRIPTION_MAXLEN description max. 256 chars
 */
#define DESCRIPTION_MAXLEN 256
	int		( *rtlsdr_get_tuner_stage_gains )
											( rtlsdr_dev_t *dev
											, uint8_t stage
											, int32_t *gains
											, char *description
											);

/*!
 * Get the number of stages for per stage gain control for this device.
 * \param dev the device handle given by rtlsdr_open()
 * \return <= 0 on error, number of available stages otherwise
 */
	int		( *rtlsdr_get_tuner_stage_count )
											( rtlsdr_dev_t *dev );

/*!`
 * Set the gain of a stage in the tuner
 *
 * \param dev the device handle given by rtlsdr_open()
 * \param stage the stage to set gain for
 * \param in tenths of a dB, e.g. -30 means -3.0 dB.
 * \return in tenths of a dB, e.g. -30 means -3.0 dB. (0dB if inappropriate)
 */
	int		( *rtlsdr_get_tuner_stage_gain )( rtlsdr_dev_t *dev
											, uint8_t stage
											);

/*!`
 * Set the gain of a stage in the tuner
 *
 * \param dev the device handle given by rtlsdr_open()
 * \param stage the stage to set gain for
 * \param in tenths of a dB, e.g. -30 means -3.0 dB.
 * \return <= 0 on error, 0 on success
 */
	int		( *rtlsdr_set_tuner_stage_gain )( rtlsdr_dev_t *dev
											, uint8_t stage
											, int32_t gain
											);
//  /ADDED_STAGE_GAIN_MATERIAL




	/*!
	 * Set the gain mode (automatic/manual) for the device.
	 * Manual gain mode must be enabled for the gain setter function to work.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param manual gain mode, 1 means manual gain mode shall be enabled.
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_tuner_gain_mode )	( rtlsdr_dev_t *dev
											, int manual
											);

	/*!
	 * Set the sample rate for the device, also selects the baseband filters
	 * according to the requested sample rate for tuners where this is possible.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param samp_rate the sample rate to be set, possible values are:
	 * 		    225001 - 300000 Hz
	 * 		    900001 - 3200000 Hz
	 * 		    sample loss is to be expected for rates > 2400000
	 * \return 0 on success, -EINVAL on invalid rate
	 */
	int		( *rtlsdr_set_sample_rate )		( rtlsdr_dev_t *dev
											, uint32_t rate
											);

	/*!
	 * Get actual sample rate the device is configured to.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return 0 on error, sample rate in Hz otherwise
	 */
	uint32_t	( *rtlsdr_get_sample_rate )	( rtlsdr_dev_t *dev );


	uint32_t	( *rtlsdr_get_corr_sample_rate )
											( rtlsdr_dev_t *dev );

	/*!
	 * Enable test mode that returns an 8 bit counter instead of the samples.
	 * The counter is generated inside the RTL2832.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param test mode, 1 means enabled, 0 disabled
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_testmode )		( rtlsdr_dev_t *dev
											, int on
											);

	/*!
	 * Enable or disable the internal digital AGC of the RTL2832.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param digital AGC mode, 1 means enabled, 0 disabled
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_agc_mode )		( rtlsdr_dev_t *dev
											, int on
											);

	/*!
	 * Enable or disable the direct sampling mode. When enabled, the IF mode
	 * of the RTL2832 is activated, and rtlsdr_set_center_freq() will control
	 * the IF-frequency of the DDC, which can be used to tune from 0 to 28.8 MHz
	 * (xtal frequency of the RTL2832).
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param on 0 means disabled, 1 I-ADC input enabled, 2 Q-ADC input enabled
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_direct_sampling )	( rtlsdr_dev_t *dev
											, int on
											);

	/*!
	 * Get state of the direct sampling mode
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return -1 on error, 0 means disabled, 1 I-ADC input enabled
	 *	    2 Q-ADC input enabled
	 */
	int		( *rtlsdr_get_direct_sampling )	( rtlsdr_dev_t *dev );

	/*!
	 * Enable or disable offset tuning for zero-IF tuners, which allows to avoid
	 * problems caused by the DC offset of the ADCs and 1/f noise.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param on 0 means disabled, 1 enabled
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_offset_tuning )	( rtlsdr_dev_t *dev
											, int on
											);

	/*!
	 * Get state of the offset tuning mode
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return -1 on error, 0 means disabled, 1 enabled
	 */
	int		( *rtlsdr_get_offset_tuning )	( rtlsdr_dev_t *dev );

	/*!
	 * Enable or disable frequency dithering for r820t tuners.
	 * Must be performed before freq_set().
	 * Fails for other tuners.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param on 0 means disabled, 1 enabled
	 * \return 0 on success
	 */
	int		( *rtlsdr_set_dithering )		( rtlsdr_dev_t *dev
											, int dither
											);

	/* streaming functions */

	int		( *rtlsdr_reset_buffer )		( rtlsdr_dev_t *dev );

	int		( *rtlsdr_read_sync )			( rtlsdr_dev_t *dev
											, void *buf
											, int len
											, int *n_read
											);

	/*!
	 * Read samples from the device asynchronously. This function will block until
	 * it is being canceled using rtlsdr_cancel_async()
	 *
	 * NOTE: This function is deprecated and is subject for removal.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param cb callback function to return received samples
	 * \param ctx user specific context to pass via the callback function
	 * \return 0 on success
	 */
	int		( *rtlsdr_wait_async )			( rtlsdr_dev_t *dev
											, rtlsdr_read_async_cb_t cb
											, void *ctx
											);

	/*!
	 * Read samples from the device asynchronously. This function will block until
	 * it is being canceled using rtlsdr_cancel_async()
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \param cb callback function to return received samples
	 * \param ctx user specific context to pass via the callback function
	 * \param buf_num optional buffer count, buf_num * buf_len = overall buffer size
	 *		  set to 0 for default buffer count (32)
	 * \param buf_len optional buffer length, must be multiple of 512,
	 *		  set to 0 for default buffer length (16 * 32 * 512)
	 * \return 0 on success
	 */
	int		( *rtlsdr_read_async )			( rtlsdr_dev_t *dev
											, rtlsdr_read_async_cb_t cb
											, void *ctx
											, uint32_t buf_num
											, uint32_t buf_len
											);

	/*!
	 * Cancel all pending asynchronous operations on the device.
	 *
	 * \param dev the device handle given by rtlsdr_open()
	 * \return 0 on success
	 */
	int		( *rtlsdr_cancel_async )		( rtlsdr_dev_t *dev);


	unsigned __int64 ( *rtlsdr_get_version_int64 )
											( void );
};

class rtlsdr_surrogate : public IRtlSdr
{
public:
	rtlsdr_surrogate( LoadDongle* in_LD );
	~rtlsdr_surrogate();

public:
	virtual
	int			rtlsdr_set_xtal_freq		( uint32_t rtl_freq
											, uint32_t tuner_freq
											);
	virtual
	int			rtlsdr_get_xtal_freq		( uint32_t *rtl_freq
											, uint32_t *tuner_freq
											);
	virtual
	int			rtlsdr_write_eeprom			( uint8_t *data
											, uint8_t offset
											, uint16_t len
											);
	virtual
	int			rtlsdr_read_eeprom			( uint8_t *data
											, uint8_t offset
											, uint16_t len
											);
	virtual
	int			rtlsdr_read_eeprom			( eepromdata& data );
	virtual
	int			rtlsdr_set_center_freq		( uint32_t in_freq );
	virtual
	uint32_t	rtlsdr_get_center_freq		( void );
	virtual
	int			rtlsdr_set_freq_correction	( int ppm );
	virtual
	int			rtlsdr_get_freq_correction
											( void );
	virtual
	int			rtlsdr_get_tuner_type		( void );
	virtual
	int			rtlsdr_get_tuner_gains		( int *gains );
	virtual
	int			rtlsdr_set_tuner_gain		( int gain );
	virtual
	int			rtlsdr_get_tuner_gain		( void );
	virtual
	int			rtlsdr_set_tuner_if_gain	( int stage
											, int gain
											);
	//  ADDED_STAGE_GAIN_MATERIAL
	virtual
	int			rtlsdr_get_tuner_stage_gains( uint8_t stage
											, int32_t *gains
											, char *description
											);
	virtual
	int			rtlsdr_get_tuner_stage_count( void );
	virtual
	int			rtlsdr_set_tuner_stage_gain	( uint8_t stage
											, int32_t gain
											);
	virtual
	int			rtlsdr_get_tuner_stage_gain	( uint8_t stage );
	//  ADDED_STAGE_GAIN_MATERIAL
	virtual
	int			rtlsdr_set_tuner_gain_mode	( int mode );
	virtual
	int			rtlsdr_set_sample_rate		( uint32_t samp_rate );
	virtual
	uint32_t	rtlsdr_get_sample_rate		( void );
	virtual
	uint32_t	rtlsdr_get_corr_sample_rate	( void );
	virtual
	int			rtlsdr_set_testmode			( int on );
	virtual
	int			rtlsdr_set_agc_mode			( int on );
	virtual
	int			rtlsdr_set_direct_sampling	( int on );
	virtual
	int			rtlsdr_get_direct_sampling
											( void );
	virtual
	int			rtlsdr_set_offset_tuning	( int on );
	virtual
	int			rtlsdr_get_offset_tuning	( void );
	virtual
	int			rtlsdr_set_dithering		( int dither );
	virtual
//	int			rtlsdr_get_tuner_type		( int index );
//	virtual
	int			rtlsdr_open					( uint32_t index );
	virtual
	int			rtlsdr_close				( void );
	virtual
	uint32_t	rtlsdr_get_tuner_clock		( void );
	virtual
	int			rtlsdr_get_usb_strings		( char *manufact
											, char *product
											, char *serial
											);
	virtual
	int			rtlsdr_get_usb_strings		( CString& manufact
											, CString& product
											, CString& serial
											);
	virtual
	int			rtlsdr_reset_buffer			( void );
	virtual
	int			rtlsdr_read_sync			( BYTE *buf
											, int len
											, int *n_read
											);
	virtual
	int			rtlsdr_wait_async			( rtlsdr_read_async_cb_t cb
											, void *ctx
											);
	virtual
	int			rtlsdr_read_async			( rtlsdr_read_async_cb_t in_cb
											, void *in_ctx
											, uint32_t buf_num
											, uint32_t buf_len
											);
	virtual
	int			rtlsdr_cancel_async			( void );
	virtual
	const char* rtlsdr_get_version			( void );
	virtual
	unsigned __int64 
				rtlsdr_get_version_int64	( void );

	// "static" functions
	virtual
	uint32_t	rtlsdr_get_device_count		( void );
	virtual
	const char *rtlsdr_get_device_name		( uint32_t index );
	virtual
	int			rtlsdr_get_device_usb_strings
											( uint32_t index
											, char *manufact
											, char *product
											, char *serial
											);
	virtual
	int			rtlsdr_get_device_usb_strings
											( uint32_t index
											, CString& manufact
											, CString& product
											, CString& serial
											);
	virtual
	int			rtlsdr_get_index_by_serial	( const char *serial );

protected:
	rtlsdr_dev_t *						dev;
	LoadDongle *						LD;
};
