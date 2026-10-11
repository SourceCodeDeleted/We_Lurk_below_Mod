#pragma once
#include <string.h>
#include <stdarg.h>
size_t wcslen( const wchar_t * );
wchar_t * wcscpy( wchar_t *, const wchar_t * );
int swprintf( wchar_t *, size_t, const wchar_t *, ... );
int vswprintf( wchar_t *, size_t, const wchar_t *, va_list );
