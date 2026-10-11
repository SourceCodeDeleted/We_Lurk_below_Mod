//-----------------------------------------------------------------------------
// ec_debug_log.h
//
// In-memory log of up to 4096 lines, written to debug_log.txt on request.

#ifndef _EC_DEBUG_LOG_H_
#define _EC_DEBUG_LOG_H_

void debug_log_start();
void debug_log_add_line( const wchar_t * line );
void debug_log_format_line( const wchar_t * format, ... );
void debug_log_add_char_str( const char * line );
void debug_log_save( bool free_log );

#endif
