/**
	* header for eeprom emulator ( 93C86 )
	* Old pump it up games have an embedded eeprom to store data from settins and records,
	* fortunately for me PIUIO works as a bridge between the pins of the eeprom and to 2 addresses ( OUT pins and IN pins ) just like uC works
	* so the game actually does bit banging with the eeprom ( 93C86 ) and emulating this protocol and eeproms is "simple"
	*
	* FAQs
	*
	* Q: why don't you just detour the read and write function like you did with direct port access functions ?
	* A: because this behaviour will need only a few mods to work with softwares like dosbox, virtualbox, mame
	*
	* Q: did you say MK3 ?
	* A: yes, mk3 uses different adresses to read/write pins states but has the same protocol 
	*	 except for address that  are 6 bit instead of 10, the code should be easy to adapt
	*
	* Q: your english sucks
	* A: yes
	*
	* Q: this sucks
	* A: yes, but works
	*
	* Q: hey, i'm from andamiro bla bla bla 
	* A: nice
	*
	* Q: did you realize the last 3 questions aren't questions ?
	* A: yes, but i have an admirer who likes to say 2 of them and i'm saving him those minutes to use in something more productive
	*
	*/

#include "eeprom.h"

extern "C" {

	char *from_opcode_to_string(__int8 opcode)
	{
		// NOTE: iirc this asserted every time a not supported opcode was found
		// but i realizaed that was not good inside a DLL
		#define RETURN_OPCODE_ASSERT(OP) if( opcode == OP ) return #OP
		RETURN_OPCODE_ASSERT(OPCODE_ERASE);
		RETURN_OPCODE_ASSERT(OPCODE_READ);
		RETURN_OPCODE_ASSERT(OPCODE_WRITE);
		RETURN_OPCODE_ASSERT(OPCODE_EXTENDED);
		#undef RETURN_OPCODE_ASSERT
		return "OPCODE_UNKNOWN";
	}

	char *from_xopcode_to_string(__int16 opcode)
	{
		#define RETURN_OPCODE_ASSERT(OP) if( opcode == OP ) return #OP
		RETURN_OPCODE_ASSERT(EXTENDED_ERAL);
		RETURN_OPCODE_ASSERT(EXTENDED_EWDS);
		RETURN_OPCODE_ASSERT(EXTENDED_EWEN);
		RETURN_OPCODE_ASSERT(EXTENDED_WRAL);
		#undef RETURN_OPCODE_ASSERT
		return "EXTENDED_UNKNOWN";
	}

	char *from_state_to_string(__int32 state)
	{
		#define RETURN_STATE_ASSERT(STATE) if( state == STATE ) return #STATE
		RETURN_STATE_ASSERT(EEPROM_STATE_IDLE);
		RETURN_STATE_ASSERT(EEPROM_STATE_READING_OPCODE);
		RETURN_STATE_ASSERT(EEPROM_STATE_READING_ADDRESS);
		RETURN_STATE_ASSERT(EEPROM_STATE_READING_DATA);
		RETURN_STATE_ASSERT(EEPROM_STATE_WRITTING_DATA);
		#undef RETURN_STATE_ASSERT
		return "STATE_UNKNOWN";
	}

	void warn_log(const char *msg)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		WORD saved_attributes;
		DWORD written;
		/* Save current attributes */
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
		saved_attributes = consoleInfo.wAttributes;

		SetConsoleTextAttribute(hConsole,6);

		WriteConsoleA(hConsole, msg, strlen(msg), &written, NULL);

		/* Restore original attributes */
		SetConsoleTextAttribute(hConsole, saved_attributes);
	}

	void error_log(const char *msg)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		WORD saved_attributes;
		DWORD written;
		/* Save current attributes */
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
		saved_attributes = consoleInfo.wAttributes;

		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);

		WriteConsoleA(hConsole, msg, strlen(msg), &written, NULL);

		/* Restore original attributes */
		SetConsoleTextAttribute(hConsole, saved_attributes);
	}

	void debug_log(const char *msg)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD written;
		WriteConsoleA(hConsole, msg, strlen(msg), &written, NULL);
	}


	void eeprom_load_memory_from_file(eeprom_t *eeprom, char *filename)
	{
		FILE *fp = NULL;
		fopen_s(&fp, filename, "rb");
		if (fp == NULL)	// we dont have the file , create it
		{
			fopen_s(&fp, filename, "wb");
			if (fp == NULL)	// oops  nothing to do 
			{
				LOG_ERROR("error creating eeprom.bin\n");
				return;
			}
			fwrite(eeprom->memory, 2, 2048, fp);
			fclose(fp);
			return;
		}

		// we can read the file now
		fread(eeprom->memory, 2, 2048, fp);
		fclose(fp);
	}

	void eeprom_save_memory_to_file(eeprom_t *eeprom, char *filename)
	{
		FILE *fp = NULL;
		fopen_s(&fp, filename, "wb");

		if (fp == NULL)	// oops  nothing to do 
		{
			LOG_ERROR("error creating eeprom.bin\n");
			return;
		}

		fwrite(eeprom->memory, 2, 2048, fp);
		fclose(fp);
	}

	// init eeprom emulation internals
	void eeprom_init(eeprom_t *eeprom)
	{
		// init pins as 0s
		eeprom->pins.chipSelect = 0;
		eeprom->pins.datain = 0;
		eeprom->pins.dataout = 1;
		eeprom->pins.clock = 0;
		eeprom->pins.org = 0;

		// initial state 
		eeprom->state = EEPROM_STATE_IDLE;

		//
		eeprom->address = 0;
		eeprom->bitcount = 0;
		eeprom->opcode = 0;
		eeprom->data = 0;

		// load eeprom memory from file
		eeprom_load_memory_from_file(eeprom, "eeprom.bin");
	}

	//	updates the eeprom based on the new states received, returns the new state of pins
	void eeprom_update(eeprom_t *eeprom, pin_states_t newpins)
	{
		// check for edges in signals
		bool clock_rising_edge =  (eeprom->pins.clock == 0) && (newpins.clock == 1);
		bool clock_falling_edge = (eeprom->pins.clock == 1) && (newpins.clock == 0);
		// update pin states
		eeprom->pins.datain = newpins.datain;
		// data out must be updated only by internal functions 
		// eeprom->pins.dataout = newpins.dataout;
		eeprom->pins.clock = newpins.clock;
		eeprom->pins.chipSelect = newpins.chipSelect;

		DEBUG_WARN("%s\n", from_state_to_string(eeprom->state));

		// if chipSelect pin is low we must not go further
		// with the special case of eeprom writting to memory, in that case eeprom will enter to idle until writting is done
		if (newpins.chipSelect == 0 && eeprom->state != EEPROM_STATE_WRITING_MEMORY)
		{
			if (eeprom->state != EEPROM_STATE_IDLE)
			{
				eeprom->state = EEPROM_STATE_IDLE;
				DEBUG_ERROR("EEPROM  DISABLED\n");
			}
			return;
		}
		/*
		if (clock_rising_edge)
			LOG("r");
		if (clock_falling_edge)
			LOG("f");
		*/
		// depending on the state
		switch (eeprom->state)
		{
		case EEPROM_STATE_READING_ADDRESS:
			if (!clock_rising_edge)
				return;
			eeprom->bitcount++;
			eeprom->address <<= 1;
			if (eeprom->pins.datain == 1)
				eeprom->address |= 0x01;

			// NOTE: depending on the state of org we should wait 11 bits instead of 10
			// PIU uses 10 so this is a TODO
			if (eeprom->bitcount == 10)
			{
				DEBUG_WARN("EEPROM_STATE_READING_ADDRESS: address[%04X]\n", eeprom->address);
				// the next steps is dependant of opcode
				switch (eeprom->opcode)
				{
				case OPCODE_EXTENDED:
					DEBUG_WARN("OPCODE_EXTENDED:  %s [0x%04X]\n", from_xopcode_to_string(eeprom->address), eeprom->address);
					// now check for extended cmd
					switch (eeprom->address)
					{
					case EXTENDED_EWEN:
						// enable write 
						eeprom->locked = 0;
						// the return to IDLE
						eeprom->state = EEPROM_STATE_IDLE;
						return;
					case EXTENDED_EWDS:
						// save eeprom here
						eeprom_save_memory_to_file(eeprom, "eeprom.bin");
					case EXTENDED_ERAL:	// TODO
					case EXTENDED_WRAL:	// TODO
					default:
						eeprom->locked = 1;
						eeprom->state = EEPROM_STATE_IDLE;
						return;
						break;
					}
				case OPCODE_READ:
					eeprom->bitcount = 0;
					eeprom->state = EEPROM_STATE_READING_DATA;
					eeprom->data = 0;
					DEBUG_ERROR("STARTING READ OPERATION address[0x%04X] ", eeprom->address);
					return;
				case OPCODE_WRITE:
					eeprom->bitcount = 0;
					eeprom->state = EEPROM_STATE_WRITTING_DATA;
					eeprom->data = 0;
					DEBUG_ERROR("STARTING WRITE OPERATION address[0x%04X] ", eeprom->address);
					return;
				case OPCODE_ERASE:	// TODO
				default:
					DEBUG_ERROR("OPCODE_ERASE NOT IMPLEMENTED OPCODE[%02X] ADDRESS[0x%04X]\n", eeprom->opcode, eeprom->address);
					eeprom->state = EEPROM_STATE_IDLE;
					return;
					break;
				}

			}
			break;
		case EEPROM_STATE_WRITING_MEMORY:
			eeprom->bitcount++;
			DEBUG_WARN("EERPROM_STATE_WRITTING_MEMORY bitcount[%d]\n", eeprom->bitcount);
			if (eeprom->bitcount < 5)
			{
				eeprom->pins.dataout = 0; 
				return;
			}

			if (eeprom->bitcount >= 5 && eeprom->bitcount <= 8 )
			{
				eeprom->pins.dataout = 1; 
				return;
			}

			if (eeprom->bitcount > 8)
			{
				eeprom->pins.dataout = 0;
				eeprom->state = EEPROM_STATE_IDLE;
				return;
			}
			break;
		case EEPROM_STATE_WRITTING_DATA:
			// flag as busy until we write the data to internal memory
			eeprom->pins.dataout = 0;

			if (!clock_rising_edge)
				return;

			eeprom->bitcount++;
			
			// only do this for the first 16 bits
			if (eeprom->bitcount <= 16)
			{
				eeprom->data <<= 1;
				if (eeprom->pins.datain == 1)
					eeprom->data |= 0x01;
			}

			if (eeprom->bitcount >= 16)
			{
				eeprom->memory[eeprom->address] = eeprom->data;
				eeprom->state = EEPROM_STATE_WRITING_MEMORY;
				eeprom->bitcount = 0;
				DEBUG_WARN("EEPROM_WRITE: recevied[%04X] address[%04X]\n",eeprom->data,eeprom->address);
				return;
			}

			break;
		case EEPROM_STATE_READING_DATA:
			// we update dataout on falling edge
			if (!clock_falling_edge)
				return;
			eeprom->bitcount++;
			/// we have a dummy bit before starting to update data
			if (eeprom->bitcount < 2)
			{
				eeprom->pins.dataout = 0;
				DEBUG_WARN(" O(D)");
				return;
			}

			// this should be sent from MSB(BIT15) to LSB(BIT0)
			if ( ((eeprom->memory[eeprom->address]) & (1<<(17-eeprom->bitcount))) != 0)
			{
				DEBUG_WARN(" O(1)");
				eeprom->pins.dataout = 1;
				eeprom->data = 1;
			}
			else
			{
				DEBUG_WARN(" O(0)");
				eeprom->pins.dataout = 0;
				eeprom->data = 0;
			}

			
			
			
			if (eeprom->bitcount == 17)
			{
				DEBUG_ERROR(" DATA SENT SHOULD BE [0x%04X] \n", eeprom->memory[eeprom->address]);
				eeprom->state = EEPROM_STATE_IDLE;
				return;
			}
			break;
		case EEPROM_STATE_READING_OPCODE:
			if (!clock_rising_edge)
				return;
			eeprom->bitcount++;
			eeprom->opcode <<= 1;

			if (eeprom->pins.datain == 1)
				eeprom->opcode |= 0x01;

			//
			if (eeprom->bitcount == 2)
			{
				// we should read address next
				eeprom->state = EEPROM_STATE_READING_ADDRESS;
				eeprom->bitcount = 0;
				DEBUG_WARN("EEPROM_STATE_READING_OPCODE: opcode %s[%02X] \n", from_opcode_to_string(eeprom->opcode),eeprom->opcode);
				return;
			}
			break;
		case EEPROM_STATE_IDLE:
		default:
			DEBUG_WARN("EEPROM_STATE_IDLE\n");
			// check for start bit events
			if (eeprom->pins.chipSelect == 1 && eeprom->pins.datain == 1 && clock_rising_edge)
			{
				eeprom->state = EEPROM_STATE_READING_OPCODE;

				// clear command vars
				eeprom->opcode = OPCODE_EXTENDED;
				eeprom->address = 0x0000;
				eeprom->data = 0x0000;
				eeprom->bitcount = 0;
				eeprom->pins.dataout = 1;
				eeprom->pins.datain = 0;
				return;
			}
			break;
		}
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