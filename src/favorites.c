/*
 * MadShelf - bookshelf application.
 *
 * Copyright (C) 2009 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <libintl.h>
/* This is to mark statically-allocated strings as translatable */
#define _(x) (x)

#include <Edje.h>
#include <echoicebox.h>

#include "utils.h"
#include "favorites.h"
#include "fileinfo.h"
#include "fileinfo_render.h"
#include "file_context_menu.h"
#include "screen_context_menu.h"
#include "tags.h"
#include "overview.h"
#include "run.h"

static void _open_file_context_menu(madshelf_state_t* state, const char* filename);
static void _open_screen_context_menu(madshelf_state_t* state, Evas_Object* choicebox);

typedef struct
{
    madshelf_loc_t loc;

    Eina_Array* files;
    madshelf_favorites_type_t type;
} favorites_loc_t;

static void _update_files(favorites_loc_t* _loc, Eina_Array* files)
{
    char* item;
    Eina_Array_Iterator iterator;
    unsigned int i;
    EINA_ARRAY_ITER_NEXT(_loc->files, i, item, iterator)
        free(item);
    eina_array_free(_loc->files);

    _loc->files = files;
}

static void _free(madshelf_state_t* state)
{
    favorites_loc_t* fav_loc = (favorites_loc_t*)state->loc;
    _update_files(fav_loc, NULL);
    free(fav_loc);
}

static void _fill_file(const char* filename, int serial, void* param)
{
    /* FIXME: filter favorites. */
    Eina_Array* files = (Eina_Array*)param;
    eina_array_push(files, strdup(filename));
}

/*
 * List depends on:
 * - state->favorites_sort
 * - state->tags
 * - loc->filter
 */
static Eina_Array* _fill_files(const madshelf_state_t* state)
{
    /* FIXME: filter */
    Eina_Array* files = eina_array_new(10);
    tag_list(state->tags, "favorites", (tags_sort_t)state->favorites_sort, _fill_file, files);
    return files;
}

static const char* titles[] = {
    _("Favorites"),
    _("Favorite books"),
    _("Favorite pictures"),
    _("Favorite music"),
};

static void _update_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    Evas_Object* header = evas_object_name_find(state->canvas, "main_edje");

    favorites_loc_t* fav_loc = (favorites_loc_t*)state->loc;

    choicebox_set_selection(choicebox, -1);
    choicebox_set_size(choicebox, eina_array_count_get(fav_loc->files));
    choicebox_invalidate_interval(choicebox, 0, eina_array_count_get(fav_loc->files));


    edje_object_part_text_set(header, "title",
                              gettext(titles[fav_loc->type]));

    set_sort_icon(state, state->favorites_sort);
}

static bool _key_down(madshelf_state_t* state, Evas_Object* choicebox,
                       Evas_Event_Key_Down* ev)
{
    const char* k = ev->keyname;

    if(!strcmp(k, "Escape"))
    {
        /* Skip favorites menu */
        /* go(state, choicebox, favorites_menu_make(state)); */
        go(state, overview_make(state));
        return true;
    }
    if(!strcmp(k, "space"))
    {
        _open_screen_context_menu(state, choicebox);
        return true;
    }

    /*
     * Special case: file list opened with OK button should be treated as
     * long-pressed.
     */
    if(!strcmp(k, "Return") || !strcmp(k, "KP_Return"))
    {
        choicebox_activate_current(choicebox, true);
        return true;
    }

    return false;
}

static void _activate_item(madshelf_state_t* state, Evas_Object* choicebox,
                           int item_num, bool is_alt)
{
    favorites_loc_t* fav_loc = (favorites_loc_t*)state->loc;
    char* filename = eina_array_data_get(fav_loc->files, item_num);

    if(is_alt)
        _open_file_context_menu(state, filename);
    else
        run_default_handler(state->handlers, filename);
}

static void _draw_item(const madshelf_state_t* state,
                         Evas_Object* item, int item_num)
{
    item_clear(item);

    favorites_loc_t* fav_loc = (favorites_loc_t*)state->loc;
    char* filename = eina_array_data_get(fav_loc->files, item_num);

    fileinfo_t* fileinfo = fileinfo_create(filename);
    fileinfo_render(item, fileinfo, false);
    fileinfo_destroy(fileinfo);
}

static madshelf_loc_t loc = {
    &_free,
    &_update_gui,
    &_key_down,
    &_activate_item,
    &_draw_item,
};

madshelf_loc_t* favorites_make(madshelf_state_t* state, madshelf_favorites_type_t type)
{
    favorites_loc_t* fav_loc = malloc(sizeof(favorites_loc_t));
    fav_loc->loc = loc;
    fav_loc->type = type;
    fav_loc->files = _fill_files(state);

    return (madshelf_loc_t*)fav_loc;
}

/* Item context menu */

static void draw_file_context_action(const madshelf_state_t* state, Evas_Object* item,
                              const char* filename, int item_num)
{
    edje_object_part_text_set(item, "text", gettext("Remove from favorites"));
}

static void handle_file_context_action(madshelf_state_t* state, const char* filename,
                                int item_num, bool is_alt)
{
    tag_remove(state->tags, "favorites", filename);
    close_file_context_menu(state->canvas, true);
}

static void file_context_menu_closed(madshelf_state_t* state, const char* filename, bool touched)
{
    if(touched)
    {
        favorites_loc_t* _loc = (favorites_loc_t*)state->loc;
        _update_files(_loc, _fill_files(state));
        _update_gui(state);
    }
}

static void _open_file_context_menu(madshelf_state_t* state, const char* filename)
{
    open_file_context_menu(state,
                           gettext("Actions"),
                           filename,
                           1,
                           draw_file_context_action,
                           handle_file_context_action,
                           file_context_menu_closed);
}

/* Screen context menu */

static const char* _scm_titles[] = {
    _("Sort by name"), /* Sort items should match madshelf_sortex_t */
    _("Sort by name (reversed)"),
    _("Sort by order"),
    _("Clear favorites"),
};

static void _scm_draw(const madshelf_state_t* state,
                       Evas_Object* item, int item_num)
{
    item_clear(item);
    edje_object_part_text_set(item, "title", gettext(_scm_titles[item_num]));
}

static void _scm_handle(madshelf_state_t* state, int item_num, bool is_alt)
{
    favorites_loc_t* _loc = (favorites_loc_t*)state->loc;

    if(item_num < 3)
        set_favorites_sort(state, (madshelf_sortex_t)item_num);
    else
        tag_clear(state->tags, "favorites");

    _update_files(_loc, _fill_files(state));
    _update_gui(state);
    close_screen_context_menu(state->canvas);
}

static void _scm_closed(madshelf_state_t* state)
{
}

static void _open_screen_context_menu(madshelf_state_t* state, Evas_Object* choicebox)
{
    open_screen_context_menu(state,
                             gettext("Favorites"),
                             sizeof(_scm_titles)/sizeof(char*),
                             _scm_draw,
                             _scm_handle,
                             _scm_closed);
}
