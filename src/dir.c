/*
 * MadShelf - bookshelf application.
 *
 * Copyright © 2009,2010 Mikhail Gusarov <dottedmag@dottedmag.net>
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

#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <libintl.h>
#include <err.h>
#include <unistd.h>
/* This is to mark statically-allocated strings as translatable */
#define _(x) (x)

#include <Edje.h>
#include <Eina.h>
#include <Ecore_File.h>
#include <libchoicebox.h>

#include "curdir.h"
#include "dir.h"
#include "fileinfo.h"
#include "fileinfo_render.h"
#include "overview.h"
#include "file_context_menu.h"
#include "screen_context_menu.h"
#include "run.h"
#include "filters.h"

static void _open_screen_context_menu(madshelf_state_t* state);
static void _open_file_context_menu(madshelf_state_t* state, const char* filename);

typedef struct
{
    madshelf_loc_t loc;

    Eina_Array* files;
    char* dir;
    int old_pos;
} _loc_t;

static void _free_files(Eina_Array* files)
{
    char* item;
    Eina_Array_Iterator iterator;
    unsigned int i;
    EINA_ARRAY_ITER_NEXT(files, i, item, iterator)
        free(item);
    eina_array_free(files);
}

static void _free(madshelf_state_t* state)
{
    _loc_t* _loc = (_loc_t*)state->loc;

    if(_loc->dir)
    {
        Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
        if(choicebox)
        {
            int sel = choicebox_get_scroll_pos(choicebox);
            if(sel == -1 || sel >= eina_array_count_get(_loc->files))
                curdir_set(_loc->dir, NULL);
            else
            {
                char* filename = eina_array_data_get(_loc->files, sel);
                if(filename)
                {
                    char* c = strdup(filename);
                    if(c)
                    {
                        char* base = basename(c);
                        curdir_set(_loc->dir, base);
                        free(c);
                    }
                    else
                        curdir_set(_loc->dir, NULL);
                }
                else
                    curdir_set(_loc->dir, NULL);
            }
        }
        else
            curdir_set(_loc->dir, NULL);
    }

    close_file_context_menu(state->canvas, false);
    close_screen_context_menu(state->canvas);

    _free_files(_loc->files);

    free(_loc->dir);
    free(_loc);
}

static void go_to_parent(madshelf_state_t* state)
{
    _loc_t* _loc = (_loc_t*)state->loc;

    char* t = strdup(_loc->dir);
    char* next_dir = dirname(t);

    madshelf_disk_t* cur_disk = find_disk(state->disks, _loc->dir);
    madshelf_disk_t* next_disk = find_disk(state->disks, next_dir);

    if(cur_disk != next_disk || !strcmp(_loc->dir, "/"))
    {
        /* Top directory reached - move to "overview" screen */
        go(state, overview_make(state));
    }
    else
    {
        go(state, dir_make(state, next_dir));
    }

    free(t);
}

static void go_to_directory(madshelf_state_t* state, const char* dirname)
{
    go(state, dir_make(state, dirname));
}

static int _name(const void* lhs, const void* rhs)
{
    return strcasecmp((const char*)lhs, (const char*)rhs);
}

static int _namerev(const void* lhs, const void* rhs)
{
    return -_name(lhs, rhs);
}

static bool file_is_hidden(const madshelf_state_t* state, const char* filename)
{
    char* filename_copy = strdup(filename);
    char* b = basename(filename_copy);

    bool is_hidden = *b == '.' || has_tag(state->tags, "hidden", filename);
    free(filename_copy);

    return is_hidden;
}

/*
 * List depends on:
 *  - state->sort
 *  - state->tags
 *  - state->filter
 *  - dir contents on FS
 */
static Eina_Array* _fill_files(const madshelf_state_t* state, const char* dir, int* old_pos)
{
    char* old_file = curdir_get(dir);
    if(old_pos) *old_pos = -1;

    Eina_Array* files = eina_array_new(10);

    Eina_List* ls = ecore_file_ls(dir);
    ls = eina_list_sort(ls, eina_list_count(ls),
                        state->sort == MADSHELF_SORT_NAME ? &_name : &_namerev);

    /* First select directories */
    Eina_List* i;
    for(i = ls; i; i = eina_list_next(i))
    {
        const char* file = eina_list_data_get(i);
        char* filename;
        if(!asprintf(&filename, "%s/%s", !strcmp(dir, "/") ? "" : dir, file))
            err(1, "Whoops, out of memory");

        if(!state->show_hidden && file_is_hidden(state, filename))
        {
            free(filename);
            continue;
        }

        if(ecore_file_is_dir(filename))
        {
            if(old_file && old_pos)
                if(!strcmp(old_file, file))
                    *old_pos = eina_array_count_get(files);
            eina_array_push(files, filename);
        }
        else
            free(filename);
    }

    /* Then files */
    for(i = ls; i; i = eina_list_next(i))
    {
        const char* file = eina_list_data_get(i);
        char* filename;
        if(!asprintf(&filename, "%s/%s", !strcmp(dir, "/") ? "" : dir, file))
            err(1, "Whoops, out of memory");

        if(!state->show_hidden && file_is_hidden(state, filename))
        {
            free(filename);
            continue;
        }

        if(!is_visible(state->filter, filename))
        {
            free(filename);
            continue;
        }

        if(!ecore_file_is_dir(filename))
        {
            if(old_file && old_pos)
                if(!strcmp(old_file, file))
                    *old_pos = eina_array_count_get(files);
            eina_array_push(files, filename);
        }
        else
            free(filename);
    }

    char* s;
    EINA_LIST_FREE(ls, s)
        free(s);

    free(old_file);

    return files;
}

static void _init_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");

    _loc_t* _loc = (_loc_t*)state->loc;
    choicebox_set_size(choicebox, eina_array_count_get(_loc->files));
    choicebox_set_selection(choicebox, -1);

    choicebox_scroll_to(choicebox, _loc->old_pos == -1 ? 0 : _loc->old_pos);
}

static void _update_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    Evas_Object* header = evas_object_name_find(state->canvas, "main_edje");

    _loc_t* _loc = (_loc_t*)state->loc;

    choicebox_set_size(choicebox, eina_array_count_get(_loc->files));
    choicebox_invalidate_interval(choicebox, 0, eina_array_count_get(_loc->files));

    madshelf_disk_t* d = find_disk(state->disks, _loc->dir);

    char* header_txt;
    if(!asprintf(&header_txt, "%s:%s", d->short_name, _loc->dir + strlen(d->path)))
        err(1, "Whoops, out of memory");

    edje_object_part_text_set(header, "title", header_txt);
    free(header_txt);

    set_sort_icon(state, state->sort);
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
    go_to_parent(state);
}

static void _activate_file(madshelf_state_t* state, int item_num)
{
    _loc_t* _loc = (_loc_t*)state->loc;
    char* filename = eina_array_data_get(_loc->files, item_num);

    if(!ecore_file_exists(filename))
        return;

    if(ecore_file_is_dir(filename))
    {
        go_to_directory(state, filename);
        return;
    }

    run_default_handler(state, filename);
}

static void _activate_item(madshelf_state_t* state, Evas_Object* choicebox,
                           int item_num, bool is_alt)
{
    _loc_t* _loc = (_loc_t*)state->loc;
    char* filename = eina_array_data_get(_loc->files, item_num);

    if(is_alt)
        _open_file_context_menu(state, filename);
    else
        _activate_file(state, item_num);
}

static void _draw_item(const madshelf_state_t* state, Evas_Object* item,
                       int item_num)
{
    item_clear(item);

    _loc_t* _loc = (_loc_t*)state->loc;
    char* filename = eina_array_data_get(_loc->files, item_num);

    fileinfo_t* fileinfo = fileinfo_create(filename);
    fileinfo_render(item, fileinfo, file_is_hidden(state, filename));
    fileinfo_destroy(fileinfo);
}

static void _update_filelist_gui(madshelf_state_t* state)
{
    _loc_t* _loc = (_loc_t*)state->loc;
    _free_files(_loc->files);
    _loc->files = _fill_files(state, _loc->dir, NULL);
    _update_gui(state);
}

static void _fs_updated(madshelf_state_t* state)
{
    _loc_t* _loc = (_loc_t*)state->loc;
    madshelf_disk_t* disk = find_disk(state->disks, _loc->dir);

    if(!disk_mounted(disk))
        go(state, overview_make(state));
    else
        _update_filelist_gui(state);
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
};

madshelf_loc_t* dir_make(madshelf_state_t* state, const char* dir)
{
    /* FIXME: Validate dir */

    _loc_t* _loc = calloc(1, sizeof(_loc_t));
    _loc->loc = loc;
    _loc->dir = strdup(dir);
    _loc->files = _fill_files(state, dir, &_loc->old_pos);

    return (madshelf_loc_t*)_loc;
}

/* File menu */

static void draw_file_context_action(const madshelf_state_t* state, Evas_Object* item,
                                     const char* filename, int item_num)
{
    item_clear(item);

    if(ecore_file_is_dir(filename))
            item_num--;

    if(item_num == -1) /* "open directory" */
    {
        edje_object_part_text_set(item, "title", gettext("Open"));
    }
    if(item_num == 0)
    {
        if(has_tag(state->tags, "hidden", filename))
            edje_object_part_text_set(item, "title", gettext("Unhide"));
        else
            edje_object_part_text_set(item, "title", gettext("Hide"));
    }
    if(item_num == 1)
    {
        if(has_tag(state->tags, "favorites", filename))
            edje_object_part_text_set(item, "title", gettext("Remove from favorites"));
        else
            edje_object_part_text_set(item, "title", gettext("Add to favorites"));
    }
}

static void handle_file_context_action(madshelf_state_t* state, const char* filename,
                                       int item_num, bool is_alt)
{
    if(ecore_file_is_dir(filename))
        item_num--;

    if(item_num == -1) /* "open directory" */
    {
        go_to_directory(state, filename);
        close_file_context_menu(state->canvas, false);
    }
    if(item_num == 0)
    {
        if(has_tag(state->tags, "hidden", filename))
            tag_remove(state->tags, "hidden", filename);
        else
            tag_add(state->tags, "hidden", filename);

        close_file_context_menu(state->canvas, true);
    }
    if(item_num == 1)
    {
        if(has_tag(state->tags, "favorites", filename))
            tag_remove(state->tags, "favorites", filename);
        else
            tag_add(state->tags, "favorites", filename);

        close_file_context_menu(state->canvas, true);
    }
}

static void delete_file_context_action(madshelf_state_t* state, const char* filename)
{
    unlink(filename);
    (*state->loc->fs_updated)(state);
}


static void file_context_menu_closed(madshelf_state_t* state, const char* filename, bool touched)
{
    if(touched)
        _update_filelist_gui(state);
}

static void _open_file_context_menu(madshelf_state_t* state, const char* filename)
{
    int actions_num = ecore_file_is_dir(filename) ? 3 : 2;

    open_file_context_menu(state,
                           gettext("Actions"),
                           filename,
                           actions_num,
                           draw_file_context_action,
                           handle_file_context_action,
                           delete_file_context_action,
                           file_context_menu_closed);
}

/* Screen menu */

static const char* _sc_titles[] = {
    "Sort by name", /* Sort items should match madshelf_sort_t */
    "Sort by name (reversed)",
};

static void draw_screen_context_action(const madshelf_state_t* state,
                                       Evas_Object* item,
                                       int item_num)
{
    item_clear(item);

    if(item_num == 2)
    {
        if(state->show_hidden)
            edje_object_part_text_set(item, "title", gettext("Do not show hidden files"));
        else
            edje_object_part_text_set(item, "title", gettext("Show hidden files"));
    }
    else
    {
        edje_object_part_text_set(item, "title", gettext(_sc_titles[item_num]));
    }
}

static void handle_screen_context_action(madshelf_state_t* state,
                                         int item_num,
                                         bool is_alt)
{
    if(item_num == 2)
    {
        set_show_hidden(state, !state->show_hidden);
    }
    else
    {
        set_sort(state, (madshelf_sort_t)item_num);
    }

    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    choicebox_scroll_to(choicebox, 0);
    _update_filelist_gui(state);
    close_screen_context_menu(state->canvas);
}

static void screen_context_menu_closed(madshelf_state_t* state)
{
}

static void _open_screen_context_menu(madshelf_state_t* state)
{
    open_screen_context_menu(state,
                             gettext("Menu"),
                             3,
                             draw_screen_context_action,
                             handle_screen_context_action,
                             screen_context_menu_closed);
}
