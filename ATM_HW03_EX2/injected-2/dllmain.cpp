// dllmain.cpp : Defines the entry point for the DLL application.
#pragma once
#include "stdafx.h"
#include <stdlib.h>
#include <strsafe.h>
#include <iostream>
#include "injected-2.h"

using namespace std;

const wstring changedTitle = L"NOTEPAD is PWNED by Matan, David and Tom";

// This struct is a tuple of two parameters for EnumWindowsProc 
typedef struct EnumWindowsProcParam {
	DWORD pid;
	wstring title;
} EnumWindowsProcParam;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	EnumWindowsProcParam params = *(EnumWindowsProcParam*)lParam;
	int pid;
	GetWindowThreadProcessId(hwnd, (LPDWORD)&pid); // Set pid variable to PID suitable to current window

												   // Check if current PID is the wanted PID
	if (pid == params.pid) {
		if (!SetWindowText(hwnd, params.title.c_str()))
		{
			OutputDebugStringA("[Error] SetWindowText error");
			return 1;
		}
		return 0; // Return false - Do not keep running
	}
	return 1;	// Return true - Keep looking for correct window
}

void changeWindowText(wstring text_to_set) {
	DWORD pid = GetCurrentProcessId(); // Get current pid
	EnumWindowsProcParam lParam = { pid, text_to_set };
	EnumWindows(EnumWindowsProc, (LPARAM)&lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)

{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// A process is loading the DLL.
		PatchIAT("MessageBoxW", "user32.dll");

		MessageBox(NULL, _T("Advanced topics in malware"), _T("IAT patch"), MB_OK);
		std::cout << "Done" << std::endl;
		OutputDebugStringA("[INFO] Process is loading the DLL");
		changeWindowText(changedTitle);
		OutputDebugStringA("[INFO] Window text has changed");
		OutputDebugStringA("[INFO] Started IAT patching");

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