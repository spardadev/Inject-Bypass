#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <TlHelp32.h>
#include <winuser.h>
#include <conio.h>
#include <winternl.h>
#include <winioctl.h>

#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>


#include "utils.hpp"
#include <Windows.h>
#include <Urlmon.h>
#include <iostream>

#pragma comment(lib, "urlmon.lib")

#define NT_SUCCESS(x) ((NTSTATUS)(x) >= 0)

extern "C" NTSTATUS __stdcall NtReadVirtualMemory(HANDLE, PVOID, PVOID, ULONG, PULONG);
extern "C" NTSTATUS __stdcall NtWriteVirtualMemory(HANDLE, PVOID, PVOID, ULONG, PULONG);
extern "C" NTSTATUS __stdcall NtTerminateProcess(HANDLE, NTSTATUS);
extern "C" NTSTATUS __stdcall NtGetContextThread(HANDLE, PCONTEXT);
extern "C" NTSTATUS __stdcall NtSetContextThread(HANDLE, PCONTEXT);
extern "C" NTSTATUS __stdcall NtUnmapViewOfSection(HANDLE, PVOID);
extern "C" NTSTATUS __stdcall NtResumeThread(HANDLE, PULONG);
