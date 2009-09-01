#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <Eina.h>
#include <Ecore.h>

#include <mpd/async.h>
#include <mpd/parser.h>
#include "empd.h"

static void
line_callback(empd_connection_t * conn, const char *line) {
}


static void
idle_line_callback(empd_connection_t * conn, const char *line) {
    printf("idle signal\n");
    conn->line_callback = &line_callback;
    if(conn->idle_mode) {
        conn->idle_mode = false;
        conn->idle_callback(conn);
    }
}

void
empd_enter_idle_mode(empd_connection_t * conn) {
    conn->line_callback = &idle_line_callback;
    conn->idle_mode = true;
}

#define MPD_WELCOME_MESSAGE   "OK MPD "

static void
hello_line_callback(empd_connection_t * conn, const char *line) {
    if (strncmp(line, MPD_WELCOME_MESSAGE, strlen(MPD_WELCOME_MESSAGE))) {
//        empd_error_code(&conn->error, MPD_ERROR_NOTMPD);
//        empd_error_printf(&conn->error, "mpd not running");
        printf("mpd not running\n");
    }
    printf("Got hello line\n");
    conn->line_callback = &line_callback;
}


static int
io_callback(void *data, Ecore_Fd_Handler *fd_handler) {
    empd_connection_t * conn = (empd_connection_t *) data;
    int events=0;

    if (ecore_main_fd_handler_active_get(conn->fdh, ECORE_FD_READ))
        events |= MPD_ASYNC_EVENT_READ;

    if (ecore_main_fd_handler_active_get(conn->fdh, ECORE_FD_WRITE))
        events |= MPD_ASYNC_EVENT_WRITE;

    if (ecore_main_fd_handler_active_get(conn->fdh, ECORE_FD_ERROR))
        events |= MPD_ASYNC_EVENT_HUP | MPD_ASYNC_EVENT_ERROR;

    mpd_async_io(conn->async, events);

    char *line = mpd_async_recv_line(conn->async);
    if(line) {
        conn->line_callback(conn, line);
    }
    return 1;
}

#define LENGTH_OF_SOCKADDR_UN(s) \
    (strlen((s)->sun_path) + (size_t)(((struct sockaddr_un *)NULL)->sun_path))

static int
empd_socket(const char *sockpath) {
    struct sockaddr_un  socket_unix;
    char                buf[4096];
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    int curstate = 0;
    if(fd < 0) goto err_sock;
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) goto err_sock;
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) goto err_sock;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &curstate, sizeof(curstate)) < 0) goto err_sock;
    socket_unix.sun_family = AF_UNIX;
    strncpy(socket_unix.sun_path, buf, sizeof(socket_unix.sun_path));
    int socket_unix_len = LENGTH_OF_SOCKADDR_UN(&socket_unix);
    if (connect(fd, (struct sockaddr *)&socket_unix, socket_unix_len) < 0)
        goto err_sock;
    return fd;
err_sock:
    return -1;
}

empd_connection_t *
empd_connection_new(const char *sockpath) {
    empd_connection_t *conn = calloc(1, sizeof(empd_connection_t));
    int sock = -1;
    if(conn) {
        sock = empd_socket(sockpath);
        if( 0 < sock) {
            printf("Can't get socket\n");
            goto sock_err;
        }
        conn->async = mpd_async_new(sock);
        conn->line_callback = &hello_line_callback;
        conn->sock = sock;

        if(!conn->async) {
            printf("Can't create mpd_async\n");
            goto sock_err;
        }

        conn->parser = mpd_parser_new();
        if(!conn) {
            printf("Can't alloc parser\n");
            goto sock_err;
        }
        conn->fdh = ecore_main_fd_handler_add(sock,
            ECORE_FD_READ | ECORE_FD_WRITE | ECORE_FD_ERROR,
            io_callback, (void *)conn, NULL, NULL);
        printf("Connection initialized\n");
    }
    return conn;
sock_err:
    if(conn->async)
        mpd_async_free(conn->async);
    if(conn->parser)
        mpd_parser_free(conn->parser);
    if(sock >= 0)
        close(sock);
    free(conn);
    return NULL;
}


void
empd_connection_del(empd_connection_t * conn) {
    if(conn) {
        if(conn->parser)
            mpd_parser_free(conn->parser);
        if(conn->async)
            mpd_async_free(conn->async);
        if(conn->fdh)
            ecore_main_fd_handler_del(conn->fdh);
        close(conn->sock);
    }
}
