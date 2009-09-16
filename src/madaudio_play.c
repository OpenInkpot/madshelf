#include <assert.h>
#include <string.h>
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

static int
reconnect_callback(void* data)
{
    madaudio_player_t* player = (madaudio_player_t *) data;
    madaudio_connect(player);
    return 0;
}

static void
connect_errback(const char* sockpath, void* data)
{
    madaudio_player_t* player = (madaudio_player_t *) data;
    if(--player->retry)
    {
        if(!player->mpd_run)
        {
            player->mpd_run=true;
            printf("spawing mpd\n");
            Ecore_Exe* exe;
            exe = ecore_exe_run("/usr/bin/mpd /etc/madshelf/mpd.conf",
                NULL);
            if(exe)
                ecore_exe_free(exe);
        }
        ecore_timer_add(1.0, reconnect_callback, data);
    }
    else
    {
        printf("Can't wake up mpd, exitting\n");
        ecore_main_loop_quit();
    }
}

static void
connect_callback(void *data, void* cb_data)
{
    printf("connected\n");
    madaudio_player_t* player = (madaudio_player_t*) cb_data;
    empd_connection_t* conn = (empd_connection_t*) data;
    assert(player);
    player->conn = conn;
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
    empd_connection_new("/var/run/mpd/socket",
       connect_callback, connect_errback, player);
}

static void
ready_callback(void *data, void* cb_data)
{
    printf("ready callback\n");
}

void
madaudio_pause(madaudio_player_t* player)
{

    empd_send_wait(player->conn, ready_callback, player, "pause", NULL);
}

void
madaudio_play(madaudio_player_t* player)
{
    empd_send_wait(player->conn, ready_callback, player, "play", NULL);
}

void
madaudio_play_pause(madaudio_player_t* player)
{
    if(mpd_status_get_state(player->conn->status) == MPD_STATE_PLAY)
        madaudio_pause(player);
    else
        madaudio_play(player);
}

void
madaudio_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    madaudio_player_t* player = (madaudio_player_t*)param;
    Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
    const char* action = keys_lookup_by_event(player->keys, "player", ev);
    if(!strcmp(action, "PlayPause"))
        madaudio_play_pause(player);
    if(!strcmp(action, "Quit"))
        ecore_main_loop_quit();
}
