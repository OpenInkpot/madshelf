#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <iconv.h>

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


#define CR3_STATE_FILE "/.crengine/cr3hist.bmk"

/* Either NULL or zero-terminated string */
typedef struct {
    char *value;
    size_t len;
} str_t;

static void
str_fini(str_t *s)
{
    free(s->value);
    s->value = NULL;
    s->len = 0;
}

static void
str_append(str_t *s, const XML_Char *to_append, int len)
{
    if (!len)
        return;

    s->value = realloc(s->value, (s->len + len + 1) * sizeof(char));
    if (!s->value)
        perror("str_append");
    memcpy(s->value + s->len, to_append, len);
    s->len += len;
    s->value[s->len] = 0;
}

static const char *
str_get(str_t *s)
{
    return s->value ? s->value : "";
}



typedef struct plugin_t plugin_t;
struct plugin_t {
    char *state_file;
    Eina_List *cache;
    Ecore_File_Monitor *monitor;
};


static void nothing(void *ptr __attribute__((unused))) {};


typedef struct {
    XML_Parser parser;
    str_t filename;
    str_t dirname;
    int pos;

    bool FictionBookMarks;
    bool doc_filename_flag;
    bool doc_dirname_flag;


    Eina_List **cache;
} _context_t;

static const char *
get_attribute(const char *name, const XML_Char **attrs)
{
    while (*attrs && strcmp(name, *attrs))
        attrs += 2;
    return *(attrs + 1);
}

/*
 * This function fills the 256-byte Unicode conversion table for single-byte
 * encoding. The easy way to do it is to convert every byte to UTF-32 and then
 * construct Unicode character from 4-byte representation.
 */
static int
fill_byte_encoding_table(const char *encoding, XML_Encoding *info)
{
    int i;

    iconv_t ic = iconv_open("UTF-32BE", encoding);
    if (ic == (iconv_t)-1)
        return XML_STATUS_ERROR;

    for (i = 0; i < 256; ++i) {
        char from[1] = { i };
        unsigned char to[4];

        char *fromp = from;
        unsigned char *top = to;
        size_t fromleft = 1;
        size_t toleft = 4;

        size_t res = iconv(ic, &fromp, &fromleft, (char **)&top, &toleft);

        if (res == (size_t) -1 && errno == EILSEQ) {
            info->map[i] = -1;
        }
        else if (res == (size_t) -1) {
            iconv_close(ic);
            return XML_STATUS_ERROR;
        }
        else
            info->map[i] = to[0] * (1 << 24) + to[1] * (1 << 16) + to[2] * (1 << 8) + to[3];
    }

    iconv_close(ic);
    return XML_STATUS_OK;
}

static int
unknown_encoding_handler(void *user, const XML_Char *name, XML_Encoding *info)
{
    /*
     * Just pretend that all encodings are single-byte :)
     */
    return fill_byte_encoding_table(name, info);
}


static void
_handle_char(void *param, const XML_Char *s, int len)
{
    _context_t *ctx = param;
    if(ctx->doc_filename_flag)
        str_append(&ctx->filename, s, len);
    if(ctx->doc_dirname_flag)
        str_append(&ctx->dirname, s, len);
}

static void
_start_element(void *param, const XML_Char *name, const XML_Char **attrs)
{
    _context_t *ctx = param;

    dbg(" start_element: %s", name);

    if (!strcmp(name, "FictionBookMarks")) {
        ctx->FictionBookMarks = true;
        return;
    }
    if(!ctx->FictionBookMarks)
        fprintf(stderr, "cr3 parser out of sync\n");

    if(!strcmp(name, "doc-filename"))
        ctx->doc_filename_flag = true;
    if(!strcmp(name, "doc-filepath"))
        ctx->doc_dirname_flag = true;

    if (!strcmp(name, "bookmark")) {
        const char *bookmark_type = get_attribute("type", attrs);
        if (!strcmp(bookmark_type, "lastpos")) {
            const char *percent = get_attribute("percent", attrs);
            if(!percent||!*percent) {
                ctx->pos = -1;
                return;
            }
            float value;
            sscanf(percent, "%f%%", &value);
            ctx->pos = (int) value;
        }
    }
}

static void
_end_element(void *param, const XML_Char *name)
{
    _context_t *ctx = param;
    if(!strcmp(name, "doc-filename"))
        ctx->doc_filename_flag = false;
    if(!strcmp(name, "doc-filepath"))
        ctx->doc_dirname_flag = false;
    if(!strcmp(name, "file")) {
        char *filename = xasprintf("%s%s",
            str_get(&ctx->dirname), str_get(&ctx->filename));

        *ctx->cache = position_cache_append(*ctx->cache,
            filename,  ctx->pos);
        dbg("Cache value %s -> %d\n", filename, ctx->pos);
        free(filename);
        str_fini(&ctx->filename);
        str_fini(&ctx->dirname);
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
    XML_SetElementHandler(parser, _start_element, _end_element);
    XML_SetCharacterDataHandler(parser, _handle_char);

    _context_t ctx = {
        .cache = cache,
        .parser = parser,
    };

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
                XML_SetUnknownEncodingHandler(parser, unknown_encoding_handler, NULL);
            if (read_ == 0)
                break;
            if(res == XML_STATUS_ERROR)
            {
                dbg("positions_cr3: %s [%d]: %s\n",
                    filename,
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
                break;
            }
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
    int value =  position_cache_find(plugin->cache, filename);
    return value;
}

static void *
_load_position_fb()
{
    plugin_t *plugin = calloc(1, sizeof(plugin_t));
    plugin->state_file = xasprintf("%s" CR3_STATE_FILE, getenv("HOME"));
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
madshelf_plugin_positions_cr3(void)
{
    static const madshelf_plugin_methods_t methods = {
        .load = _load_position_fb,
        .unload = _unload_position_fb,
        .get_position = _get_position_fb,
    };
    return &methods;
}
