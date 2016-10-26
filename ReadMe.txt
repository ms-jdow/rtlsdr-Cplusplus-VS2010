========================================================================
    DYNAMIC LINK LIBRARY : rtsdr Project Overview
========================================================================

AppWizard has created this rtsdr DLL for you.

This file contains a summary of what you will find in each of the files that
make up your rtsdr application.


rtsdr.vcxproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

rtsdr.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

rtsdr.cpp
    This is the main DLL source file.

	When created, this DLL does not export any symbols. As a result, it
	will not produce a .lib file when it is built. If you wish this project
	to be a project dependency of some other project, you will either need to
	add code to export some symbols from the DLL so that an export library
	will be produced, or you can set the Ignore Input Library property to Yes
	on the General propert page of the Linker folder in the project's Property
	Pages dialog box.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named rtsdr.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////


Let's rethink loading dongles. Presume this is the only tool that changes dongle
names. So we record the changed data in the registry and go back to work. If we
always load from our cache we're quick and the cache is up to date.

That means we will gave to "recover" if a dongle is manipulated by some other
tool. Perhaps we can insert a "tag" byte to identify this event. No, that still
requires a full EEPROM reading.

Hm, we could read the WinUSB.sys cache and compare with our dongle at the same
USB address. If they are different perform the full read and work and update
the registry from that data.

Let's rethink again. It is not particularly burdensome to read five or 10
individual addresses in the EEPROM area. So let's start with that concept and
not read the whole thing to see if a dongle has changed. Just make it a high
probability thing.

Let's figure out important scenarios.
Normal operation            Nothing changes so we should be fast.
Dongle renamed              The registry will be right the WinUSB cache wrong.
Dongle's path changed.      That means WinUSB cache will be correct.
Dongle renamed elsewhere    Then WinUSB will be right and registry wrong.
Dongle renamed other tool   Then nothing is correct.

New strategy. Read the linusb reported names. Read the registry reported names.
1   Read the registry reported data.
2   Reconcile the two to the extent possible opening the dongles to see if they
    are there.
3   On the remaining dongles work harder to reconcile - mark registry entries
    "not there" and add new ones.

We do NOT catch a local rename with a different EEPROM diddle tool. Can this be
economically tested for items that are present and agree with the registry? We
can try to open them. We may be able to make "good guesses" based on random
tests on the EEPROM....

Reconcile
a   Names, tunertype, & path all the same.                  What's to reconcile.
b   Names & tunertype all the same regardless of path.      Likely good.
c   tunertype & path the same.                              Likely rename. Ask?
d   Names & path    - retry once                            No match
e   path                                                    no match - shuffled

a   Data is good
b   Log new path - delete old path
c   Ask then do as appropriate
d   This is a new dongle on an old path suggest name change.
e   This is a new dongle on an old path.



EEPROM layout:
0 1         0x28 0x32       Device type
2 3         0xda 0x0b       0x0bda  VID
4 5         0x32 0x28       0x2832  PID
dat[6] == 0xa5 means have_serial
dat[7] & 0x01 means remote wakeup
dat[7] & 0x02 means enable IR
dat[8] == 0x02 ? just preserve it

dat[9]              Offset to first string
String format:
data[pos] = size
data[pos+1] = 3     Comstant
data[pos+2] = start of string
data[pos+size] = next string location.
//  We could squeeze in an additional string if needed.


Currently - get dongle count and other entry points get to GetCatalog.
    GetCatalog reads registry and if it is stale reinits all the dongles.
        reinitdongles:
            gets libusb device list.
            Checks each libusb device against registry.
                if a match is found loop to next device
                fill in some basic data such as we can.
                Compare path - update it if appropriate.
                if we can open the device get eeprom level strings. (Slow)
                merge into master list.
                Write the registry
            .
        .
    .

    Getstrings
        GetCatalog
        drag in registry data such as we find
        if we found it in registry we're done
        else blow lots of time reading the eeproms.

So we are mostly "there". GetStrings needs some fiddling.

Let's add a "goodness" test up front. Libusb==registry, almost ==, not ==.

If only path is different ignore the differences. If no match we first sample
the data and check EEPROM where characters differ....
If everything agrees then sample first 8 bytes, string lengths, and last 2 bytes
of strings. (Actually bytes 0-5 and 9 are fixed values that matter.)

So we need a new function to read single EEPROM bytes. This is a step back on
eeprom reading but it can speed up operations.

We also need a function to create a skeleton EEPROM image such as we'd write so
we can make proper comparisons.

Note that if I cannot read a dongle I simply "punt it".


D:\Projects\VS2010\sdrsharp SVN copy - Virgin\trunk\Debug\SDRSharp.exe

D:\Projects\VS2010\sdrsharp SVN copy - Virgin\trunk\Debug


Have the sparse eeprom read and the sparse compare test to use in reinit. Now
I need to fold it in.
D:\Downloads\SDR\sdrdev\SDRDEV\RTLSDR Work\sdrsharp.exe

Hm, Dongle == can fail if strings match, pid's match, but it is a duplicate on
a different port path. But if I test port path it fails if I move the dongle.
How can I solve this? I think I will have to split things out to save a dongles
array for the most recent reinit and compare with the registry path. (And I may
have to include the port path in some other identifying information that doctors
names sent out. `1234567` or `41` or whatever appended to the name somewhere.)

find_requested_dongle_name needs conceptual fix, too.

Whole thing needs thinking regarding what if eeprom names are identical?
Need to doctor serial number to include ":N[N]".

We can also use a memoryfile sort of static location for Dongles array etc.

reinitdongles:
    Scan for all dongles and place them in a local dongle array.
    Then we can conditionalize what we do from there.
    if ( tempdongles has no duplicates )
    {
        Merge with Dongles array. Presume full name matches are moved dongles.
    }
    else
    {
        Merge with dongles array.
            if not one of the dupes Presume full name matches are moved dongles.
            else if one dupe tested matches port nums that is good
            else add to Dongles.
    }

Find by name
    If name includes suffix search with artificial suffix.
    else if name is clean test for name

Report names
    Always add suffixes. This will mess up existing setups. But, once setup it
    will work better.



Hm, would it be "feasible" to make this puppy into a service? If so how might
that service work? (Periodically taste the devices? Actually "own" the devices
and transfer (copy) data into the program using it? Hm, that latter would be a
whole 4.8 megabytes/second load on the system. Compared to modern gigabyte per
second chips, memories, and backplanes this is nothing.) Should I make the tuner
software a series of plugins for the service? That sounds like a very
interesting proposition. The problem here is moving the data from the server's
memory space to the program's memory space.

Perhaps a better choice would be to keep a value added server present on the
system for intercepting the dongle IR ports, translating them, and injecting
them into the system at appropriate places. It can also keep the shared memory
location for all dongles present open, poll dongles periodically, and make the
rtlsdr.dll job somewhat easier. Unfortunately, indications are that this is just
plain not going to work due to WinUSB limitations. The two interfaces would have
to be dealt with in the same program. BLEAH!

Something to remember - if the full dongle ID strings have been seen before then
we can open the second one if it's not busy. And if it's already marked as found
do not alter the path....

Regardless I can open the shared memory using only the CreateFileMap function
without trying to mediate first with OpenFileMap. The memory area is always the
same size. So when Create... returns ERROR_ALREADY_EXISTS the handle returned is
sufficient for our needs. That grossly simplifies things.

===========

The device class for notifications is: (Are there others?)
{88bae032-5a81-49f0-bc3d-a4ff138216d6}

Raw USB devices....
{ 0xa5dcbf10, 0x6530, 0x11d2, { 0x90, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x51, 0xed } }
{a5dcbf10-6530-11d2-901f-00c04fb951ed}

See:
http://www.ftdichip.com/Support/Documents/AppNotes/AN_152_Detecting_USB_%20Device_Insertion_and_Removal.pdf


http://stackoverflow.com/questions/706352/use-registerdevicenotification-for-all-usb-devices
GUID_DEVINTERFACE_USB_DEVICE (in "usbiodef.h")

DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
  ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

  NotificationFilter.dbcc_size = sizeof(NotificationFilter);
  NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  NotificationFilter.dbcc_reserved = 0;

  NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

  HDEVNOTIFY hDevNotify = RegisterDeviceNotification(hwnd, &NotificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE)


http://blogs.msdn.com/b/doronh/archive/2006/02/15/532679.aspx

  HDEVNOTIFY
RegisterInterfaceNotificationWorker(
    HANDLE Recipient,
    LPCGUID Guid, 
    DWORD Flags
    )
{
    DEV_BROADCAST_DEVICEINTERFACE dbh;

    ZeroMemory(&dbh, sizeof(dbh));

    dbh.dbcc_size = sizeof(dbh);
    dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    CopyMemory(&dbh.dbcc_classguid, Guid, sizeof(GUID));

    return RegisterDeviceNotification(Recipient, &dbh, Flags);
}

$(SolutionDir)SDRRadio\SDRConsole.exe

20160910 changes
1) Mark busy before safe find in write_eeprom
2) If read_eeprom search for dongle in DB fails try path, PID, VID only
3) Greatly revised dongle database update function in reinit_dongles.
4) Protect registry accesses with global mutex.


===

	/* filter corner = lowest */
	rc = r820t_write_reg_mask(priv, 0x0a, 0x0f, 0x0f);
	if (rc < 0)
		return rc;


    filter_cur = 0x40 low?
	rc = r820t_write_reg_mask(priv, 0x0a, filter_cur, 0x60);
	if (rc < 0)
		return rc;

    /* r10[4]:low q(1'b1) */
    0 <= fil_cal_code <= 0x0f
	rc = r820t_write_reg_mask(priv, 0x0a,
				  filt_q | priv->fil_cal_code, 0x1f);


	/* filter bw=+2cap, hp=5M */
	rc = r820t_write_reg_mask(priv, 0x0b, 0x60, 0x6f);
	if (rc < 0)
		return rc;

    // From calibration
	/* Set filt_cap */
	rc = r820t_write_reg_mask(priv, 0x0b, hp_cor, 0x60);
	if (rc < 0)
		return rc;

	/* Set BW, Filter_gain, & HP corner */
	rc = r820t_write_reg_mask(priv, 0x0b, hp_cor, 0xef);
	if (rc < 0)
		return rc;

    hp_cor:
    hp_cor = 0x6a;		/* 1.7m disable, +2cap, 1.25mhz */
    hp_cor = 0x6b;		/* 1.7m disable, +2cap, 1.0mhz */
//  hp_cor = 0x2b;		/* 1.7m disable, +1cap, 1.0mhz */
    hp_cor = 0x2a;		/* 1.7m disable, +1cap, 1.25mhz */
    hp_cor = 0x0b;		/* 1.7m disable, +0cap, 1.0mhz */
    ls nybble is BW
    Upper part is > 0x60 mask 0-3

    AirSpy uses
     LPF    LPFA    LPFB    HPF     CF          BW
     0x3c,  0x03,	0x00,	0x02,   10000000    //	10 MHz
     0x31,  0x0E,	0x00,	0x0b,   10000000    //	5 MHz
     0x28,  0x07,	0x60,	0x0d,   10000000    //	2.5 MHz
     0x06,  0x09,	0xE0,	0x03,   10000000    //	1.25 MHz
     0x00,  0x0F,	0xE0,	0x04,   10000000    //	.625 MHz
            
     0x20,  0x0F,	0x60,	0x00,   6000000,    //	6 MHz
     0x1e,  0x01,	0x80,	0x05,   6000000,    //	3 MHz
     0x12,  0x0D,	0x80,	0x05,   6000000,    //	1.5 MHz
     0x06,  0x09,	0xE0,	0x06,   6000000,    //	.75 MHz
            
     0x08,  0x07,	0xE0,	0x00,   3000000,    //	3 MHz
     0x07,  0x08,	0xE0,	0x04,   3000000,    //	1.5 MHz
     0x02,  0x0D,	0xE0,	0x04,   3000000,    //	.625 MHz
            
     0x04,  0x0B,	0xE0,   0x00,   2500000,    //	2.5 MHz
     0x06,  0x09,	0xE0,   0x03,   2500000,    //	1.25 MHz
     0x06,  0x09,	0xE0,   0x04,   2500000,    //	.625 MHz

    a = 0xE0 | 0x00-0x0f
    b = 0xE0, 0x80, 0x60, 0x00 | 0x00-0x0f

BW 10.000000 MHz, Reg 0x0a 0xe3, Reg 0x0B 0x0d, IF 0, decimation 0, rate 10000000
BW 5.000000 MHz,  Reg 0x0a 0xee, Reg 0x0B 0x04, IF 0, decimation 1, rate 10000000
BW 2.500000 MHz,  Reg 0x0a 0xe7, Reg 0x0B 0x62, IF 270000, decimation 2, rate 10000000
BW 1.250000 MHz,  Reg 0x0a 0xe9, Reg 0x0B 0xec, IF 3500000, decimation 3, rate 10000000
BW 0.625000 MHz,  Reg 0x0a 0xef, Reg 0x0B 0xeb, IF 3500000, decimation 4, rate 10000000

BW 6.000000 MHz,  Reg 0x0a 0xef, Reg 0x0B 0x6f, IF 0, decimation 0, rate 6000000
BW 3.000000 MHz,  Reg 0x0a 0xe1, Reg 0x0B 0x8a, IF 300000, decimation 1, rate 6000000
BW 1.500000 MHz,  Reg 0x0a 0xed, Reg 0x0B 0x8a, IF 750000, decimation 2, rate 6000000
BW 0.750000 MHz,  Reg 0x0a 0xe9, Reg 0x0B 0xe9, IF 1000000, decimation 3, rate 6000000

BW 3.000000 MHz,  Reg 0x0a 0xe7, Reg 0x0B 0xef, IF 0, decimation 0, rate 3000000
BW 1.500000 MHz,  Reg 0x0a 0xe8, Reg 0x0B 0xeb, IF -250000, decimation 1, rate 3000000
BW 0.625000 MHz,  Reg 0x0a 0xed, Reg 0x0B 0xeb, IF -300000, decimation 2, rate 3000000

BW 2.500000 MHz,  Reg 0x0a 0xeb, Reg 0x0B 0xef, IF 0, decimation 0, rate 2500000
BW 1.250000 MHz,  Reg 0x0a 0xe9, Reg 0x0B 0xec, IF -250000, decimation 1, rate 2500000
BW 0.625000 MHz,  Reg 0x0a 0xe9, Reg 0x0B 0xeb, IF -300000, decimation 2, rate 2500000

    Leif
    a = 0xEF
    b = 0xE7-0xef, 0x51     - 51 seems odd....

    So
    lpfa = 0xe0 | 0x10 | 0x00..0x0f
    lpfb = 0xE0, 0x80, 0x60, 0x00 
    hpfb = 0x00-0x11
    filtq = 0x10

    regA = 0xe0 | filtq | lpfa
    regB = lpfb | hpfb

    bw      rega    regb    if
    2100k   ef      52      4750k
    1800k


    Leif BW         with        without
    300 kHz EF E7 if 2050       2050
    400 kHz EF E8 if 2000-1975  2050
    550 kHz EF E9 if 1925       2050
    700 kHz EF EA if 1850       2050
    1.0 kHz EF EB if 1700-1750  2050
    1.2 kHz EF EC if 1425       1750
    1.3 kHz EF ED if 1400       1750
    1.6 kHz EF EF if 1200               really 1.5 MHz at 3dB
    2.2 kHz EF 51 if 

    mutability
    1.9     FF 6B if 3570
    tweaked (more gain)
    1.9     FF 0e if 3575

    2.1     FF 0E if 1900
            EF 0E ...
    2.0     EF 8E if 1700
    1.85    EF 8D if 1700
    1.75    EF 8C if 1825
    1.6     EF 8B if 1950
    1.3     EF 8A if 2100
    1.1     EF 89 if 2150
    0.95    EF 88 if 2200
    0.7     EF 87 if 2325

    1.3     EF AB if 1750
    0.8     EF AA if 1875
    0.65    EF A9 if 1950
    0.55    ED EA of 1750
    0.45    EF E9 if 1825
    0.25    EF E8 if 1900


    So what do we use?
xx  2.1     FF 0E if 1900
    2.0     EF 8E if 1700
    1.85    EF 8D if 1700
    1.75    EF 8C if 1825
    1.6     EF 8B if 1950
xx  1.3b    EF AB if 1750
    1.1     EF 89 if 2150
 x  0.95    EF 88 if 2200
x   0.8     EF AA if 1875
 x  0.7     EF 87 if 2325
    0.65    EF A9 if 1950
 x  0.55    ED EA of 1750
xx  0.45    EF E9 if 1825
xx  0.25    EF E8 if 1900

struct
{
    DWORD   Bandwidth,
    DWORD   IfFreq,
    BYTE    Reg0A,
    BYTE    reg0B,
};



RTLSDR_API int rtlsdr_get_tuner_bandwidths(rtlsdr_dev_t *dev, int *bandwidths);
RTLSDR_API int rtlsdr_set_tuner_bandwidth(rtlsdr_dev_t *dev, int bandwidth);
RTLSDR_API int rtlsdr_get_bandwidth_set_name( rtlsdr_dev_t *dev
                                            , int nSet
                                            , char* pString
                                            );
RTLSDR_API int rtlsdr_set_bandwidth_set( rtlsdr_dev_t *dev, int nSet ); 
#if defined( SET_SPECIAL_FILTER_VALUES )    // For testing
RTLSDR_API int rtlsdr_set_if_values     ( rtlsdr_dev_t *dev
                                        , BYTE          regA
                                        , BYTE          regB
                                        , DWORD         ifFreq
                                        );

#endif
