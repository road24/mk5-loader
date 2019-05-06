// El siguiente bloque ifdef muestra la forma estándar de crear macros que facilitan 
// la exportación de archivos DLL. Todos los archivos de este archivo DLL se compilan con el símbolo IOHOOK_EXPORTS
// definido en la línea de comandos. Este símbolo no debe definirse en ningún proyecto
// que use este archivo DLL. De este modo, otros proyectos cuyos archivos de código fuente incluyan el archivo 
// interpretan que las funciones IOHOOK_API se importan de un archivo DLL, mientras que este archivo DLL interpreta los símbolos
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