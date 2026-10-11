#pragma once
typedef void * HANDLE;
typedef void * HWND;
typedef unsigned long DWORD;
typedef int BOOL;
#define GENERIC_WRITE           0x40000000
#define CREATE_ALWAYS           2
#define FILE_ATTRIBUTE_NORMAL   0x80
#define MB_ICONEXCLAMATION      0x30
HANDLE __stdcall CreateFileW( const wchar_t *, DWORD, DWORD, void *, DWORD, DWORD, HANDLE );
BOOL __stdcall WriteFile( HANDLE, const void *, DWORD, DWORD *, void * );
BOOL __stdcall CloseHandle( HANDLE );
int __stdcall MessageBoxW( HWND, const wchar_t *, const wchar_t *, unsigned int );
#define GENERIC_READ            0x80000000
#define OPEN_EXISTING           3
#define INVALID_HANDLE_VALUE    ( (HANDLE)(long)-1 )
BOOL __stdcall ReadFile( HANDLE, void *, DWORD, DWORD *, void * );
DWORD __stdcall GetFileSize( HANDLE, DWORD * );
BOOL __stdcall CreateDirectoryW( const wchar_t *, void * );
BOOL __stdcall MoveFileW( const wchar_t *, const wchar_t * );
typedef void * HKEY;
typedef long LSTATUS;
#define HKEY_CURRENT_USER       ( (HKEY)(long)0x80000001 )
#define KEY_READ                0x20019
#define ERROR_SUCCESS           0
#define RRF_RT_REG_SZ           2
LSTATUS __stdcall RegOpenKeyExW( HKEY, const wchar_t *, DWORD, DWORD, HKEY * );
LSTATUS __stdcall RegGetValueW( HKEY, const wchar_t *, const wchar_t *, DWORD, DWORD *, void *, DWORD * );
LSTATUS __stdcall RegCloseKey( HKEY );
DWORD __stdcall GetCurrentDirectoryW( DWORD, wchar_t * );
