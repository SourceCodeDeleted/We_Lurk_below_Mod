#pragma once
typedef unsigned int size_t;
#ifndef NULL
#define NULL 0
#endif
size_t strlen( const char * );
char * strcat( char *, const char * );
char * strcpy( char *, const char * );
int strcmp( const char *, const char * );
void * memset( void *, int, size_t );
void * memcpy( void *, const void *, size_t );
int _stricmp( const char *, const char * );
