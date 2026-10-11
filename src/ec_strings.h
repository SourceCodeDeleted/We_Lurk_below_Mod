//-----------------------------------------------------------------------------
// ec_strings.h
//
// safestring: fixed-size, length-prefixed wide string used everywhere in the game (2052 bytes).

#ifndef _EC_STRINGS_H_
#define _EC_STRINGS_H_

#define max_string_size     1024

struct safestring
{
    int         length;
    wchar_t     text[ max_string_size ];
};

// scratch string shared by the whole program (0x60B960); its original name is not known
extern safestring temp_string;

void safestring_create_from_id( safestring * dest, const dword string_token_id );
void safestring_create( safestring * dest, const wchar_t * src );
void safestring_create_from_char( safestring * dest, const char * src );
void safestring_copy( safestring * dest, const safestring * src );
void safestring_concat( safestring * dest, const safestring * src );
void safestring_concat_str( safestring * dest, const wchar_t * src );
void safestring_concat_charstr( safestring * dest, const char * src );
void safestring_concat_from_id( safestring * dest, const dword string_token_id );
void safestring_concat_int( safestring * dest, const int value );
void safestring_concat_char( safestring * dest, const wchar_t c );
void safestring_concat_float1( safestring * dest, const float value );
void safestring_concat_float2( safestring * dest, const float value );
void safestring_concat_key( safestring * dest, const dword key, bool add_quote );
void safestring_truncate( safestring * dest, const int new_length );
bool safestring_compare( safestring * a, safestring * b, bool ignore_case );
void safestring_to_char( safestring * src, char * dest );

#endif
