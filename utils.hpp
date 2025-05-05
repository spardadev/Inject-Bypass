#include "stdafx.hpp"

namespace utils
{

	namespace process
	{
		__inline DWORD get_id( )
		{
			DWORD pid {};
			const auto hwnd = FindWindowA( "LWJGL", nullptr );

			GetWindowThreadProcessId( hwnd, &pid );

			return pid;
		}

		__inline bool file_exists( const std::string &filePath )
		{
			DWORD fileAttributes = GetFileAttributesA( filePath.c_str( ) );
			return ( fileAttributes != INVALID_FILE_ATTRIBUTES && !( fileAttributes & FILE_ATTRIBUTE_DIRECTORY ) );
		}

		__inline size_t get_base_address( )
		{
			const auto handle = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, get_id( ) );

			if ( handle )
			{
				MODULEENTRY32 entry;
				entry.dwSize = sizeof( entry );

				if ( Module32First( handle, &entry ) )
				{
					do
					{
						if ( !strcmp( entry.szModule, "jvm.dll" ) )
						{
							CloseHandle( handle );
							return reinterpret_cast< size_t >( entry.modBaseAddr );
						}
					} while ( Module32Next( handle, &entry ) );
				}
				CloseHandle( handle );
			}

			printf( "error while creating handle or module name [ %s ] is incorrect", "javaw.exe" );
		}		

		__inline bool get_privileges( const char *privileges )
		{
			HANDLE token_handle {};
			if ( !OpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle ) )
				return false;

			LUID identifier {};
			if ( !LookupPrivilegeValue( NULL, privileges, &identifier ) )
				return false;

			TOKEN_PRIVILEGES token_privilege {};

			token_privilege.PrivilegeCount = 1;
			token_privilege.Privileges[ 0 ].Luid = identifier;
			token_privilege.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

			if ( !AdjustTokenPrivileges( token_handle, 0, &token_privilege, sizeof( TOKEN_PRIVILEGES ), nullptr, nullptr ) )
				return false;

			CloseHandle( token_handle );
			return true;
		}
	}

}
