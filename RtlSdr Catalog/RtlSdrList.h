#pragma once
#include "..\libusb/libusb.h"
#include "Recognized Dongles.h"
#include "..\Common\MyMutex.h"

#include "SharedMemoryFile.h"

typedef BYTE usbpath_t[ MAX_USB_PATH ];

#define EEPROM_SIZE		256
typedef uint8_t eepromdata[ EEPROM_SIZE ];

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

class RtlSdrList
{
public:
					RtlSdrList				( void );
	virtual			~RtlSdrList				( void );

	bool			IsSharedMemoryActive	( void )
					{
						return Dongles != NULL && RtlSdrArea != NULL;
					}
	int				GetCount				( void );
	bool			GetStrings				( int index
											, CString& man
											, CString& prd
											, CString& ser
											);
	bool			IsBusy					( DWORD index );
	bool			IsFound					( DWORD index );
	void			GetCatalog				( void );
	void			SetMasterPresent		( bool set = true )
											{
												RtlSdrArea->MasterPresent = true;
											}
	int				GetMasterUpdate			( void )
											{
												BOOL update = RtlSdrArea->MasterUpdate;
												RtlSdrArea->MasterUpdate = false;
												return update;
											}
	bool			SharedMemExistedOnStart	( void )
					{
						//	You get one and only one valid query.
						if ( SharedDongleData.alreadyexisted )
						{
							SharedDongleData.alreadyexisted = false;
							return true;
						}
						return false;
					}
	void			RemoveDongle			( int index );

protected:

	void			ReadRegistry			( void );
	void			WriteRegistry			( void );
	uint32_t		WriteSingleRegistry		( int index );
	void			WriteRegLastCatalogTime	( void );
	void			reinitDongles			( void );
	const rtlsdr_dongle_t *
					find_known_device		( uint16_t vid
											, uint16_t pid
											);
	int				rtlsdr_open				( uint32_t devindex );
	void			ClearVars				( void );
	int				basic_open				( uint32_t devindex );
	int				claim_opened_device		( void );
	int				rtlsdr_close			( void );
	int				open_requested_device	( libusb_context *ctx
											, uint32_t index
											, libusb_device_handle **ldevh
											);
	void			rtlsdr_set_i2c_repeater	( int on );
	int				rtlsdr_demod_write_reg	( uint8_t page
											, uint16_t addr
											, uint16_t val
											, uint8_t len
											);
	uint16_t		rtlsdr_demod_read_reg	( uint8_t page
											, uint16_t addr
											, uint8_t len
											);
	void			rtlsdr_init_baseband	( void );
	int				rtlsdr_deinit_baseband	( void );
	uint16_t		rtlsdr_read_reg			( uint8_t block
											, uint16_t addr
											, uint8_t len
											);
	int				rtlsdr_write_reg		( uint8_t block
											, uint16_t addr
											, uint16_t val
											, uint8_t len
											);
	int				rtlsdr_read_array		( uint8_t block
											, uint16_t addr
											, uint8_t *data
											, uint8_t len
											);
	int				rtlsdr_write_array		( uint8_t block
											, uint16_t addr
											, uint8_t *data
											, uint8_t len
											);
	int				rtlsdr_i2c_write_reg	( uint8_t i2c_addr
											, uint8_t reg
											, uint8_t val
											);
	uint8_t			rtlsdr_i2c_read_reg		( uint8_t i2c_addr
											, uint8_t reg
											);
	void			rtlsdr_set_gpio_bit		( uint8_t gpio
											, int val
											);
	void			rtlsdr_set_gpio_output	( uint8_t gpio );
	int				GetDongleIndexFromDongle( const Dongle dongle );
	void			mergeToMaster			( Dongle& tempd );
	int				rtlsdr_read_eeprom_raw	( eepromdata& data );
	int				GetEepromStrings		( uint8_t *data
											, int datalen
											, char* manf
											, char* prod
											, char* sern
											);
	int				GetEepromString			( const uint8_t *data
											, int datalen		// Length to read.
											, int pos
											, char* string	// BETTER be 256 bytes long
											);
	bool			CompareSparseRegAndData	( Dongle*		dng
											, eepromdata&	data
											);
	int				srtlsdr_eep_img_from_Dongle
											( eepromdata&	dat
											, Dongle*		regentry
											);
	int				FindInMasterDB			( Dongle*	dng
											, bool		exact
											);
	int				FindGuessInMasterDB		( Dongle* dng );

	Dongle						m_dongle;
	time_t						lastCatalog;
	libusb_context *			ctx;
	struct libusb_device_handle *devh;
	enum rtlsdr_tuner			tuner_type;
	int							i2c_repeater_on;
	bool						goodCatalog;
	bool						dev_lost;

//	static RtlSdrAreaDef*		RtlSdrArea;
//	static Dongle*				Dongles;
	static CMyMutex				registry_mutex;
};

