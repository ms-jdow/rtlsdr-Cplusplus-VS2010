#include "StdAfx.h"
#include "SharedMemoryFile.h"
#include <Sddl.h>

#define NEED_MORE_INFO

RtlSdrAreaDef	TestDongles =
{
	{ 'R', 'T', 'L', '-', 'S', 'D', 'R', 0 },
	false, false,
	32, 0
};

SharedMemoryFile::SharedMemoryFile( void )
	: active( false )
	, sharedHandle( NULL )
	, sharedMem( NULL )
{
	Init();
}


SharedMemoryFile::~SharedMemoryFile( void )
{
	TRACE( "~SharedMemoryFile: 0x%x ~SharedMemoryFile with ( 0x%x, 0x%x )\n", this, sharedMem, sharedHandle );
	if ( sharedMem != NULL )				// have a shared area mapped?
		UnmapViewOfFile( sharedMem );		// unmap it
	sharedMem = NULL;						// and forget it
	if ( sharedHandle != NULL )				// have a shared area?
		CloseHandle( sharedHandle );		// close it
	sharedHandle = NULL;					// and forget it
}

bool SharedMemoryFile::Init( void )//* sysRef)
{
	TRACE("0x%x SharedMemoryFile::Init\n", this );

#if defined( NEED_MORE_INFO )
	TCHAR namebuf [256];

	if (!GetModuleFileName( NULL, namebuf, sizeof namebuf ))
		TRACE("0x%x SharedMemoryFile unable to get name of caslling process!\n", this );
	else
	{
		TRACE( _T( "0x%x SharedMemoryFile process %d (0x%X) is \"%s\"\n" ), this,
				   GetCurrentProcessId(), GetCurrentProcessId(), namebuf);
	}
#endif

	//	This is ASIOInit, except that we only get the sysRef part of the
	//	ASIODriverInfo structure. Note that we have to exist at the time this
	//	function is called, so previous magic of some sort has caused us to
	//	come into existance.
	//
	//	The spec does NOT define what to do here if we have already been 
	//	initialized, nor if we are currently running.  It might be reasonable
	//	to tear down any structure and start over from scratch.  But that is
	//	not what the sample driver does.  It just returns true.
	//
	//	NOTE that this is about the only ASIO function that does not return
	//	an ASIOError value!

	if ( active )								// already initialized?
		return true;							// just ignore this attempt!

	//	Creating the shared area will create the input and output structures
	//	and hook up the various pointers.

	if ( !CreateSharedArea())
		return false;

	active = true;						// we are open for business!

	//	Since we are returning true we do not need to set up the
	//	error message value as it will never be referenced.

	return true;
}



//------------------------------------------------------------------------------------------
//	Master setup function called from ASIOInit to set up for master operation.

bool SharedMemoryFile::CreateSharedArea( void )
{
	alreadyexisted = false;
	//	This is part of ASIOInit.  We are setting up to be the master.
	//
	//	Create the shared area, set up the basic information in the bottom of
	//	the area, and set up the channel structures. Do this all in an order
	//	that will prevent a user falling into a hole if he tries to open while
	//	we are doing the setup.

	TRACE( "0x%x->SharedMemoryFile CreateSharedArea\n", this );

	if ( sharedHandle != NULL || sharedMem != NULL )
	{
		TRACE( "0x%x Shared area already open!\n", this );
		return false;						// looks like we are already open!
	}

	//	First determine how big the shared area needs to be.

	int size = sizeof ( RtlSdrAreaDef );

	//	Now try to create the area.
	//	Return value
	//	If the function succeeds, the return value is a handle to the newly
	//	created file mapping object.

	//	If the object exists before the function call, the function returns a
	//	handle to the existing object (with its current size, not the specified
	//	size), and GetLastError returns ERROR_ALREADY_EXISTS. 

	HANDLE area = CreateFileMapping( INVALID_HANDLE_VALUE// use the page file for backing
								   , NULL				// default security attributes for now
								   , PAGE_READWRITE		// protections
								   | SEC_COMMIT			// commit the pages
								   , 0					// high DWORD of area size
								   , size				// low DWORD of area size
								   , kSharedAreaName	// the area file name
								   );
	if ( area == NULL )
	{
		TRACE( "0x%x Shared area Create failure! %d\n", this, GetLastError());
		return false;
	}

	if ( size != sizeof( RtlSdrAreaDef ))
	{
		TRACE( "0x%x Shared area existed wrong size.\n", this );
		CloseHandle( area );
		return false;
	}

	alreadyexisted = GetLastError() == ERROR_ALREADY_EXISTS;

	//	We have the area open, see about mapping a view of the file.

	LPVOID sma = MapViewOfFile( area		// shared area handle
							  , FILE_MAP_ALL_ACCESS
							  , 0			// base offset high DWORD
							  , 0			// base offset low DWORD
							  , 0			// map the entire file
							  );
	if ( sma == NULL )
	{
		CloseHandle( area );
		TRACE( "0x%x Shared area Mapping failure!\n", this );
		return false;
	}

	sharedMem = (RtlSdrAreaDef*) sma;
	sharedHandle = area;					// return sharead area handle too
	if ( alreadyexisted )
	{
		TRACE( "0x%x Shared area already existed\n", this );
		//	check validity
		if ( strcmp( sharedMem->validityKey, kValidityKey ) != 0 )
		{
			TRACE( "0x%x Shared memory found but not valid\n", this );
			//	Force valid for now.
			memset( sma, 0, sizeof( sma ));
			strcpy_s( sharedMem->validityKey, kValidityKey );
			sharedMem->numEntries = 32;
		}
	}
	else
	{
		TRACE( "0x%x New shared area\n", this );
		//	Initialize
		memset( sma, 0, sizeof( sma ));
		strcpy_s( sharedMem->validityKey, kValidityKey );
		sharedMem->numEntries = 32;
		sharedMem->MasterPresent = false;
		sharedMem->MasterUpdate = false;
	}

	//	TODOTODO Open/create the access MUTEX here.

	TRACE( "0x%x SharedMemoryFile area @ %.8X for %X (%d)\n", this, sharedMem, size, size );

	return true;							// the open worked
}


void SharedMemoryFile::Close( void )
{
}
