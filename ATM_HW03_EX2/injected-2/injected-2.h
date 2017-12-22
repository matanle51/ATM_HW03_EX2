#pragma once

#include "stdafx.h"

#include <windows.h>
#include "tchar.h"
#include <iostream>

void** getFunctionAddress(char const* wantedFunctionName, char const* wantedDllName);
int WINAPI MyMaliciousFunction(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType);
void PatchIAT(char const* const wantedFunctionName, char const* const wantedDllName);