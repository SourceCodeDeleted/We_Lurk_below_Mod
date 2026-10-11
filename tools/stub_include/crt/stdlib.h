#pragma once
#include <string.h>
void * malloc( size_t );
void free( void * );
wchar_t * _wgetenv( const wchar_t * );
int atoi( const char * );
double atof( const char * );
long long _atoi64( const char * );
int rand();
