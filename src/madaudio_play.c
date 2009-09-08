#include <assert.h>
#include <mpd/status.h>
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


static void
connected_callback(void *data, void* cb_data)
{
    printf("connected\n");
    madaudio_player_t* player = (madaudio_player_t *) cb_data;
    assert(player);
    assert(player->conn);
    assert(player->conn == data);
    empd_status_sync(player->conn, status_callback, player);
}

void
madaudio_connect(madaudio_player_t* player)
{
    player->conn = empd_connection_new("/var/run/mpd/socket",
       connected_callback, player);
}

void
madaudio_pause(madaudio_player_t* player)
{

    empd_send_wait(player->conn, connected_callback, player, "pause", NULL);
}

void
madaudio_play(madaudio_player_t* player)
{
    empd_send_wait(player->conn, connected_callback, player, "play", NULL);
}

void
madaudio_play_pause(madaudio_player_t* player)
{
    if(mpd_status_get_state(player->conn->status) == MPD_STATE_PLAY)
        madaudio_pause(player);
    else
        madaudio_play(player);
}
