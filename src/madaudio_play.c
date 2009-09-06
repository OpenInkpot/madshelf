#include "empd.h"
#include "madaudio.h"

void
madaudio_play_file(madaudio_player_t* player, const char* filename)
{
}

static void
status_callback(void *data, void *cb_data)
{
    printf("madaudio: status callback\n");
    madaudio_player_t* player = (madaudio_player_t *) cb_data;
    madaudio_draw_song(player);
}

void
madaudio_connect(madaudio_player_t* player)
{
    player->conn = empd_connection_new("/var/run/mpd/socket");
    empd_status_sync(player->conn, status_callback, player);
}
