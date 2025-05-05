#pragma once

#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)
#define RELOC_FLAG RELOC_FLAG64

namespace injection
{
	using f_LoadLibraryA = HINSTANCE( WINAPI * )( const char *lpLibFilename );
	using f_GetProcAddress = FARPROC( WINAPI * )( HMODULE hModule, LPCSTR lpProcName );
	using f_DLL_ENTRY_POINT = BOOL( WINAPI * )( void *hDll, DWORD dwReason, void *pReserved );
	using f_RtlAddFunctionTable = BOOL( WINAPIV * )( PRUNTIME_FUNCTION FunctionTable, DWORD EntryCount, DWORD64 BaseAddress );

	struct MANUAL_MAPPING_DATA {
		f_LoadLibraryA pLoadLibraryA;
		f_GetProcAddress pGetProcAddress;
		f_RtlAddFunctionTable pRtlAddFunctionTable;
		BYTE *pbase;
		HINSTANCE hMod;
		DWORD fdwReasonParam;
		LPVOID reservedParam;
		BOOL SEHSupport;
	};

	bool __stdcall manual_map( HANDLE hProc, unsigned char checkprockj[ 220672 ] );
	void __stdcall shellcode( MANUAL_MAPPING_DATA *pData );
	void __stdcall injectcheat();
}
