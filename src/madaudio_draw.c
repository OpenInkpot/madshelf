#include <stdio.h>
#include <Evas.h>
#include <Edje.h>

#include <mpd/tag.h>
#include <mpd/song.h>
#include <mpd/status.h>
#include "madaudio.h"

static void
blank_gui(Evas_Object* gui)
{
    edje_object_part_text_set(gui, "composer", "");
    edje_object_part_text_set(gui, "album", "");
    edje_object_part_text_set(gui, "artist", "");
    edje_object_part_text_set(gui, "genre", "");
    edje_object_part_text_set(gui, "year", "");
}

static void
draw_song_tag(Evas_Object* gui, const char *field, const struct mpd_song* song,
                enum mpd_tag_type type)
{
    const char* value = mpd_song_get_tag(song, type, 0);
    printf("draw tag: %s\n", field);
    if(value)
    {
        edje_object_part_text_set(gui, field, value);
        printf("part_text_set(%s, %s)\n", field, value);
    }
}

static void
draw_song(Evas_Object* gui, const struct mpd_song* song)
{
    draw_song_tag(gui, "composer", song, MPD_TAG_COMPOSER);
    draw_song_tag(gui, "artist", song, MPD_TAG_ARTIST);
    draw_song_tag(gui, "album", song, MPD_TAG_ALBUM);
    draw_song_tag(gui, "genre", song, MPD_TAG_GENRE);
    draw_song_tag(gui, "year", song, MPD_TAG_DATE);
}

static void
draw_status(Evas_Object* gui, const struct mpd_status* status)
{
}

void
madaudio_draw_captions(madaudio_player_t* player)
{
    Evas_Object* gui = player->gui;
    printf("captions\n");
    edje_object_part_text_set(gui, "caption-composer", "Composer");
    edje_object_part_text_set(gui, "caption-artist", "Artist");
    edje_object_part_text_set(gui, "caption-album", "Album");
    edje_object_part_text_set(gui, "caption-genre", "Genre");
    edje_object_part_text_set(gui, "caption-year", "Year");
}

void
madaudio_draw_song(madaudio_player_t* player)
{
    printf("madaudio_draw_song()\n");
    madaudio_draw_captions(player);
    blank_gui(player->gui);
    draw_status(player->gui, player->conn->status);
    if(mpd_status_get_state(player->conn->status) == MPD_STATE_PLAY) {
        int song_id = mpd_status_get_song(player->conn->status);
        struct mpd_song* song = eina_list_nth(player->conn->playlist, song_id);
        if(song)
            draw_song(player->gui, song);
        else
            printf("No song\n");
    }
}
