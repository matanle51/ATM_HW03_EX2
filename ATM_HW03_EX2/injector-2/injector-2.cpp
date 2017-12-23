#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include <string>

using namespace std;

constexpr char injectedDllName[15] = "injected-2.dll";

int main(int argc, char **argv)
{
	// Check for correct number of arguments - 1 argument is excepted
	if (argc != 2) {
		OutputDebugStringA("[Error] Wrong number of arguments");
		return 0;
	}

	// Get DLL full path (in current directory)
	char workingDir[MAX_PATH];
	if (!GetModuleFileNameA(NULL, workingDir, MAX_PATH))
		return -1;

	string injectedDLLPath(workingDir);
	int const found = injectedDLLPath.find_last_of("\\");
	injectedDLLPath.replace(found + 1, 14, injectedDllName);
	
	OutputDebugStringA(injectedDLLPath.c_str());
	// Get pid of the victim process from the given argument
	int const pid = atoi(argv[1]);

	// Get the address of "LoadLibraryA" of current process
	DWORD const pLoadLibraryA = (DWORD)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
	if (pLoadLibraryA == NULL) {
		OutputDebugStringA("[Error] Cannot find LoadLibraryA address in kernel32.dll");
		return 0;
	}

	HANDLE const rProcessHandle = 
		// Allocate memory on the virtual space of the remote process
		OpenProcess(PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE 
			, FALSE, (DWORD)pid); // Process security attributes are required for the createRemoteThread hProcess parameter
	if (rProcessHandle == NULL) {
		OutputDebugStringA("[Error] Cannot get handle to remote process with PID: " + pid);
		return 0;
	}

	LPVOID const pDllName = VirtualAllocEx(rProcessHandle, NULL, strlen(injectedDLLPath.c_str()), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (pDllName == NULL) {
		OutputDebugStringA("[Error] Cannot allocate memory in remote process with PID: " + pid);
		return 0;
	}

	// Write DLL name to allocated memory in the virtual space of the remote process
	if (!WriteProcessMemory(rProcessHandle, pDllName, injectedDLLPath.c_str(), strlen(injectedDLLPath.c_str()), NULL)) {
		OutputDebugStringA("[Error] Cannot write memory in remote process with PID: " + pid);
		return 0;
	}

	OutputDebugStringA("[Info] Before CreateRemoteThread function");
	
	// Create thread in remote process with LoadLibraryA function as thread function
	if (!CreateRemoteThread(rProcessHandle, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pDllName, NULL, NULL)) {
		OutputDebugStringA((string("[Error] Cannot create thread in remote process with PID: " + pid) + string(", With Error Code: " + GetLastError())).c_str());
		return 0;
	}
	OutputDebugStringA("[Info] Thread Created Successfully");

	return 1;
}

