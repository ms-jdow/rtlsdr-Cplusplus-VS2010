//	Recognized Dongles.h

#pragma once

//	This is the structure defining the recognition data and device
//	name for the various dongle types rtlsdr.dll recognizes and can
//	properly manipulate.

typedef struct rtlsdr_dongle
{
	uint16_t vid;
	uint16_t pid;
	const char *name;
	const char *manfname;
	const char *prodname;
} rtlsdr_dongle_t;

extern const rtlsdr_dongle_t known_devices[];
#if defined( LIBSDRNAMES )
/*
 * Please add your device here and send a patch to osmocom-sdr@lists.osmocom.org
 */
const rtlsdr_dongle_t known_devices[] =
{
	{ 0x0bda, 0x2832, "* Generic RTL2832U"
					, "Generic", "RTL2832U" },
	{ 0x0bda, 0x2838, "* Generic RTL2832U OEM"
					, "Generic", "RTL2832U OEM" },
	{ 0x0413, 0x6680, "* DigitalNow Quad DVB-T PCI-E card"
					, "DigitalNow", "Quad DVB-T PCI-E card" },
	{ 0x0413, 0x6f0f, "* Leadtek WinFast DTV Dongle mini D"
					, "Leadtek", "WinFast DTV Dongle mini D" },
	{ 0x0458, 0x707f, "* Genius TVGo DVB-T03 USB dongle (Ver. B)"
					, "Genius", "TVGo DVB-T03 USB dongle (Ver. B)" },
	{ 0x0ccd, 0x00a9, "* Terratec Cinergy T Stick Black (rev 1)"
					, "Terratec", "Cinergy T Stick Black (rev 1)" },
	{ 0x0ccd, 0x00b3, "* Terratec NOXON DAB/DAB+ USB dongle (rev 1)"
					, "Terratec", "NOXON DAB/DAB+ USB dongle (rev 1)" },
	{ 0x0ccd, 0x00b4, "* Terratec Deutschlandradio DAB Stick"
					, "Terratec", "Deutschlandradio DAB Stick" },
	{ 0x0ccd, 0x00b5, "* Terratec NOXON DAB Stick - Radio Energy"
					, "* Terratec", "NOXON DAB Stick - Radio Energy" },
	{ 0x0ccd, 0x00b7, "* Terratec Media Broadcast DAB Stick"
					, "Terratec", "Media Broadcast DAB Stick" },
	{ 0x0ccd, 0x00b8, "* Terratec BR DAB Stick"
					, "Terratec", "BR DAB Stick" },
	{ 0x0ccd, 0x00b9, "* Terratec WDR DAB Stick"
					, "Terratec", "WDR DAB Stick" },
	{ 0x0ccd, 0x00c0, "* Terratec MuellerVerlag DAB Stick"
					, "Terratec", "MuellerVerlag DAB Stick" },
	{ 0x0ccd, 0x00c6, "* Terratec Fraunhofer DAB Stick"
					, "Terratec", "Fraunhofer DAB Stick" },
	{ 0x0ccd, 0x00d3, "* Terratec Cinergy T Stick RC (Rev.3)"
					, "Terratec", "Cinergy T Stick RC (Rev.3)" },
	{ 0x0ccd, 0x00d7, "* Terratec T Stick PLUS"
					, "Terratec", "T Stick PLUS" },
	{ 0x0ccd, 0x00e0, "* Terratec NOXON DAB/DAB+ USB dongle (rev 2)"
					, "Terratec", "NOXON DAB/DAB+ USB dongle (rev 2)" },
	{ 0x1554, 0x5020, "* PixelView PV-DT235U(RN)"
					, "PixelView", "PV-DT235U(RN)" },
	{ 0x15f4, 0x0131, "* Astrometa DVB-T/DVB-T2"
					, "Astrometa", "DVB-T/DVB-T2" },
	{ 0x15f4, 0x0133, "* HanfTek DAB+FM+DVB-T"
					, "HanfTek", "DAB+FM+DVB-T" },
	{ 0x185b, 0x0620, "* Compro Videomate U620F"
					, "Compro", "Videomate U620F"},
	{ 0x185b, 0x0650, "* Compro Videomate U650F"
					, "Compro", "Videomate U650F"},
	{ 0x185b, 0x0680, "* Compro Videomate U680F"
					, "Compro", "Videomate U680F"},
	{ 0x1b80, 0xd393, "* GIGABYTE GT-U7300"
					, "GIGABYTE", "GT-U7300" },
	{ 0x1b80, 0xd394, "* DIKOM USB-DVBT HD"
					, "DIKOM", "USB-DVBT HD" },
	{ 0x1b80, 0xd395, "* Peak 102569AGPK"
					, "Peak", "102569AGPK" },
	{ 0x1b80, 0xd397, "* KWorld KW-UB450-T USB DVB-T Pico TV"
					, "KWorld", "KW-UB450-T USB DVB-T Pico TV" },
	{ 0x1b80, 0xd398, "* Zaapa ZT-MINDVBZP"
					, "Zaapa", "ZT-MINDVBZP" },
	{ 0x1b80, 0xd39d, "* SVEON STV20 DVB-T USB & FM"
					, "SVEON", "STV20 DVB-T USB & FM" },
	{ 0x1b80, 0xd3a4, "* Twintech UT-40"
					, "Twintech", "UT-40" },
	{ 0x1b80, 0xd3a8, "* ASUS U3100MINI_PLUS_V2"
					, "ASUS", "U3100MINI_PLUS_V2" },
	{ 0x1b80, 0xd3af, "* SVEON STV27 DVB-T USB & FM"
					, "SVEON", "STV27 DVB-T USB & FM" },
	{ 0x1b80, 0xd3b0, "* SVEON STV21 DVB-T USB & FM"
					, "SVEON", "STV21 DVB-T USB & FM" },
	{ 0x1d19, 0x1101, "* Dexatek DK DVB-T Dongle (Logilink VG0002A)"
					, "Dexatek", "DK DVB-T Dongle (Logilink VG0002A)" },
	{ 0x1d19, 0x1102, "* Dexatek DK DVB-T Dongle (MSI DigiVox mini II V3.0)"
					, "Dexatek", "DK DVB-T Dongle (MSI DigiVox mini II V3.0)" },
	{ 0x1d19, 0x1103, "* Dexatek Technology Ltd. DK 5217 DVB-T Dongle"
					, "Dexatek", "Technology Ltd. DK 5217 DVB-T Dongle" },
	{ 0x1d19, 0x1104, "* MSI DigiVox Micro HD"
					, "MSI", "DigiVox Micro HD" },
	{ 0x1f4d, 0xa803, "* Sweex DVB-T USB"
					, "Sweex", "DVB-T USB" },
	{ 0x1f4d, 0xb803, "* GTek T803"
					, "GTek", "T803" },
	{ 0x1f4d, 0xc803, "* Lifeview LV5TDeluxe"
					, "Lifeview", "LV5TDeluxe" },
	{ 0x1f4d, 0xd286, "* MyGica TD312"
					, "MyGica", "TD312" },
	{ 0x1f4d, 0xd803, "* PROlectrix DV107669"
					, "PROlectrix", "DV107669" },
};
#endif
