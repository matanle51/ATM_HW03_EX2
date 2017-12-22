
// #include <stdio.h>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <string>
#include <algorithm>
using namespace std;

const char* injectedDllName = "injected-2.dll";

int main(int argc, char **argv)
{
	char debugBuf[256];

	// Check for correct number of arguments - 1 argument is excepted
	if (argc != 2) {
		sprintf_s(debugBuf, 256, "[Error] Wrong number of arguments");
		OutputDebugStringA(debugBuf);
		return 0;
	}

	// Get DLL full path (in current directory)
	string injectedDLLPath(argv[0]);
	int found = injectedDLLPath.find_last_of("\\");
	injectedDLLPath.replace(found + 1, 14, injectedDllName);
	//printf("%s", injectedDLLPath.c_str());
	//sprintf_s(debugBuf, 256, injectedDLLPath.c_str());
	OutputDebugStringA(injectedDLLPath.c_str());
	// Get pid of the victim process from the given argument
	int pid = atoi(argv[1]);

	// Get the address of "LoadLibraryA" of current process
	DWORD pLoadLibraryA = (DWORD)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
	if (pLoadLibraryA == NULL) {
		sprintf_s(debugBuf, 256, "[Error] Cannot find LoadLibraryA address in kernel32.dll");
		OutputDebugStringA(debugBuf);
		return 0;
	}

	// Allocate memory on the virtual space of the remote process
	DWORD processSecurityAttributes = PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE;
	HANDLE rProcessHandle = OpenProcess(processSecurityAttributes, FALSE, (DWORD)pid); // Process security attributes are required for the createRemoteThread hProcess parameter
	if (rProcessHandle == NULL) {
		sprintf_s(debugBuf, 256, "[Error] Cannot get handle to remote process with PID: %d", pid);
		OutputDebugStringA(debugBuf);
		return 0;
	}

	LPVOID pDllName = VirtualAllocEx(rProcessHandle, NULL, strlen(injectedDLLPath.c_str()), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (pDllName == NULL) {
		sprintf_s(debugBuf, 256, "[Error] Cannot allocate memory in remote process with PID: %d", pid);
		OutputDebugStringA(debugBuf);
		return 0;
	}

	// Write DLL name to allocated memory in the virtual space of the remote process
	if (!WriteProcessMemory(rProcessHandle, pDllName, injectedDLLPath.c_str(), strlen(injectedDLLPath.c_str()), NULL)) {
		sprintf_s(debugBuf, 256, "[Error] Cannot write memory in remote process with PID: %d", pid);
		OutputDebugStringA(debugBuf);
		return 0;
	}

	// Create thread in remote process with LoadLibraryA function as thread function
	HANDLE hThread = NULL;
	sprintf_s(debugBuf, 256, "[Info] Before CreateRemoteThread function");
	OutputDebugStringA(debugBuf);

	hThread = CreateRemoteThread(rProcessHandle, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pDllName, NULL, NULL);
	if (hThread == NULL) {
		sprintf_s(debugBuf, 256, "[Error] Cannot create thread in remote process with PID: %d ,With Error Code: %d", pid, GetLastError());
		OutputDebugStringA(debugBuf);
		return 0;
	}
	sprintf_s(debugBuf, 256, "[Info] Thread Created Successfully");
	OutputDebugStringA(debugBuf);

	return 1;
}

