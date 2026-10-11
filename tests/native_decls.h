// Force-included when src/ is built natively for the tests: declarations MSVC's runtime provides.
#pragma once
#include <wchar.h>
wchar_t * _wgetenv( const wchar_t * name );
