//-----------------------------------------------------------------------------
// ec_excel.cpp
// Reconstructed from ILB.exe: get_next_word_in_line 0x404500, read_data_excel_file 0x4045B0 (the
// only copy is specialised for the strings table and has get_num_categories and get_num_lines
// inlined), free_data_excel_file 0x40A130. read_binary_excel_file and free_binary_excel_file exist
// only inlined, once per table, in the table loader at 0x57ECD0 and the shutdown code at 0x57F3B0.
//
// Original names: all functions except read_binary_excel_file (inferred from
// free_binary_excel_file), plus the names quoted in the asserts. Local variable names not quoted
// in an assert are chosen here.
//
// The shipped game reads every table from its .gbe file except strings, which goes through the
// text reader.

#include "main.h"
#include "ec_fileio.h"
#include "ec_excel.h"

//-----------------------------------------------------------------------------
// Copies characters up to the next control character (tab, CR) and returns how many were copied.
// An empty cell therefore returns 0, which every caller treats as an error.

int get_next_word_in_line( byte * source, byte * source_end, char * dest )
{
    if ( !source ) { assert_exp( source ); return -1; }
    if ( !source_end ) { assert_exp( source_end ); return -1; }
    if ( !( source_end > source ) ) { assert_exp( source_end > source ); return -1; }
    if ( !dest ) { assert_exp( dest ); return -1; }

    dest[ 0 ] = 0;

    int word_size = 0;
    while ( *source >= 0x20 && source < source_end )
    {
        if ( word_size >= max_excel_word_size )
        {
            dest[ 0 ] = 0;
            return word_size;
        }
        dest[ word_size++ ] = *source++;
    }

    if ( word_size >= max_excel_word_size )
    {
        dest[ 0 ] = 0;
        return word_size;
    }

    dest[ word_size ] = 0;
    return word_size;
}

//-----------------------------------------------------------------------------

static int get_num_categories( byte * source_text, byte * end_text )
{
    char word[ max_excel_word_size ];
    int num_categories = 0;

    for ( ;; )
    {
        int word_size = get_next_word_in_line( source_text, end_text, word );
        assert_return_zero( word_size > 0 );
        source_text += word_size;

        if ( word[ 0 ] != 0 ) num_categories++;

        if ( source_text >= end_text || *source_text != '\t' ) break;
        source_text++;
    }

    return num_categories;
}

//-----------------------------------------------------------------------------

static int get_num_lines( byte * source_text, byte * end_text )
{
    if ( !source_text ) { assert_exp( source_text ); return -1; }
    if ( !end_text ) { assert_exp( end_text ); return -1; }

    int num_lines = 0;
    while ( source_text < end_text )
    {
        if ( *source_text != 0 ) num_lines++;

        while ( *source_text != '\r' )
        {
            source_text++;
            if ( source_text >= end_text ) return num_lines;
        }
        source_text += 2;
    }

    return num_lines;
}

//-----------------------------------------------------------------------------
// writes one cell into its field; an empty cell gets the column's default

static void excel_store_cell( excel_data * data, char * text, byte * dest_location )
{
    if ( text[ 0 ] != 0 )
    {
        switch ( data->type )
        {
        case excel_type_safestring:
        {
            // drop the quotes the spreadsheet export puts around the text
            int start = 0;
            while ( text[ start ] == '"' ) start++;
            int end = (int)strlen( text ) - 1;
            while ( end > 0 && text[ end ] == '"' ) end--;

            if ( text[ start ] != 0 && end != 0 && start < end )
            {
                text[ end + 1 ] = 0;
                safestring_create_from_char( (safestring *)dest_location, &text[ start ] );
            }
            else
            {
                safestring_create( (safestring *)dest_location, NULL );
            }
            break;
        }
        case excel_type_string:     strcpy( (char *)dest_location, text ); break;
        case excel_type_char:       *(char *)dest_location = text[ 0 ]; break;
        case excel_type_int:        *(int *)dest_location = atoi( text ); break;
        case excel_type_float:      *(float *)dest_location = (float)atof( text ); break;
        case excel_type_byte:       *dest_location = (byte)atoi( text ); break;
        case excel_type_flag:       if ( text[ 0 ] == '1' ) *(dword *)dest_location |= 1 << (byte)data->value; break;
        case excel_type_bool:       *(bool *)dest_location = ( text[ 0 ] == '1' ); break;
        case excel_type_int64:      *(long long *)dest_location = _atoi64( text ); break;
        default: break;
        }
    }
    else
    {
        switch ( data->type )
        {
        case excel_type_safestring: safestring_create( (safestring *)dest_location, NULL ); break;
        case excel_type_string:
        case excel_type_bool:       *dest_location = 0; break;
        case excel_type_char:
        case excel_type_byte:       *dest_location = (byte)data->value; break;
        case excel_type_int:        *(int *)dest_location = data->value; break;
        case excel_type_float:      *(float *)dest_location = 0.0f; break;
        case excel_type_int64:      *(long long *)dest_location = 0; break;
        default: break;
        }
    }
}

//-----------------------------------------------------------------------------
// Reads the tab-separated text file. The first line names the columns and must include "id";
// each following line is one entry, stored in the destination array at index id. The array has
// highest id + 1 records followed by an int set to -1.

void read_data_excel_file( excel_parse_table * table )
{
    dword file_size = 0;
    byte * file_data = pack_file_load( table->txt_filename, &file_size );
    assert_return( file_data );
    assert_return( file_size > 0 );

    byte * end_text = file_data + file_size;
    char word[ max_excel_word_size ];

    do  // single pass; every failure below leaves through pack_file_close
    {
        int num_categories = get_num_categories( file_data, end_text );
        if ( !( num_categories > 0 ) ) { assert_exp( num_categories > 0 ); break; }

        table->all_categories = (excel_category *)drb_malloc( num_categories * sizeof( excel_category ), __FILE__, __LINE__, NULL );
        if ( !table->all_categories ) { assert_exp( table->all_categories ); break; }

        // column names
        byte * source = file_data;
        table->id_category = NULL;
        bool failed = false;
        for ( int category_index = 0; category_index < num_categories; )
        {
            int word_size = get_next_word_in_line( source, end_text, word );
            if ( !( word_size > 0 ) ) { assert_exp( word_size > 0 ); failed = true; break; }
            source += word_size;

            if ( word[ 0 ] != 0 )
            {
                excel_category * category = &table->all_categories[ category_index ];
                strcpy( category->name, word );
                if ( _stricmp( category->name, "id" ) == 0 ) table->id_category = category;
                category->first_cell = NULL;
                category->next = NULL;

                if ( table->first_category )
                {
                    excel_category * last = table->first_category;
                    while ( last->next ) last = last->next;
                    last->next = category;
                }
                else
                {
                    table->first_category = category;
                }
                category_index++;
            }

            if ( source < end_text && *source == '\t' ) source++;
        }
        if ( failed ) break;
        if ( !( table->id_category != NULL ) ) { assert_exp( table->id_category != NULL ); break; }

        if ( source < end_text && *source == '\r' ) source += 2;

        table->num_entries = get_num_lines( source, end_text );
        if ( !( table->num_entries > 0 ) ) { assert_exp( table->num_entries > 0 ); break; }

        table->all_cells = (excel_cell *)drb_malloc( table->num_entries * num_categories * sizeof( excel_cell ), __FILE__, __LINE__, NULL );
        if ( !table->all_cells ) { assert_exp( table->all_cells ); break; }

        // every cell of every line, chained per column
        int cell_index = 0;
        while ( source < end_text )
        {
            char line[ max_excel_word_size ];
            int line_size = 0;
            while ( source < end_text && *source != '\r' && line_size < max_excel_word_size - 1 ) line[ line_size++ ] = *source++;
            line[ line_size ] = 0;

            if ( line[ 0 ] != 0 )
            {
                byte * line_text = (byte *)line;
                byte * line_end = (byte *)&line[ line_size ];
                for ( excel_category * category = table->first_category; category; category = category->next )
                {
                    int word_size = get_next_word_in_line( line_text, line_end, word );
                    if ( !( word_size > 0 ) ) { assert_exp( word_size > 0 ); failed = true; break; }
                    line_text += word_size;

                    excel_cell * cell = &table->all_cells[ cell_index ];
                    strcpy( cell->text, word );
                    cell->next = NULL;

                    if ( category->first_cell )
                    {
                        excel_cell * last = category->first_cell;
                        while ( last->next ) last = last->next;
                        last->next = cell;
                    }
                    else
                    {
                        category->first_cell = cell;
                    }
                    cell_index++;

                    if ( line_text < line_end && *line_text == '\t' ) line_text++;
                }
                if ( failed ) break;
            }

            if ( source >= end_text ) break;
            if ( *source == '\r' ) source += 2;
        }
        if ( failed ) break;
        if ( !( cell_index == table->num_entries * num_categories ) ) { assert_exp( cell_index == table->num_entries * num_categories ); break; }

        int highest_id = -1;
        for ( excel_cell * cell = table->id_category->first_cell; cell; cell = cell->next )
        {
            int id = atoi( cell->text );
            if ( id > highest_id ) highest_id = id;
        }
        if ( !( highest_id >= 0 ) ) { assert_exp( highest_id >= 0 ); break; }

        int num_destination = highest_id + 1;
        dword dest_size = num_destination * table->destination_size + sizeof( int );
        if ( !( dest_size > sizeof( int ) ) ) { assert_exp( dest_size > sizeof( int ) ); break; }

        byte * destination_data = (byte *)drb_malloc( dest_size, __FILE__, __LINE__, NULL );
        if ( !destination_data ) { assert_exp( destination_data ); break; }
        memset( destination_data, 0, dest_size );
        *table->destination_data = destination_data;

        int * end_of_data = (int *)( destination_data + dest_size - sizeof( int ) );
        if ( !( (void *)end_of_data >= (void *)destination_data ) ) { assert_exp( (void *)end_of_data >= (void *)destination_data ); break; }
        *end_of_data = -1;

        for ( int i = 0; i < table->num_data && !failed; i++ )
        {
            excel_data * data = &table->data_list[ i ];
            if ( data->type == excel_type_link ) continue;

            excel_category * category = table->first_category;
            while ( category && _stricmp( data->name, category->name ) != 0 ) category = category->next;
            if ( !category ) continue;

            excel_cell * cell = category->first_cell;
            excel_cell * id_cell = table->id_category->first_cell;
            for ( int entry = 0; entry < table->num_entries; entry++ )
            {
                byte * dest_location = destination_data + data->offset + atoi( id_cell->text ) * table->destination_size;
                if ( !( (void *)dest_location < (void *)end_of_data ) ) { assert_exp( (void *)dest_location < (void *)end_of_data ); failed = true; break; }

                excel_store_cell( data, cell->text, dest_location );

                cell = cell->next;
                id_cell = id_cell->next;
            }
        }
        if ( failed ) break;

        *table->num_destination = num_destination;
    }
    while ( 0 );

    pack_file_close( file_data );
}

//-----------------------------------------------------------------------------
// The .gbe file is the destination array itself; it stays owned by the pack file.

void read_binary_excel_file( excel_parse_table * table )
{
    dword file_size;
    *table->destination_data = pack_file_load( table->gbe_filename, &file_size );
    *table->num_destination = file_size / table->destination_size;
}

//-----------------------------------------------------------------------------

void free_data_excel_file( excel_parse_table * table )
{
    if ( table->all_categories ) drb_free( table->all_categories );
    table->all_categories = NULL;
    if ( table->all_cells ) drb_free( table->all_cells );
    table->all_cells = NULL;
    table->first_category = NULL;

    assert_return( *table->destination_data );
    drb_free( *table->destination_data );
}

//-----------------------------------------------------------------------------

void free_binary_excel_file( excel_parse_table * table )
{
    if ( table->all_categories ) drb_free( table->all_categories );
    table->all_categories = NULL;
    if ( table->all_cells ) drb_free( table->all_cells );
    table->all_cells = NULL;
    table->first_category = NULL;

    assert_return( *table->destination_data );
    pack_file_close( *table->destination_data );
}
