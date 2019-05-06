// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"

#include <stdio.h>
#include <windows.h>

//#define __20030808_BUILD
//#define __X_3_1
#ifdef __20030808_BUILD

#define DSOUNDCREATE_JUMP_OFFSET 0x00436018
#define OUTBYTE_OFFSET			 0x0041C400
#define INWORD_OFFSET			 0x0041C410
#define OUTWORD_OFFSET			 0x0041C430
#define EEPROM_WRITE_OFFSET		 0x0041BB80
#define EEPROM_READ_OFFSET		 0x0041BC00
#define GIVEIO_BYPASS_OFFSET	 0x0041D8BB
#define AUD_DEADLOCK_FIX		 0x0041CB57

#elif defined(__X_3_1)

#define DSOUNDCREATE_JUMP_OFFSET 0x00437018
#define OUTBYTE_OFFSET			 0x0041CBB0
#define INWORD_OFFSET			 0x0041CBC0
#define OUTWORD_OFFSET			 0x0041CBE0
#define EEPROM_WRITE_OFFSET		 0x0041C330
#define EEPROM_READ_OFFSET		 0x0041C3B0
#define LOCK_CHECK1_OFFSET		 0x00417A06
#define LOCK_CHECK2_OFFSET		 0x00410D60
#define LOCK_CHECK3_OFFSET		 0x00417A06
#define GIVEIO_BYPASS_OFFSET	 0x0041E42B
#define AUD_DEADLOCK_FIX		 0x0041D6E7

#else

#define DSOUNDCREATE_JUMP_OFFSET 0x00437018
#define OUTBYTE_OFFSET			 0x0041C9D0
#define INWORD_OFFSET			 0x0041C9E0
#define OUTWORD_OFFSET			 0x0041CA00
#define EEPROM_WRITE_OFFSET		 0x0041C150
#define EEPROM_READ_OFFSET		 0x0041C1D0
#define LOCK_CHECK1_OFFSET		 0x00404543
#define LOCK_CHECK2_OFFSET		 0x00410CB0
#define LOCK_CHECK3_OFFSET		 0x00417826
#define GIVEIO_BYPASS_OFFSET	 0x0041E1DB
#define AUD_DEADLOCK_FIX		 0x0041D477

#endif


#define DETOUR(ADDRESS,NEW_ADDRESS)\
	do{\
		if (NEW_ADDRESS == 0) MessageBox(0, TEXT("Failed to load "TEXT(#ADDRESS)), TEXT("PIU32 LOADER"), MB_ICONERROR);\
		*(BYTE *)(ADDRESS) = 0xE9;\
		*(UINT32 *)(ADDRESS+1) = (UINT32)(NEW_ADDRESS) - ADDRESS - 5;\
	}while(0)


#include <strsafe.h>

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}




BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	// library handler
	HINSTANCE dllHandle = NULL;
	HINSTANCE dllIoHook = NULL;

	if (reason == DLL_PROCESS_ATTACH)
	{
		AllocConsole();
		printf("fixing wrong IAT patch \n");
		dllHandle = LoadLibrary(TEXT("C:\\Windows\\System32\\dsound.dll"));
		if (dllHandle == NULL)
		{
			// failed to load DLL
			MessageBox(0, TEXT("Failed to load original dsound.dll"), TEXT("PIU32 LOADER"), MB_ICONERROR);
			return FALSE;	// failed to load DLL
		}

		// unprotect the exe
		
		auto hExecutableInstance = (size_t)GetModuleHandle(NULL);
		IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)(hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
		SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
		DWORD oldProtect;
		VirtualProtect((VOID*)hExecutableInstance, size, PAGE_EXECUTE_READWRITE, &oldProtect);

		
		dllIoHook = LoadLibrary(TEXT("iohook.dll"));

		if (dllIoHook == NULL)
		{
			MessageBox(0, TEXT("Failed to load iohook.dll"), TEXT("PIU32 LOADER"), MB_ICONERROR);
		}

		// now patch our offset 
		
		// dsoundcreate workaround
		
		*(UINT32 *)(DSOUNDCREATE_JUMP_OFFSET) = (UINT32)GetProcAddress(dllHandle, "DirectSoundCreate");
		
		// outputword
		//*(BYTE *)(OUTWORD_OFFSET) = 0xFF;
		//*(BYTE *)(OUTWORD_OFFSET + 1) = 0x25;
		//*(BYTE *)(OUTWORD_OFFSET) = 0xE9;
		//UINT32 offset = (UINT32 )GetProcAddress(dllIoHook, "outword") - OUTWORD_OFFSET - 5;
		//if (offset == 0)
		//{
			//MessageBox(0, TEXT("Failed to load outword hook"), TEXT("PIU32 LOADER"), MB_ICONERROR);
		//	ErrorExit(TEXT("outword"));
		//}
		//*(UINT32 *)(OUTWORD_OFFSET + 1) = offset;
		DETOUR(OUTBYTE_OFFSET, (UINT32)GetProcAddress(dllIoHook, "outbyte"));
		DETOUR(OUTWORD_OFFSET, (UINT32)GetProcAddress(dllIoHook, "outword"));
		DETOUR(INWORD_OFFSET, (UINT32)GetProcAddress(dllIoHook, "inputword"));
		DETOUR(EEPROM_READ_OFFSET, (UINT32)GetProcAddress(dllIoHook, "eeprom_read"));
		DETOUR(EEPROM_WRITE_OFFSET, (UINT32)GetProcAddress(dllIoHook, "eeprom_write"));
		
#ifndef __20030808_BUILD
		// remove lockcheck to dump a better eeprom signal
		//*(BYTE *)LOCK_CHECK1_OFFSET = 0xB8; 
		//*(BYTE *)LOCK_CHECK2_OFFSET = 0xB8;
		//*(BYTE *)LOCK_CHECK3_OFFSET = 0xB8;
#endif
		// do not start giveio and also fix the "do not make illegal copy msg"
		*(BYTE *)GIVEIO_BYPASS_OFFSET   = 0xB8;
		*(UINT32 *)(GIVEIO_BYPASS_OFFSET+1) = 0x0001;

		// fix for sound deadlock
		*(BYTE *)AUD_DEADLOCK_FIX = 0x00;


	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		if ( dllHandle != NULL )
		{
			FreeLibrary(dllHandle);
			dllHandle = NULL;
		}

		if (dllIoHook != NULL)
		{
			FreeLibrary(dllIoHook);
			dllIoHook = NULL;
		}

	}
	return TRUE;
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