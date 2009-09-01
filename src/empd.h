#ifndef _EMPD_H
#define _EMPD_H

#include <mpd/async.h>
#include <mpd/parser.h>
#include <Ecore.h>

typedef struct empd_connection_t empd_connection_t;
struct empd_connection_t  {
    struct mpd_async* async;
    struct mpd_parser* parser;
    Ecore_Fd_Handler* fdh;
    int sock;

    bool idle_mode;

    void (*line_callback)(empd_connection_t*, const char *);

    void (*idle_callback)(empd_connection_t*);

};

empd_connection_t*
empd_connection_new(const char *);

void
empd_connection_del(empd_connection_t*);

void
empd_enter_idle_mode(empd_connection_t*);

#endif
