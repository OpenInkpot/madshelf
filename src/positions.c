#define _GNU_SOURCE
#include <dlfcn.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define _(x) x
#include <Evas.h>
#include <Edje.h>
#include <Ecore_File.h>
#include <Efreet.h>
#include <Ecore.h>
#include <libchoicebox.h>
#include <libeoi_utils.h>
#include "madshelf_positions.h"

typedef struct position_callback_t position_callback_t;
struct position_callback_t {
    void (*callback)(void *);
    void *param;
};


/* not API, private structure */
typedef struct madshelf_plugin_t  madshelf_plugin_t;
struct madshelf_plugin_t {
    void *module;
    const madshelf_plugin_methods_t *methods;
    void *instance;
};

typedef struct position_engine_t position_engine_t;
struct position_engine_t {
    Eina_List *plugins;
    Eina_List *callbacks;
};
static position_engine_t *engine;

static const char *
get_plugins_dir()
{
    return getenv("MADSHELF_PLUGINS_DIR")
        ? getenv("MADSHELF_PLUGINS_DIR") :
        MADSHELF_PLUGINS_DIR;
}


static int filter_files(const struct dirent* d)
{
    unsigned short int len = _D_EXACT_NAMLEN(d);
    return (len > 2) && !strcmp(d->d_name + len - 3, ".so");
}

static madshelf_plugin_t *
make_plugin(const madshelf_plugin_methods_t *methods, void *module)
{
    madshelf_plugin_t *plugin=calloc(1, sizeof(madshelf_plugin_t));
    if(!plugin)
        err(1, "Out of memory while loading plugin\n");

    plugin->methods = methods;
    if(plugin->methods->load)
        plugin->instance = plugin->methods->load();

    plugin->module = module; /* save, to unload later */

    return plugin;
}

static madshelf_plugin_t *
load_single_plugin(char *name)
{
    char *libname = xasprintf("%s/%s", get_plugins_dir(), name);
    if(!libname)
        err(1, "Out of memory while load configlet %s\n", name);

    void *libhandle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    if(!libhandle)
    {
        fprintf(stderr, "unable to load %s: %s\n", libname, dlerror());
        free(libname);
        return NULL;
    };

    /* Remove '.so' from filename */
    name[strlen(name)-3] = 0;

    char *plugin_name = xasprintf("madshelf_plugin_%s", name);
    if(!plugin_name)
        err(1, "Out of memory while load configlet %s\n", name);

    madshelf_plugin_constructor_t ctor =
        dlsym(libhandle, plugin_name);

    if(!ctor)
    {
        fprintf(stderr, "Unable to get entry point in %s: %s", name, dlerror());
        free(plugin_name);
        free(libname);
        dlclose(libhandle);
        return NULL;
    }

    free(plugin_name);
    free(libname);
    const madshelf_plugin_methods_t *methods =  ctor();
    return make_plugin(methods, libhandle);
}

static Eina_List *
load_plugins()
{
    Eina_List *lst = NULL;
    int i;
    struct dirent **files;
    int nfiles = scandir(get_plugins_dir(),
            &files, &filter_files,  &versionsort);

    if(nfiles == -1)
    {
        fprintf(stderr, "Unable to load configlets from %s: %s\n",
            get_plugins_dir(), strerror(errno));
        return NULL;
    }

    for(i = 0; i != nfiles; ++i)
    {
        madshelf_plugin_t *plugin = load_single_plugin(files[i]->d_name);
        if(plugin)
            lst = eina_list_append(lst, plugin);
    }
    return lst;
}

void *
positions_update_subscribe(positions_updated_cb callback, void *user_param)
{
    position_callback_t *cb = calloc(1, sizeof(position_callback_t));
    cb->callback = callback;
    cb->param = user_param;
    engine->callbacks = eina_list_append(engine->callbacks, cb);
    return cb;
}

void
positions_update_unsubscribe(void *passed_param)
{
    Eina_List *item = eina_list_data_find(engine->callbacks, passed_param);
    if(item)
    {
        engine->callbacks = eina_list_remove(engine->callbacks, item);
        free(passed_param);
    }
}

void
positions_update_fire_event()
{
    Eina_List *item;
    position_callback_t *cb;
    EINA_LIST_FOREACH(engine->callbacks, item, cb)
        cb->callback(cb->param);
}

int
positions_get(const char *filename)
{
    int value = -1;
    Eina_List *item = engine->plugins;
    while(item)
    {
        madshelf_plugin_t *plugin = eina_list_data_get(item);
        int next_value = plugin->methods->get_position(plugin->instance,
                                                        filename);
        if(next_value > value)
            value = next_value;
        item = eina_list_next(item);
    }
    return value;
}

void
position_engine_fini()
{
    madshelf_plugin_t *plugin;
    Eina_List *list = engine->plugins;
    EINA_LIST_FREE(list, plugin)
    {
        if(plugin->instance)
            plugin->methods->unload(plugin->instance);
        if(plugin->module)
            dlclose(plugin->module);
        free(plugin);
    }

    position_callback_t *callback;
    EINA_LIST_FREE(list, callback)
        free(callback);
    ecore_file_shutdown();
}

void
position_engine_init()
{
    ecore_file_init();
    engine = calloc(1, sizeof(position_engine_t));
    engine->plugins = load_plugins();
}

/* internal API for plugins */

typedef struct cache_item_t cache_item_t;
struct cache_item_t {
    const char *filename;
    int pos;
};

Eina_List *
position_cache_append(Eina_List *list, const char *filename, int pos)
{
    cache_item_t *item = calloc(1, sizeof(cache_item_t));
    item->filename = strdup(filename);
    item->pos = pos;
    return eina_list_append(list, item);
}

int
position_cache_find(Eina_List *list, const char *filename)
{
    while(list)
    {
        cache_item_t *item = eina_list_data_get(list);
        if(!strcmp(filename, item->filename))
            return item->pos;
        list = eina_list_next(list);
    }
    return -1;
}

void
position_cache_free(Eina_List *list)
{
    cache_item_t *ptr;
    EINA_LIST_FREE(list, ptr)
        free(ptr);
}
