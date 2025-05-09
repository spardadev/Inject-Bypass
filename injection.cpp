#include "stdafx.hpp"
#include "injection.hpp"
#include "bytes.h"

namespace injection
{
	bool manual_map( HANDLE hProc, unsigned char checkprockj[ 220672 ] )
	{
		BYTE *pSrcData = reinterpret_cast< BYTE * >( checkprockj );
		IMAGE_NT_HEADERS *pOldNtHeader = nullptr;
		IMAGE_OPTIONAL_HEADER *pOldOptHeader = nullptr;
		IMAGE_FILE_HEADER *pOldFileHeader = nullptr;
		BYTE *pTargetBase = nullptr;

		if ( reinterpret_cast< IMAGE_DOS_HEADER * >( pSrcData )->e_magic != 0x5A4D )
			return false;

		pOldNtHeader = reinterpret_cast< IMAGE_NT_HEADERS * >( pSrcData + reinterpret_cast< IMAGE_DOS_HEADER * >( pSrcData )->e_lfanew );
		pOldOptHeader = &pOldNtHeader->OptionalHeader;
		pOldFileHeader = &pOldNtHeader->FileHeader;

		pTargetBase = reinterpret_cast< BYTE * >( VirtualAllocEx( hProc, nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
		if ( !pTargetBase )
			return false;

		DWORD oldp = 0;
		VirtualProtectEx( hProc, pTargetBase, pOldOptHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &oldp );

		MANUAL_MAPPING_DATA data { 0 };
		data.pLoadLibraryA = LoadLibraryA;
		data.pGetProcAddress = GetProcAddress;
		data.pRtlAddFunctionTable = ( f_RtlAddFunctionTable ) RtlAddFunctionTable;
		data.pbase = pTargetBase;
		data.fdwReasonParam = DLL_PROCESS_ATTACH;
		data.reservedParam = 0;
		data.SEHSupport = true;

		if ( !WriteProcessMemory( hProc, pTargetBase, pSrcData, 0x1000, nullptr ) )
		{
			VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
			return false;
		}

		IMAGE_SECTION_HEADER *pSectionHeader = IMAGE_FIRST_SECTION( pOldNtHeader );
		for ( UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader )
		{
			if ( pSectionHeader->SizeOfRawData )
			{
				if ( !WriteProcessMemory( hProc, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr ) )
				{
					VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
					return false;
				}
			}
		}

		BYTE *MappingDataAlloc = reinterpret_cast< BYTE * >( VirtualAllocEx( hProc, nullptr, sizeof( MANUAL_MAPPING_DATA ), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
		if ( !MappingDataAlloc )
		{
			VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
			return false;
		}

		if ( !WriteProcessMemory( hProc, MappingDataAlloc, &data, sizeof( MANUAL_MAPPING_DATA ), nullptr ) )
		{
			VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
			VirtualFreeEx( hProc, MappingDataAlloc, 0, MEM_RELEASE );
			return false;
		}

		void *pShellcode = VirtualAllocEx( hProc, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
		if ( !pShellcode )
		{
			VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
			VirtualFreeEx( hProc, MappingDataAlloc, 0, MEM_RELEASE );
			return false;
		}

		if ( !WriteProcessMemory( hProc, pShellcode, shellcode, 0x1000, nullptr ) )
		{
			VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
			VirtualFreeEx( hProc, MappingDataAlloc, 0, MEM_RELEASE );
			VirtualFreeEx( hProc, pShellcode, 0, MEM_RELEASE );
			return false;
		}

		HANDLE hThread = CreateRemoteThread( hProc, nullptr, 0, reinterpret_cast< LPTHREAD_START_ROUTINE >( pShellcode ), MappingDataAlloc, 0, nullptr );
		if ( !hThread )
		{
			VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
			VirtualFreeEx( hProc, MappingDataAlloc, 0, MEM_RELEASE );
			VirtualFreeEx( hProc, pShellcode, 0, MEM_RELEASE );
			return false;
		}
		CloseHandle( hThread );

		HINSTANCE hCheck = NULL;
		while ( !hCheck )
		{
			DWORD exitcode = 0;
			GetExitCodeProcess( hProc, &exitcode );
			if ( exitcode == STILL_ACTIVE )
				return false;

			MANUAL_MAPPING_DATA data_checked { 0 };
			ReadProcessMemory( hProc, MappingDataAlloc, &data_checked, sizeof( data_checked ), nullptr );
			hCheck = data_checked.hMod;

			if ( hCheck == ( HINSTANCE ) 0x404040 )
			{
				VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
				VirtualFreeEx( hProc, MappingDataAlloc, 0, MEM_RELEASE );
				VirtualFreeEx( hProc, pShellcode, 0, MEM_RELEASE );
				return false;
			}

			Sleep( 10 );
		}

		BYTE *emptyBuffer = ( BYTE * ) malloc( 1024 * 1024 * 20 );
		if ( emptyBuffer == nullptr )
			return false;

		memset( emptyBuffer, 0, 1024 * 1024 * 20 );
		WriteProcessMemory( hProc, pTargetBase, emptyBuffer, 0x1000, nullptr );

		pSectionHeader = IMAGE_FIRST_SECTION( pOldNtHeader );
		for ( UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader )
		{
			if ( pSectionHeader->Misc.VirtualSize )
			{
				DWORD old = 0;
				DWORD newP = PAGE_READONLY;

				if ( ( pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE ) > 0 )
					newP = PAGE_READWRITE;

				else if ( ( pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE ) > 0 )
					newP = PAGE_EXECUTE_READ;

				VirtualProtectEx( hProc, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, newP, &old );
			}
		}

		DWORD old = 0;
		VirtualProtectEx( hProc, pTargetBase, IMAGE_FIRST_SECTION( pOldNtHeader )->VirtualAddress, PAGE_READONLY, &old );

		WriteProcessMemory( hProc, pShellcode, emptyBuffer, 0x1000, nullptr );
		VirtualFreeEx( hProc, pTargetBase, 0, MEM_RELEASE );
		VirtualFreeEx( hProc, pShellcode, 0, MEM_RELEASE );
		VirtualFreeEx( hProc, MappingDataAlloc, 0, MEM_RELEASE );

		return true;
	}

	void __stdcall shellcode( MANUAL_MAPPING_DATA *pData )
	{
		if ( !pData )
		{
			pData->hMod = ( HINSTANCE ) 0x404040;
			return;
		}

		BYTE *pBase = pData->pbase;
		auto *pOpt = &reinterpret_cast< IMAGE_NT_HEADERS * >( pBase + reinterpret_cast< IMAGE_DOS_HEADER * >( ( uintptr_t ) pBase )->e_lfanew )->OptionalHeader;

		auto _LoadLibraryA = pData->pLoadLibraryA;
		auto _GetProcAddress = pData->pGetProcAddress;
		auto _RtlAddFunctionTable = pData->pRtlAddFunctionTable;
		auto _DllMain = reinterpret_cast< f_DLL_ENTRY_POINT >( pBase + pOpt->AddressOfEntryPoint );

		BYTE *LocationDelta = pBase - pOpt->ImageBase;
		if ( LocationDelta )
		{
			if ( pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].Size )
			{
				auto *pRelocData = reinterpret_cast< IMAGE_BASE_RELOCATION * >( pBase + pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress );
				const auto *pRelocEnd = reinterpret_cast< IMAGE_BASE_RELOCATION * >( reinterpret_cast< uintptr_t >( pRelocData ) + pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].Size );
				while ( pRelocData < pRelocEnd && pRelocData->SizeOfBlock )
				{
					UINT AmountOfEntries = ( pRelocData->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( WORD );
					WORD *pRelativeInfo = reinterpret_cast< WORD * >( pRelocData + 1 );

					for ( UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo )
					{
						if ( RELOC_FLAG( *pRelativeInfo ) )
						{
							UINT_PTR *pPatch = reinterpret_cast< UINT_PTR * >( pBase + pRelocData->VirtualAddress + ( ( *pRelativeInfo ) & 0xFFF ) );
							*pPatch += reinterpret_cast< UINT_PTR >( LocationDelta );
						}
					}
					pRelocData = reinterpret_cast< IMAGE_BASE_RELOCATION * >( reinterpret_cast< BYTE * >( pRelocData ) + pRelocData->SizeOfBlock );
				}
			}
		}

		if ( pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].Size )
		{
			auto *pImportDescr = reinterpret_cast< IMAGE_IMPORT_DESCRIPTOR * >( pBase + pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );
			while ( pImportDescr->Name )
			{
				char *szMod = reinterpret_cast< char * >( pBase + pImportDescr->Name );
				HINSTANCE hDll = _LoadLibraryA( szMod );

				ULONG_PTR *pThunkRef = reinterpret_cast< ULONG_PTR * >( pBase + pImportDescr->OriginalFirstThunk );
				ULONG_PTR *pFuncRef = reinterpret_cast< ULONG_PTR * >( pBase + pImportDescr->FirstThunk );

				if ( !pThunkRef )
					pThunkRef = pFuncRef;

				for ( ; *pThunkRef; ++pThunkRef, ++pFuncRef )
				{
					if ( IMAGE_SNAP_BY_ORDINAL( *pThunkRef ) )
					{
						*pFuncRef = ( ULONG_PTR ) _GetProcAddress( hDll, reinterpret_cast< char * >( *pThunkRef & 0xFFFF ) );
					}
					else
					{
						auto *pImport = reinterpret_cast< IMAGE_IMPORT_BY_NAME * >( pBase + ( *pThunkRef ) );
						*pFuncRef = ( ULONG_PTR ) _GetProcAddress( hDll, pImport->Name );
					}
				}
				++pImportDescr;
			}
		}

		if ( pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_TLS ].Size )
		{
			auto *pTLS = reinterpret_cast< IMAGE_TLS_DIRECTORY * >( pBase + pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_TLS ].VirtualAddress );
			auto *pCallback = reinterpret_cast< PIMAGE_TLS_CALLBACK * >( pTLS->AddressOfCallBacks );
			for ( ; pCallback && *pCallback; ++pCallback )
				( *pCallback )( pBase, DLL_PROCESS_ATTACH, nullptr );
		}

		bool ExceptionSupportFailed = false;

		if ( pData->SEHSupport )
		{
			auto excep = pOpt->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ];
			if ( excep.Size )
			{
				if ( !_RtlAddFunctionTable( reinterpret_cast< IMAGE_RUNTIME_FUNCTION_ENTRY * >( pBase + excep.VirtualAddress ), excep.Size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY ), ( DWORD64 ) pBase ) )
				{
					ExceptionSupportFailed = true;
				}
			}
		}

		_DllMain( pBase, pData->fdwReasonParam, pData->reservedParam );

		if ( ExceptionSupportFailed )
			pData->hMod = reinterpret_cast< HINSTANCE >( 0x505050 );
		else
			pData->hMod = reinterpret_cast< HINSTANCE >( pBase );
	}

	void injectcheat()
	{
		char edge_path[] = R"(C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe)";

		if ((edge_path))
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			ZeroMemory(&pi, sizeof(pi));

			if (!CreateProcessA(NULL, edge_path, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
				return;

			CloseHandle(pi.hProcess);

			const auto hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.dwProcessId);
			if (!hProc)
				return;

			if (!manual_map(hProc, rawData))
				CloseHandle(hProc);
		}
		return;
	}
}
