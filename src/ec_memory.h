//-----------------------------------------------------------------------------
// ec_memory.h
//
// Tracked allocator: every allocation is recorded with file, line and an info string, and
// anything still allocated at shutdown is reported in a message box.

#ifndef _EC_MEMORY_H_
#define _EC_MEMORY_H_

void memory_system_init();
void memory_system_close();
void * drb_malloc( dword size, const char * file, int line, const char * info );
void drb_free( void * p );

#endif
