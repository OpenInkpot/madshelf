#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <expat.h>

#include <Ecore_File.h>
#include <eina_list.h>

#include <libeoi_utils.h>
#include "positions.h"

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


#define FBREADER_STATE_FILE "/.FBReader/state.xml"

typedef struct plugin_t plugin_t;
struct plugin_t {
    char *state_file;
    Eina_List *cache;
    Ecore_File_Monitor *monitor;
};


static void nothing(void *ptr __attribute__((unused))) {};


typedef struct {
    XML_Parser parser;
    char *filename;
    Eina_List **cache;
} _context_t;

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
    _context_t *ctx = param;

    dbg(" start_element: %s", name);

    if (!strcmp(name, "group")) {
        const char *f = get_attribute("name", attrs);
        if (!f) {
            return;
        }
        dbg("  name: %s", f);
        char *filename = strdup(f);
        char *c = strchr(filename, ':');
        if (c)
            *c = '\0';
        dbg("  filename: %s", filename);
        ctx->filename = filename;
        return;
    }

    if (ctx->filename) {
        if (!strcmp(name, "option")) {
            dbg("  option");
            const char *n = get_attribute("name", attrs);
            if (!strcmp(n, "Position")) {
                const char *v = get_attribute("value", attrs);
                if (v) {
                    int pos = atoi(v);
                    dbg("   position: %s->%d", v, pos);
                    *ctx->cache = position_cache_append(*ctx->cache,
                        ctx->filename, pos);
                    free(ctx->filename);
                    ctx->filename = NULL;
                    return;
                }
            }
        }
    }
}

static void
parse_position(Eina_List **cache, const char *filename, const char *state_file)
{
    dbg("Checking current position of %s", filename);

    int fd = open(state_file, O_RDONLY);
    if (fd == -1)
        return;

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetStartElementHandler(parser, _start_element);

    _context_t ctx = { .cache = cache, .filename = NULL, .parser = parser };

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


    close(fd);
}


static void
_positions_updated(void *data, Ecore_File_Monitor *monitor,
                  Ecore_File_Event event, const char *path)
{
    plugin_t *plugin = (plugin_t *) data;
    if(plugin->cache)
    {
        position_cache_free(plugin->cache);
        plugin->cache=NULL;
    }
    positions_update_fire_event();
}

static int
_get_position_fb(void *self, char *filename)
{
    plugin_t *plugin = (plugin_t *) self;
    if(!plugin->cache)
        parse_position(&plugin->cache, filename,
                        plugin->state_file);
    return position_cache_find(plugin->cache, filename);
}

static void *
_load_position_fb()
{
    plugin_t *plugin = calloc(1, sizeof(plugin_t));
    plugin->state_file = xasprintf("%s" FBREADER_STATE_FILE, getenv("HOME"));
    plugin->cache = NULL;
    plugin->monitor = ecore_file_monitor_add(plugin->state_file,
        _positions_updated, plugin);
    return plugin;
}

static void
_unload_position_fb(void *self)
{
    plugin_t *plugin = (plugin_t *)self;
    if(plugin->monitor)
        ecore_file_monitor_del(plugin->monitor);
    position_cache_free(plugin->cache);
    free(plugin->state_file);
    free(plugin);
}


const madshelf_plugin_methods_t *
madshelf_plugin_positions_fb(void)
{
    static const madshelf_plugin_methods_t methods = {
        .load = _load_position_fb,
        .unload = _unload_position_fb,
        .get_position = _get_position_fb,
    };
    return &methods;
}
