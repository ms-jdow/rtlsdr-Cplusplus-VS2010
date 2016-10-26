#pragma once

//	NB. This is meant to be included in librtlsdr.h as part of the
//	rtlsdr class. It's broken out this way for over all clarity.

typedef uint8_t usbportnums[ MAX_USB_PATH ];

protected:	//	Static functions		Dongle array handling
	STATIC void	FillDongleArraysAndComboBox
										( CDongleArray * da
										, CComboBox * combo
										, LPCTSTR selected
										);

	STATIC bool	GetDongleIdString		( CString& string
										, int devindex
										);
	STATIC int	FindDongleByIdString	( LPCTSTR source );

protected:	//	Static functions		Registry handling
	STATIC void	WriteRegistry			( void );
	STATIC uint32_t WriteSingleRegistry	( int index );
	STATIC void WriteRegLastCatalogTime	( void );
	STATIC void	ReadRegistry			( void );
	STATIC void mergeToMaster			( Dongle& tempd );
	STATIC const rtlsdr_dongle_t *
				find_known_device		( uint16_t vid
										, uint16_t pid
										);
	STATIC const char*
				find_requested_dongle_name
										( libusb_context *ctx
										, uint32_t index
										);
	STATIC int	open_requested_device	( libusb_context *ctx
										, uint32_t index
										, libusb_device_handle **ldevh
										, bool devindex
										);

	STATIC int	GetDongleIndexFromNames	( const char * manufact
										, const char * product
										, const char * serial
										);
	STATIC int	GetDongleIndexFromNames	( const CString & manufact
										, const CString & product
										, const CString & serial
										);
	STATIC int	GetDongleIndexFromDongle( const Dongle dongle );


protected:	//	Class functions
	int			GetEepromStrings		( const eepromdata& data
										, CString* manf
										, CString* prod
										, CString* sern
										);
	int			GetEepromStrings		( uint8_t *data
										, int datalen
										, char* manf
										, char* prod
										, char* sern
										);
	int			FullEepromParse			( const eepromdata& data
										, Dongle& tdongle
										);
	int			SafeFindDongle			( const Dongle& tdongle );
	STATIC int	FindInMasterDB			( Dongle*	dng
										, bool		exact
										);
	STATIC int	FindGuessInMasterDB		( Dongle* dng );


private:
	int			GetEepromString			( const eepromdata& data
										, int pos
										, CString* string
										);
	int			GetEepromString			( const uint8_t *data
										, int datalen
										, int pos
										, char* string
										);

protected:	//	Static					Registry/Dongle array
	static time_t						lastCatalog;
	static bool							noCatalog;
	static bool							goodCatalog;

