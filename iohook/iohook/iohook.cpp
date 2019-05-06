// iohook.cpp: define las funciones exportadas de la aplicación DLL.
//

#include "stdafx.h"
#include "iohook.h"
#include <stdio.h>
#include "eeprom.cpp"	// lol, fix this ASAP

extern "C" {
	FILE *lock_fp = NULL;
	// masks
#define MASK_UPLEFT		0x0001
#define MASK_UPRIGHT	0x0002
#define MASK_CENTER		0x0004
#define MASK_DOWNLEFT	0x0008
#define MASK_DOWNRIGHT	0x0010

#define MASK_SERVICE	0x0000
#define MASK_TEST		0x0000
#define MASK_CLEAR		0x0000

#define MASK_SENSOR_RQ  0x0003

// addresses 
#define P1OUTPUT_ADDRESS		0x02A0
#define P2OUTPUT_ADDRESS		0x02A2
#define P1INPUT_ADDRESS			0x02A4
#define P2INPUT_ADDRESS			0x02A6
#define EEPROM_OUTPUT_ADDRESS	0x02AE
#define EEPROM_INPUT_ADDRESS	0x02AC
#define LOCK_INPUT_ADDRESS		0x02AA
#define LOCK_OUTPUT_ADDRESS		EEPROM_OUTPUT_ADDRESS	// this addres has is "connected" with lock pins too
#define POST_CODES_ADDRESS		0x0080	// yes, post codes address 

	UINT16	P1INPUT;
	UINT16	P2INPUT;

	UINT16	P1OUTPUT;
	UINT16	P2OUTPUT;

	typedef struct
	{
		int vkcode;
		UINT16 mask;
		UINT16 *output;
	}keybind_t;

	keybind_t keylist[] =
	{
		// player 1
		{ 'Q', MASK_UPLEFT , &P1INPUT },
		{ 'E', MASK_UPRIGHT , &P1INPUT },
		{ 'S', MASK_CENTER , &P1INPUT },
		{ 'Z', MASK_DOWNLEFT , &P1INPUT },
		{ 'C', MASK_DOWNRIGHT , &P1INPUT },

		{ VK_NUMPAD7, MASK_UPLEFT , &P2INPUT },
		{ VK_NUMPAD9, MASK_UPRIGHT , &P2INPUT },
		{ VK_NUMPAD5, MASK_CENTER , &P2INPUT },
		{ VK_NUMPAD1, MASK_DOWNLEFT , &P2INPUT },
		{ VK_NUMPAD3, MASK_DOWNRIGHT , &P2INPUT },
		
		// cab
		/*
		{ VK_F1, MASK_TEST , &P2INPUT }, // coin
		{ VK_F2, MASK_SERVICE , &P2INPUT }, // insert credit
		{ VK_F3, MASK_CLEAR , &P2INPUT }, // insert credit
		*/
	};

	eeprom_t eeprom;

	int initIO(void)
	{
		// 
		fopen_s(&lock_fp, "lock.dump", "wb");
		eeprom_init(&eeprom);
		// TODO: init usbio
		return 0;
	}

	int closeIO(void)
	{
		// TODO: close usbio here
		return 0;
	}

	// reads input from keyboard
	int readfromKeyboard(void)
	{
		P1INPUT = 0xFFFF;
		P2INPUT = 0xFFFF;

		int i;
		int keys = sizeof(keylist)/sizeof(keybind_t);
		for (i = 0; i < keys; i++)
		{
			if (GetKeyState(keylist[i].vkcode) & 0x8000)
			{
				*keylist[i].output = ( *keylist[i].output & ~(keylist[i].mask) );
			}
		}
		return i;
	}

	// read input from usbio
	int readfromUsbIO(void)
	{
		return 0;
	}

	IOHOOK_API signed int __cdecl eeprom_write(char *buffer, signed int bytes)
	{
		FILE *fp = NULL;
		fopen_s(&fp, "eeprom.bin", "wb");
		if (fp)
		{
			//rewind(fp);
			//fseek(fp, 0, SEEK_SET);
			fwrite(buffer, 1, bytes, fp);
			fclose(fp);
		}
		return bytes;
	}

	IOHOOK_API signed int __cdecl eeprom_read(char *buffer, signed int bytes)
	{
		FILE *fp = NULL;
		fopen_s(&fp, "eeprom.bin", "rb");
		if (fp)
		{

			fread(buffer, 1, bytes, fp);
			fclose(fp);
		}
		return bytes;
	}

	IOHOOK_API unsigned __int8 __cdecl outbyte(unsigned __int16 port, unsigned __int8 cmd)
	{
		switch (port)
		{
		case POST_CODES_ADDRESS: break;	// nice, PIU32 exe could raise POST CODES to debug, the codes are shown in the LED display i think
		default:
			break;
		}
		return -1;
	}

	IOHOOK_API unsigned __int16 __cdecl outword(unsigned __int16 port, unsigned __int16 cmd)
	{
		pin_states_t pins;
		unsigned __int8 lockbyte;
		#define UPDATE_PIN(MASK,PIN) do{ if(MASK) PIN = 1; else PIN = 0;}while(0)
		switch (port)
		{	
			// TODO: update when support to usbio
			case P1OUTPUT_ADDRESS: /* setSensorRq(cmd&MASK_SENSOR_RQ);*/
			case P2OUTPUT_ADDRESS: /* writeUsbOutput(cmd);*/ 
				break;
			case EEPROM_OUTPUT_ADDRESS:	// eeprom port
				lockbyte = (unsigned __int8)(cmd & 0x00F0);
				if (lock_fp)
				{
					fwrite(&lockbyte, 1, 1, lock_fp);
					fwrite(&lockbyte, 1, 1, lock_fp);
				}
				// update pins
				UPDATE_PIN(cmd & 0x0001, pins.chipSelect); 
				UPDATE_PIN(cmd & 0x0002, pins.clock);
				UPDATE_PIN(cmd & 0x0004, pins.datain);
				// update eeprom stuff
				eeprom_update(&eeprom, pins);
				break;
			default:
				break;
		}
		return -1;
	}
	
	IOHOOK_API  unsigned __int16 __cdecl inputword(unsigned __int16 port)
	{
		switch (port)
		{
			case LOCK_INPUT_ADDRESS:	// used to communicate with lock, idk the algo at this moment
				break;
			case EEPROM_INPUT_ADDRESS:	// eeprom read address
				
				if ( eeprom.pins.dataout == 0)
				{
					return 0;
				}
				else
				{
					return 1;
				}
				break;
			case P1INPUT_ADDRESS: readfromKeyboard(); return P1INPUT;  break;
			case P2INPUT_ADDRESS: readfromKeyboard(); return P2INPUT;  break;
			default:
					break;
		}

		return -1;
	}
}

Copyright (c) 2019 ROAD24

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.