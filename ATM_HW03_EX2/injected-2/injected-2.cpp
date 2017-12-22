// injected-2.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"

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
		std::cout << "DLL name: " << name << std::endl;

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
					std::cout << "Found " << functionName << std::endl;
					return (void**)(pCurrentThunk + i);
				}
				std::cout << "Function name: " << functionName << std::endl;
			}
		}
	}
};



int WINAPI MyMaliciousFunction(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType)
{
	std::cout << "PWND" << std::endl;
	return 0;
}

void PatchIAT(char const* const wantedFunctionName, char const* const wantedDllName) {
	void** pFunc = getFunctionAddress(wantedFunctionName, wantedDllName); //Get pointer to MessageBoxA function
	DWORD old_rights; //Variable to preserve old rights of page

	VirtualProtect(pFunc, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_rights); // Change Page permissions
	*pFunc = MyMaliciousFunction; //Replace functions
	VirtualProtect(pFunc, sizeof(void*), old_rights, NULL); //Preserve old rights of page
};