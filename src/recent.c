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

#include <libintl.h>
#include <string.h>
#include <unistd.h>
/* This is to mark statically-allocated strings as translatable */
#define _(x) (x)

#include <Edje.h>
#include <Ecore_File.h>
#include <libchoicebox.h>

#include "recent.h"
#include "fileinfo.h"
#include "fileinfo_render.h"
#include "file_context_menu.h"
#include "screen_context_menu.h"
#include "overview.h"
#include "tags.h"
#include "run.h"
#include "filters.h"
#include "dir.h"
#include "madshelf_positions.h"

static void _open_screen_context_menu(madshelf_state_t* state);
static void _open_file_context_menu(madshelf_state_t* state, const char* filename);

typedef struct
{
    madshelf_loc_t loc;
    Eina_Array* files;
    void *watcher;
} _loc_t;

static void _free_files(Eina_Array* files)
{
    char* item;
    Eina_Array_Iterator  iterator;
    unsigned int i;
    EINA_ARRAY_ITER_NEXT(files, i, item, iterator)
        free(item);
    eina_array_free(files);
}

static void _free(madshelf_state_t* state)
{
    _loc_t* _loc = (_loc_t*)state->loc;

    positions_update_unsubscribe(_loc->watcher);
    _free_files(_loc->files);
    close_file_context_menu(state->canvas, false);
    close_screen_context_menu(state->canvas);

    free(_loc);
}

typedef struct {
    Eina_Array *files;
    const madshelf_state_t *state;
} fill_file_args_t;

static void _fill_file(const char* filename, int serial, void* param)
{
    fill_file_args_t* args = (fill_file_args_t*)param;

    if (args->state->show_hidden || !is_hidden(args->state, filename))
        eina_array_push(args->files, strdup(filename));
}

static Eina_Array *_fill_files(const madshelf_state_t* state)
{
    Eina_Array *files = eina_array_new(10);
    fill_file_args_t args = { files, state };

    tag_list(state->tags, "recent", (tags_sort_t)state->recent_sort, _fill_file, &args);
    return files;
}

static void _update_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");

    _loc_t* _loc = (_loc_t*)state->loc;

    choicebox_set_size(choicebox, eina_array_count_get(_loc->files));
    choicebox_invalidate_interval(choicebox, 0, eina_array_count_get(_loc->files));

    set_sort_icon(state, state->recent_sort);
}


static void
_positions_updated(void *param)
{
    madshelf_state_t* state = param;
    _update_gui(state);
}

static void _init_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    Evas_Object* header = evas_object_name_find(state->canvas, "main_edje");

    choicebox_set_selection(choicebox, -1);
    edje_object_part_text_set(header, "title", gettext("Recent files"));

    _loc_t* _loc = (_loc_t*)state->loc;
    _loc->watcher = positions_update_subscribe(_positions_updated, (void*)state);
}

static bool _key_up(madshelf_state_t* state, Evas_Object* choicebox,
                       Evas_Event_Key_Up* ev)
{
    const char* action = keys_lookup(state->keys, "lists", ev->keyname);

    if(action && !strcmp(action, "ContextMenu"))
    {
        _open_screen_context_menu(state);
        return true;
    }

    if(action && !strcmp(action, "AltActivateCurrent"))
    {
        choicebox_activate_current(choicebox, true);
        return true;
    }

    return false;
}

static void _request_exit(madshelf_state_t* state, Evas_Object* choicebox)
{
    if (state->menu_navigation) {
        ecore_evas_hide(state->win);
    } else {
        go(state, overview_make(state));
    }
}

static void _activate_item(madshelf_state_t* state, Evas_Object* choicebox,
                           int item_num, bool is_alt)
{
    _loc_t* _loc = (_loc_t*)state->loc;
    char* filename = eina_array_data_get(_loc->files, item_num);

    if(is_alt)
        _open_file_context_menu(state, filename);
    else
    {
        run_default_handler(state, filename);
        (*state->loc->fs_updated)(state);
    }
}

static void _draw_item(const madshelf_state_t* state,
                       Evas_Object* item, int item_num)
{
    item_clear(item);

    _loc_t* _loc = (_loc_t*)state->loc;
    char* filename = eina_array_data_get(_loc->files, item_num);

    fileinfo_t* fileinfo = fileinfo_create(filename);
    fileinfo_render(item, fileinfo, is_hidden(state, filename));
    fileinfo_destroy(fileinfo);
}

static void _fs_updated(madshelf_state_t* state)
{
    _loc_t* _loc = (_loc_t*)state->loc;
    _free_files(_loc->files);
    _loc->files = _fill_files(state);
    _update_gui(state);
}

static void
_mounts_updated(madshelf_state_t *state)
{
    _loc_t *_loc = (_loc_t *)state->loc;
    _free_files(_loc->files);
    _loc->files = _fill_files(state);
    _update_gui(state);
}

static madshelf_loc_t loc = {
    &_free,
    &_init_gui,
    &_update_gui,
    &_key_up,
    &_request_exit,
    &_activate_item,
    &_draw_item,
    &_fs_updated,
    &_mounts_updated,
};

madshelf_loc_t* recent_make(madshelf_state_t* state)
{
    _loc_t* _loc = malloc(sizeof(_loc_t));
    _loc->loc = loc;

    _loc->files = _fill_files(state);

    return (madshelf_loc_t*)_loc;
}

/* File menu */

static void draw_file_context_action(const madshelf_state_t* state, Evas_Object* item,
                              const char* filename, int item_num)
{
    if(ecore_file_is_dir(filename) && item_num == 0)
    {
        edje_object_part_text_set(item, "title", gettext("Open"));
        return;
    }
    else
        item_num--;

    edje_object_part_text_set(item, "title", gettext("Remove from recent files"));
}

static void handle_file_context_action(madshelf_state_t* state, const char* filename,
                                int item_num, bool is_alt)
{
    if(ecore_file_is_dir(filename))
    {
        if(item_num == 0)
        {
            go(state, dir_make(state, filename));
            return;
        }
        else
            item_num--;
    }

    tag_remove(state->tags, "recent", filename);
    close_file_context_menu(state->canvas, true);
}

static void delete_file_context_action(madshelf_state_t* state, const char* filename)
{
    tag_remove(state->tags, "recent", filename);
    ecore_file_recursive_rm(filename);
    (*state->loc->fs_updated)(state);
}

static void file_context_menu_closed(madshelf_state_t* state, const char* filename, bool touched)
{
    if(touched)
    {
        _loc_t* _loc = (_loc_t*)state->loc;
        _free_files(_loc->files);
        _loc->files = _fill_files(state);

        _update_gui(state);
    }
}

static void _open_file_context_menu(madshelf_state_t* state, const char* filename)
{
    int num_actions = ecore_file_is_dir(filename) ? 2 : 1;

    open_file_context_menu(state,
                           gettext("Actions"),
                           filename,
                           num_actions,
                           draw_file_context_action,
                           handle_file_context_action,
                           delete_file_context_action,
                           file_context_menu_closed);
}

/* Screen menu */

static const char* _scm_titles[] = {
    _("Sort by name"), /* Sort items should match madshelf_sort_t */
    _("Sort by name (reversed)"),
    _("Sort by date"),
    _("Clear recent files"),
    NULL,
    _("Remove absent files"),
};

static void _scm_draw(const madshelf_state_t* state,
                       Evas_Object* item, int item_num)
{
    item_clear(item);
    if(item_num == 4)
    {
        edje_object_part_text_set(item, "title",
                                  state->show_hidden
                                  ? gettext("Do not show hidden files")
                                  : gettext("Show hidden files"));
    }
    else
        edje_object_part_text_set(item, "title", gettext(_scm_titles[item_num]));
}

static void _scm_handle(madshelf_state_t* state, int item_num, bool is_alt)
{
    _loc_t* _loc = (_loc_t*)state->loc;

    if(item_num < 3)
        set_recent_sort(state, (madshelf_sort_t)item_num);
    else if(item_num == 3)
        tag_clear(state->tags, "recent");
    else if(item_num == 4)
        set_show_hidden(state, !state->show_hidden);
    else
        tag_remove_absent(state->tags, "recent");

    _free_files(_loc->files);
    _loc->files = _fill_files(state);
    _update_gui(state);
    close_screen_context_menu(state->canvas);
}

static void _scm_closed(madshelf_state_t* state)
{
}

static void _open_screen_context_menu(madshelf_state_t* state)
{
    open_screen_context_menu(state,
                             gettext("Recent files"),
                             sizeof(_scm_titles)/sizeof(char*),
                             _scm_draw,
                             _scm_handle,
                             _scm_closed);
}
