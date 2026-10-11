//-----------------------------------------------------------------------------
// ec_excel.h
//
// Data tables. Each table is described by an excel_parse_table: the tab-separated text export of
// the spreadsheet, its pre-built binary form (.gbe, an array of the destination structs), and a
// column list mapping spreadsheet columns onto struct fields.
//
// The excel_data_* macros and excel_table_define are named as in the 2019 source; their bodies,
// the excel_type_* names and the struct field names (apart from all_categories, id_category,
// num_entries, all_cells, destination_data, taken from assert text) are reconstructions.

#ifndef _EC_EXCEL_H_
#define _EC_EXCEL_H_

#define max_excel_word_size         2048
#define max_excel_cell_size         1024

enum
{
    excel_type_safestring = 0,      // quoted text -> safestring
    excel_type_string = 1,          // text copied as a C string (row names are char[64])
    excel_type_char = 2,            // first character of the cell
    excel_type_int = 3,
    excel_type_float = 4,
    excel_type_byte = 5,
    excel_type_flag = 6,            // "1" sets bit number `value` in a dword
    excel_type_bool = 7,
    excel_type_link = 8,            // row name of another table; not handled by the text reader
    excel_type_int64 = 10,
};

struct excel_data               // one column, 80 bytes
{
    char        name[ 64 ];
    int         type;
    int         offset;         // of the field in the destination struct
    int         value;          // default when the cell is empty; bit number for excel_type_flag
    void *      link;           // excel_type_link: the other table's definitions pointer
};

struct excel_cell               // 1028 bytes
{
    char            text[ max_excel_cell_size ];
    excel_cell *    next;       // same column, next row
};

struct excel_category           // one column of the text file, 1032 bytes
{
    char                name[ max_excel_cell_size ];
    excel_cell *        first_cell;
    excel_category *    next;
};

struct excel_parse_table        // 168 bytes
{
    char                txt_filename[ 64 ];
    char                gbe_filename[ 64 ];
    int                 num_entries;
    excel_category *    first_category;
    excel_category *    id_category;
    excel_data *        data_list;
    int                 num_data;
    excel_category *    all_categories;
    excel_cell *        all_cells;
    byte **             destination_data;
    int                 destination_size;       // of one record
    int *               num_destination;
};

#define excel_data_string( name, type, field )          { name, excel_type_string, offsetof( type, field ), 0, NULL }
#define excel_data_int( name, type, field, value )      { name, excel_type_int, offsetof( type, field ), value, NULL }
#define excel_data_float( name, type, field )           { name, excel_type_float, offsetof( type, field ), 0, NULL }
#define excel_data_bool( name, type, field )            { name, excel_type_bool, offsetof( type, field ), 0, NULL }
#define excel_data_link( name, type, field, table )     { name, excel_type_link, offsetof( type, field ), sizeof( **( &table ) ), &table }

#define excel_table_define( txt, gbe, list, definitions, num ) \
    { txt, gbe, 0, NULL, NULL, list, sizeof( list ) / sizeof( excel_data ), NULL, NULL, (byte **)&definitions, sizeof( *definitions ), &num }

int get_next_word_in_line( byte * source, byte * source_end, char * dest );
void read_data_excel_file( excel_parse_table * table );
void read_binary_excel_file( excel_parse_table * table );
void free_data_excel_file( excel_parse_table * table );
void free_binary_excel_file( excel_parse_table * table );

#endif
