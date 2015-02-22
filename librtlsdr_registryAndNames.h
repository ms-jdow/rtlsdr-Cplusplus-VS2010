#pragma once

//	NB. This is meant to be included in librtlsdr.h as part of the
//	rtlsdr class. It's broken out this way for over all clarity.

#define MAX_STR_SIZE	256

typedef uint8_t usbportnums[ MAX_USB_PATH ];

protected:	//	Static functions		Dongle array handling
	static void	FillDongleArraysAndComboBox
										( CDongleArray * da
										, CComboBox * combo
										, LPCTSTR selected
										);

	static bool	GetDongleIdString		( CString& string
										, int devindex
										);
	static int	FindDongleByIdString	( LPCTSTR source );

protected:	//	Static functions		Registry handling
	static void	WriteRegistry			( void );
	static uint32_t WriteSingleRegistry	( int index );
	static void WriteRegLastCatalogTime	( void );
	static void	ReadRegistry			( void );
	static void mergeToMaster			( Dongle& tempd
										, int index
										);
	static const rtlsdr_dongle_t *
				find_known_device		( uint16_t vid
										, uint16_t pid
										);
	static int	GetDongleIndexFromNames	( const char * manufact
										, const char * product
										, const char * serial
										);
	static int	GetDongleIndexFromNames	( const CString & manufact
										, const CString & product
										, const CString & serial
										);
	static int	GetDongleIndexFromDongle( const Dongle dongle );


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
	static CDongleArray					Dongles;
	static time_t						lastCatalog;
	static bool							noCatalog;
	static bool							goodCatalog;

