#pragma once

#include "DongleArrays.h"

#define MAX_DONGLES		32

typedef struct
{
	char			validityKey[ 8 ];	//	'RTL-SDR'
	BOOL			MasterPresent;
	BOOL			MasterUpdate;
	ULONG			numEntries;			//	Total number of provisioned entries.
	ULONG			activeEntries;		//	Current number of active entries
	Dongle			dongleArray[ MAX_DONGLES ];	//	Payload
} RtlSdrAreaDef;

#define kValidityKey	"RTL-SDR"
#define kSharedAreaName	_T( "RtlSdr Shared Memory" )

class SharedMemoryFile
{
public:
					SharedMemoryFile		( void );
					~SharedMemoryFile		( void );

	bool			Init					( void );//* sysRef )
	bool			CreateSharedArea		( void );
	void			Close					( void );



	RtlSdrAreaDef*	sharedMem;
	bool			active;
	bool			alreadyexisted;

protected:
	HANDLE			sharedHandle;

};

extern RtlSdrAreaDef			TestDongles;
extern RtlSdrAreaDef*			RtlSdrArea;
extern Dongle*					Dongles;

extern SharedMemoryFile			SharedDongleData;
