#ifndef _MADSHELF_AUDIO_H
#define _MADSHELF_AUDIO_H 1

#include <Evas.h>
#include "empd.h"

typedef struct madshelf_player_t madshelf_player_t;
struct madshelf_player_t {
    Evas_Object* gui;
    empd_connection_t* conn;
};

#endif
