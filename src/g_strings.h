//-----------------------------------------------------------------------------
// g_strings.h
//
// The strings table (xls\strings.txt). Placeholder header: the original declares this table
// through the generated strings_hdr.h; the layout is from the exe's excel parse table.

#ifndef _G_STRINGS_H_
#define _G_STRINGS_H_

struct string_definition_data
{
    char        string_token[ 64 ];
    safestring  string;
};

extern string_definition_data * string_definitions;
extern int num_string_definitions;

#endif
