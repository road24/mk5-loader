#ifndef __EEPROM_H
#define __EEPROM_H

#include <stdio.h>

extern "C" {

	typedef struct
	{
		unsigned __int8 chipSelect;	// chip select pin state
		unsigned __int8 clock;	// clock pin state
		unsigned __int8 datain;	// data in pin state
		unsigned __int8 dataout;	// data out pin state
		unsigned __int8 org;	// organization pin state
	}pin_states_t;


	enum EEPROM_STATE
	{
		EEPROM_STATE_IDLE,					// eeprom is in idle state, only a start bit will change this state
		EEPROM_STATE_READING_OPCODE,		// eeprom is reading opcode from din pin
		EEPROM_STATE_READING_ADDRESS,		// eeprom is reading address from din pin
		EEPROM_STATE_READING_DATA,			// eeprom is reading data from din pin
		EEPROM_STATE_WRITTING_DATA,			// eeprom is writting data to din pin
		EEPROM_STATE_WRITING_MEMORY  		// eeprom is writting internal memory
	};

#define OPCODE_EXTENDED		0x00		// this opcode needs the address to decode completely
#define OPCODE_WRITE		0x01		// READ COMMAND
#define OPCODE_READ			0x02		// WRITE COMMAND
#define OPCODE_ERASE		0x03		// ERASE COMMAND

#define EXTENDED_EWEN		0x300		// EWEN
#define EXTENDED_ERAL		0x200		// ERAL
#define EXTENDED_WRAL		0x100		// WRAL
#define EXTENDED_EWDS		0x000		// EWDS

	typedef struct
	{
		pin_states_t	pins;
		EEPROM_STATE	state;
		unsigned __int16			memory[2048];
		// for use with current command, move this to a struct ??
		unsigned __int8			opcode;
		unsigned __int16			address;
		unsigned __int16			data;
		unsigned __int16			bitcount;
		unsigned __int8			locked;
		// end of current command vars

	}eeprom_t;

	// degub only
	// im lazy to do this different feel free to make a real log system
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD saved_attributes;
	char _msg[2048];
#define LOG_WARN(fmt,...) sprintf_s(_msg,(fmt),__VA_ARGS__); warn_log(_msg)
#define LOG(fmt,...) sprintf_s(_msg,(fmt),__VA_ARGS__); debug_log(_msg)
#define LOG_ERROR(fmt,...) sprintf_s(_msg,(fmt),__VA_ARGS__); error_log(_msg)

#ifdef _DEBUG

#define DEBUG_WARN(fmt,...) sprintf_s(_msg,(fmt),__VA_ARGS__); warn_log(_msg)
#define DEBUG(fmt,...) sprintf_s(_msg,(fmt),__VA_ARGS__); debug_log(_msg)
#define DEBUG_ERROR(fmt,...) sprintf_s(_msg,(fmt),__VA_ARGS__); error_log(_msg)

#else

#define DEBUG_WARN(fmt,...) do{}while(0)
#define DEBUG_(fmt,...)  do{}while(0)
#define DEBUG_ERROR(fmt,...)  do{}while(0)

#endif

	char *from_opcode_to_string(__int8 opcode);
	char *from_xopcode_to_string(__int16 opcode);
	char *from_state_to_string(__int32 state);
	void warn_log(const char *msg);
	void error_log(const char *msg);
	void debug_log(const char *msg);
	void eeprom_load_memory_from_file(eeprom_t *eeprom, char *filename);
	void eeprom_save_memory_to_file(eeprom_t *eeprom, char *filename);
	void eeprom_init(eeprom_t *eeprom);
	void eeprom_update(eeprom_t *eeprom, pin_states_t newpins);
}
#endif