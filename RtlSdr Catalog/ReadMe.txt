================================================================================
    MICROSOFT FOUNDATION CLASS LIBRARY : rtlsdr_service_test Project Overview
===============================================================================

The application wizard has created this rtlsdr_service_test application for
you.  This application not only demonstrates the basics of using the Microsoft
Foundation Classes but is also a starting point for writing your application.

This file contains a summary of what you will find in each of the files that
make up your rtlsdr_service_test application.

rtlsdr_service_test.vcxproj
    This is the main project file for VC++ projects generated using an application wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    application wizard.

rtlsdr_service_test.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

rtlsdr_service_test.h
    This is the main header file for the application.  It includes other
    project specific headers (including Resource.h) and declares the
    Crtlsdr_service_testApp application class.

rtlsdr_service_test.cpp
    This is the main application source file that contains the application
    class Crtlsdr_service_testApp.

rtlsdr_service_test.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Visual C++. Your project resources are in 1033.

res\rtlsdr_service_test.ico
    This is an icon file, which is used as the application's icon.  This
    icon is included by the main resource file rtlsdr_service_test.rc.

res\rtlsdr_service_test.rc2
    This file contains resources that are not edited by Microsoft
    Visual C++. You should place all resources not editable by
    the resource editor in this file.


/////////////////////////////////////////////////////////////////////////////

The application wizard creates one dialog class:

rtlsdr_service_testDlg.h, rtlsdr_service_testDlg.cpp - the dialog
    These files contain your Crtlsdr_service_testDlg class.  This class defines
    the behavior of your application's main dialog.  The dialog's template is
    in rtlsdr_service_test.rc, which can be edited in Microsoft Visual C++.


/////////////////////////////////////////////////////////////////////////////

Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named rtlsdr_service_test.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

rtlsdr_service_test.manifest
	Application manifest files are used by Windows XP to describe an applications
	dependency on specific versions of Side-by-Side assemblies. The loader uses this
	information to load the appropriate assembly from the assembly cache or private
	from the application. The Application manifest  maybe included for redistribution
	as an external .manifest file that is installed in the same folder as the application
	executable or it may be included in the executable in the form of a resource.
/////////////////////////////////////////////////////////////////////////////

Other notes:

The application wizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

If your application uses MFC in a shared DLL, you will need
to redistribute the MFC DLLs. If your application is in a language
other than the operating system's locale, you will also have to
redistribute the corresponding localized resources MFC100XXX.DLL.
For more information on both of these topics, please see the section on
redistributing Visual C++ applications in MSDN documentation.

/////////////////////////////////////////////////////////////////////////////






Device list dev_count 32
xxxxx Device  0 0x8086 0x3a39:                  ICH10 Family USB Universal Host Controller
xxxxx Device  1 0x8086 0x3a35:                  ICH10 Family USB Universal Host Controller
xxxxx Device  2 0x8086 0x3a38:                  ICH10 Family USB Universal Host Controller
xxxxx Device  3 0x8086 0x3a34:                  ICH10 Family USB Universal Host Controller
xxxxx Device  4 0x8086 0x3a37:                  ICH10 Family USB Universal Host Controller
xxxxx Device  5 0x8086 0x3a36:                  ICH10 Family USB Universal Host Controller
xxxxx Device  6 0x8086 0x3a3a:                  ICH10 Family USB Universal Host Controller
xxxxx Device  7 0x8086 0x3a3c:                  ICH10 Family USB Enhanced Host Controller

x     Device 15 0x05e3 0x0608: 0x1              Genesys Logic, Inc. hub
xx    Device 28 0x10d5 0x5a08: 0x1 0x1          Autologic Inc. / Uni Class Technology Co., Ltd (KVM?) where is kbd and mouse?

x     Device 30 0x1a40 0x0201: 0x2              Terminus Technology Inc. FE 2.1 7-port Hub 
xx    Device 18 0x07f2 0x0001: 0x2 0x2          Microcomputer Applications, Inc.  Keylock dongle.   (Gilderboop?)
xx    Device  8 0x0403 0x6001: 0x2 0x4          Future Technology Devices International, Ltd FT232 Serial (UART) IC
xx    Device 29 0x1a40 0x0101: 0x2 0x5          Terminus Technology Inc
xxx   Device 24 0x0c45 0x167f: 0x2 0x5 0x1      Microdia Thingamabob    Audio Hockey puck
xx    Device 23 0x0bda 0x2838: 0x2 0x6          Found Rafael Micro R820T tuner Sn dng 5

x     Device  9 0x0409 0x005a: 0x3              NEC Corp. HighSpeed Hub
xx    Device 10 0x0409 0x005a: 0x3 0x4          NEC Corp. HighSpeed Hub
xxx   Device 11 0x0409 0x005a: 0x3 0x4 0x1      NEC Corp. HighSpeed Hub
xxxx  Device 27 0x0ecd 0xa100: 0x3 0x4 0x1 0x1  Lite-On IT Corp LDW-411SX DVD/CD Rewritable Drive
xxxx  Device 13 0x0557 0x2011: 0x3 0x4 0x1 0x2  ATEN International Co., Ltd four port serial
xxxx  Device 14 0x0557 0x2011: 0x3 0x4 0x1 0x3  ATEN International Co., Ltd four port serial
xxxx  Device 21 0x0bda 0x2838: 0x3 0x4 0x1 0x4  Found Elonics E4000 tuner Sn dng 15
xxx   Device 25 0x0ccd 0x00d7: 0x3 0x4 0x3      Found Terratec T-Stick Plus sn 00000002
xxx   Device 12 0x0409 0x005a: 0x3 0x4 0x4      NEC Corp. HighSpeed Hub
xxxx  Device 19 0x096e 0x0201: 0x3 0x4 0x4 0x1  Feitian Technologies, Inc. security dongle
xxxx  Device 26 0x0d8c 0x000e: 0x3 0x4 0x4 0x2  C-Media Electronics, Inc

x     Device 16 0x05e3 0x0608: 0x4              Genesys Logic, Inc. hub - extender cable to East side LR
xx    Device 22 0x0bda 0x2838: 0x4 0x1          Found Rafael Micro R820T tuner Sn dng 3

x     Device 20 00xbda 0x2832: 0x5              Found Elonics E4000 tuner Sn dng 00000992

USB 3.0 board
x     Device 20 0x096e 0x0201: 0x4              NC

x     Device 20 0x096e 0x0201: 0x5              NC

x     Device 17 0x05e3 0x0608: 0x6              Genesys Logic, Inc. hub - extender cable    USB 3 #2
xx    Device 31 0x1d50 0x60a1: 0x6 0x1          AirSpy

x     Device 30 0x1a40 0x0101: 0x7              Terminus Technology Inc. hub (extender cable)   USB 3 #3
xx    Device 33 0x1d50 0x60a1: 0x7 0x4          AirSpy 2



In reinit dongles we try to open a dongle and try to find it in the database. If
it is not in the database and we could open it, add it. Otherwise, perhaps, let
it float.

Cases to oonsider:
    Finding all dongles
        cannot open the dongle -> Ignore it. Get it later.
            if usbpath matches mark busy.... (Do NOT add to local db.)
        can open the dongle
            Get names
*           duplicated names    Force a rename? What does that do to me?
            Stuff into temp database

//  So we need a de-duplicate feature in here.
//  Name may or may not appear in masterdb
//      name and path may or may not appear in masterdb
//  Name may or may not be a duplicate (but appears once in masterdb)
//      path may or may not match (Dongle may or may not be in original slot)

    while ( reinit_dongles.GetSize() > 0 )
        if exact match to masterdb
           reinit_dongles.RemoveAt( 0 )
           continue;
        else if !name in masterdb
            Add to masterdb 
            reinit_dongles.RemoveAt( 0 )
            continue;
            //  Name is in db
        else if !duplicated
            fix path in masterdb
            reinit_dongles.RemoveAt( 0 )
        else
        //  Duplicated in local db and !exact match and not in db
        //  So we have a duplicate - let's deal with it.
        //  It might be a set of new dongles that duplicate what we have
        //  or it might be a new duplicate matching one entry we already have.
        //  Suppose I increment sn by 1 and retest for matches. Repeat until
        //  no match and add to database.
            bool exact = false
            while (no name match in masterdb)
                if exact match
                    exact = true
                    write to dongle     should be impossible but....
                    break
                fi
            elihw
            if !exact
                add to masterdb
            fi
        fi

    rof
