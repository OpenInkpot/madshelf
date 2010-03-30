#include "positions.h"

#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <expat.h>

#include <libeoi_utils.h>

#include <stdio.h>
#include <stdarg.h>
static void
dbg(const char *fmt, ...)
{
    if (getenv("MADSHELF_DEBUG")) {
        fprintf(stderr, "madshelf: ");
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
}

/*
 * FIXME: this "positions" stuff works only for FBReader. Needs updating to be
 * pluggable like libextractor.
 */

#define FBREADER_STATE_FILE "/.FBReader/state.xml"

struct positions_t {
    char *state_file;
};

positions_t *
init_positions()
{
    positions_t *positions = calloc(1, sizeof(positions_t));
    positions->state_file = xasprintf("%s" FBREADER_STATE_FILE, getenv("HOME"));

    return positions;
}

typedef struct {
    XML_Parser parser;
    const char *search;
    int pos;
    bool needed_group;
} _callback_t;

static const char *
get_attribute(const char *name, const XML_Char **attrs)
{
    while (*attrs && strcmp(name, *attrs))
        attrs += 2;
    return *(attrs + 1);
}

static void
_start_element(void *param, const XML_Char *name, const XML_Char **attrs)
{
    _callback_t *ctx = param;

    dbg(" start_element: %s", name);

    if (!strcmp(name, "group")) {
        const char *f = get_attribute("name", attrs);
        if (!f) {
            ctx->needed_group = false;
            return;
        }
        dbg("  name: %s", f);
        char *filename = strdup(f);
        char *c = strchr(filename, ':');
        if (c)
            *c = '\0';
        dbg("  filename: %s", filename);
        ctx->needed_group = !strcmp(ctx->search, filename);
        return;
    }

    if (ctx->needed_group) {
        if (!strcmp(name, "option")) {
            dbg("  option");
            const char *n = get_attribute("name", attrs);
            if (!strcmp(n, "Position")) {
                const char *v = get_attribute("value", attrs);
                if (v) {
                    ctx->pos = atoi(v);
                    dbg("   position: %s->%d", v, ctx->pos);
                    XML_StopParser(ctx->parser,false);
                    return;
                }
            }
        }
    }
}

int
get_position(positions_t *p, const char *filename)
{
    dbg("Checking current position of %s", filename);

    int fd = open(p->state_file, O_RDONLY);
    if (fd == -1)
        return -1;

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetStartElementHandler(parser, _start_element);

    _callback_t ctx = { .pos = -1, .search = filename, .parser = parser };

    XML_SetUserData(parser, &ctx);

    dbg("Parsing");

    char buffer[4096];
    for (;;) {
        int read_ = read(fd, buffer, 4096);
        dbg("Read %d bytes from state.xml: %.*s", read_, read_, buffer);
        if (read_ < 0) {
            dbg("Got error %s", strerror(errno));
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else
                break;
        } else {
            int res = XML_Parse(parser, buffer, read_, read_ == 0);
            dbg("Got %d (%d: %s, %d:%d) from XML_Parse", res, XML_GetErrorCode(parser),
                XML_ErrorString(XML_GetErrorCode(parser)),
                XML_GetCurrentLineNumber(parser),
                XML_GetCurrentColumnNumber(parser));
            if (read_ == 0 || res == XML_STATUS_ERROR)
                break;
        }
    }

    dbg("Position of %s: %d", filename, ctx.pos);

    close(fd);
    return ctx.pos;
}

void
free_positions(positions_t *positions)
{
    free(positions);
}
