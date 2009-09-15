#include <assert.h>
#include <mpd/status.h>
#include "empd.h"
#include "madaudio.h"


static void
status_callback(void *data, void *cb_data)
{
    printf("madaudio: status callback\n");
    madaudio_player_t* player = (madaudio_player_t *) cb_data;
    if(mpd_status_get_state(player->conn->status) != MPD_STATE_PLAY)
    {
        madaudio_polling_stop(player);
    }
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
    if(player->filename)
    {
        /* hack: madaudio_play_file try to free madaudio->player */
        char* filename = player->filename;
        player->filename = NULL;
        madaudio_play_file(player, filename);
        free(filename);
    }
    else
        empd_status_sync(player->conn, status_callback, player);
}


static int
poll_callback(void* data)
{
    madaudio_player_t* player = (madaudio_player_t *) data;
    empd_status_sync(player->conn, status_callback, player);
    return 1;
}

void
madaudio_polling_stop(madaudio_player_t* player)
{
    if(player->poll_mode)
    {
        player->poll_mode = false;
        ecore_timer_del(player->poll_timer);
        printf("Stop polling\n");
    }
}

void
madaudio_polling_start(madaudio_player_t* player)
{
    printf("Start polling\n");
    player->poll_timer = ecore_timer_loop_add(1.0, poll_callback, player);
    player->poll_mode = true;
    poll_callback(player);
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
