#ifndef MADSHELF_POSITIONS_H
#define MADSHELF_POSITIONS_H

typedef struct madshelf_plugin_methods_t madshelf_plugin_methods_t;
struct madshelf_plugin_methods_t {
    void * (*load)();
    void (*unload)(void *self);
    int (*get_position)(void *self, char *filename);
};


typedef const madshelf_plugin_methods_t * (*madshelf_plugin_constructor_t)(void);

/*
 * Returns 0..100 for book in process of being read and -1 for book which is not
 * being read.
 */
int
positions_get(const char *filename);

/*
 * Watch for changed positions
 */
typedef void (*positions_updated_cb)();

void *
positions_update_subscribe(positions_updated_cb cb, void *param);

void
positions_update_unsubscribe(void *);

void position_engine_init();
void position_engine_fini();

/* internal API for plugins */

void positions_update_fire_event();

Eina_List * position_cache_append(Eina_List *list, const char *filename, int);
int position_cache_find(Eina_List *list, const char *filename);
void position_cache_free(Eina_List *);

#endif
