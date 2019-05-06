// El siguiente bloque ifdef muestra la forma est�ndar de crear macros que facilitan 
// la exportaci�n de archivos DLL. Todos los archivos de este archivo DLL se compilan con el s�mbolo IOHOOK_EXPORTS
// definido en la l�nea de comandos. Este s�mbolo no debe definirse en ning�n proyecto
// que use este archivo DLL. De este modo, otros proyectos cuyos archivos de c�digo fuente incluyan el archivo 
// interpretan que las funciones IOHOOK_API se importan de un archivo DLL, mientras que este archivo DLL interpreta los s�mbolos
// definidos en esta macro como si fueran exportados.
#ifdef IOHOOK_EXPORTS
#define IOHOOK_API __declspec(dllexport)
#else
#define IOHOOK_API __declspec(dllimport)
#endif
extern "C"
{
	IOHOOK_API signed int __cdecl eeprom_write(char *buffer, signed int bytes);
	IOHOOK_API signed int __cdecl eeprom_read(char *buffer, signed int bytes);
	IOHOOK_API unsigned __int8  __cdecl outbyte(unsigned __int16 port, unsigned __int8 cmd);
	IOHOOK_API unsigned __int16 __cdecl outword(unsigned __int16 port, unsigned __int16 cmd);
	IOHOOK_API unsigned __int16 __cdecl inputword(unsigned __int16 port);
	int initIO(void);
	int closeIO(void);
}
