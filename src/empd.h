#ifndef _EMPD_H
#define _EMPD_H

#include <mpd/async.h>
#include <mpd/parser.h>
#include <mpd/song.h>
#include <Eina.h>
#include <Ecore.h>

typedef void (*empd_callback_func_t)(void *data, void* value);

typedef struct empd_callback_t empd_callback_t;
struct empd_callback_t {
    empd_callback_func_t   func;
    void *data;
};

void empd_callback_set(empd_callback_t** cb, empd_callback_func_t, void*);
void empd_callback_run(empd_callback_t* cb, void *value);
void empd_callback_once(empd_callback_t** cb, void *value);
void empd_callback_free(empd_callback_t* cb);

typedef struct empd_connection_t empd_connection_t;
struct empd_connection_t  {
    struct mpd_async* async;
    struct mpd_parser* parser;
    Ecore_Fd_Handler* fdh;
    int sock;

    Eina_List* playlist;
    struct mpd_status* status;
    struct mpd_entity* entity;

    /* fire when we sync empd_connection_t with mpd */
    empd_callback_t* synced;
    empd_callback_t* next_callback;
    empd_callback_t* connected;

    /* internal callbacks */
    empd_callback_t* playlist_callback;
    empd_callback_t* status_callback;
    empd_callback_t* pair_callback;
    empd_callback_t* finish_callback;

    bool idle_mode;

    void (*line_callback)(empd_connection_t*, char *);

    void (*idle_callback)(empd_connection_t*);

};

empd_connection_t*
empd_connection_new(const char *, void (*callback)(void*,void*), void* );

void
empd_connection_del(empd_connection_t*);

void
empd_enter_idle_mode(empd_connection_t*);

void
empd_playlist_clear(empd_connection_t* conn);

void
empd_playlist_append(empd_connection_t* conn, const struct mpd_song* song);

void
empd_status_sync(empd_connection_t* conn,
                void (*callback)(void* , void *), void *data);

/* internal */
void empd_finish_entity(empd_connection_t* conn);

#endif
