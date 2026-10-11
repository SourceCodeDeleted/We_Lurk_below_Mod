//-----------------------------------------------------------------------------
// g_sounds.h
//
// The sounds table (xls\sounds.txt). Placeholder header, like g_strings.h: the layout is from the
// exe's excel parse table. stype is not a spreadsheet column; the table loader in main.cpp derives
// it from is_music / is_combat.

#ifndef _G_SOUNDS_H_
#define _G_SOUNDS_H_

struct sound_definition_data    // 336 bytes
{
    char        sound[ 64 ];
    int         stype;
    bool        is_music;
    bool        is_combat;
    bool        loops;
    float       volume_mod;
    int         num_alts;
    char        filename[ 4 ][ 64 ];
};

extern sound_definition_data * sound_definitions;
extern int num_sound_definitions;

#endif
