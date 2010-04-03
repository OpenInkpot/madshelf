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

#define _GNU_SOURCE

#include <libintl.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
/* This is to mark statically-allocated strings as translatable */
#define _(x) (x)

#include <Edje.h>
#include <libchoicebox.h>

#include "overview.h"
#include "dir.h"
/* #include "favorites_menu.h" */
#include "favorites.h"
#include "recent.h"

static const char* titles[] = {
    _("Bookshelf"),
    _("Bookshelf"),
    _("Pictures (navigation)"),
    _("Audio (navigation)")
};

static void _init_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    choicebox_scroll_to(choicebox, 0);
    choicebox_set_selection(choicebox, -1);
    set_sort_icon(state, ICON_SORT_NONE);
}

static void _update_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    Evas_Object* header = evas_object_name_find(state->canvas, "main_edje");

    /* disks + favorites + recent */
    choicebox_set_size(choicebox, state->disks->n + 2);
    choicebox_invalidate_interval(choicebox, 0, state->disks->n + 2);

    edje_object_part_text_set(header, "title", gettext(titles[state->filter]));
}

static void _request_exit(madshelf_state_t* state, Evas_Object* choicebox)
{
    ecore_evas_hide(state->win);
}

static void _activate_item(madshelf_state_t* state, Evas_Object* choicebox,
                       int item_num, bool is_alt)
{
    /* FIXME? stored paths for every disk? */
    if(item_num < state->disks->n)
    {
        madshelf_disk_t* disk = state->disks->disk + item_num;
        if(disk_mounted(disk))
        {
            const char* path = disk->current_path ? disk->current_path : disk->path;
            go(state, dir_make(state, path));
        }
    }

    if(item_num - state->disks->n == 0)
    {
        /* Skip favorites menu as useless */
        /* go(state, favorites_menu_make(state)); */
        go(state, favorites_make(state, (madshelf_favorites_type_t)state->filter));
    }

    if(item_num - state->disks->n == 1)
        go(state, recent_make(state));
}

static void _draw_item(const madshelf_state_t* state,
                        Evas_Object* item, int item_num)
{
    item_clear(item);

    if(item_num < state->disks->n)
    {
        if(disk_mounted(state->disks->disk + item_num))
            edje_object_part_text_set(item, "center-caption", state->disks->disk[item_num].name);
        else
        {
            char* d;
            if(!asprintf(&d, "<inactive>%s</inactive>", state->disks->disk[item_num].name))
                err(1, "Whoops, out of memory");

            edje_object_part_text_set(item, "center-caption", d);
            free(d);
        }
    }

    if(item_num - state->disks->n == 0)
        edje_object_part_text_set(item, "center-caption", gettext("Favorites"));

    if(item_num - state->disks->n == 1)
        edje_object_part_text_set(item, "center-caption", gettext("Recent files"));
}

static void
_fs_updated(madshelf_state_t *state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    choicebox_invalidate_interval(choicebox, 0, state->disks->n + 2);
}

static void
_mounts_updated(madshelf_state_t *state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    choicebox_invalidate_interval(choicebox, 0, state->disks->n + 2);
}

madshelf_loc_t* overview_make(madshelf_state_t* state)
{
    static madshelf_loc_t loc = {
        NULL,
        &_init_gui,
        &_update_gui,
        NULL,
        &_request_exit,
        &_activate_item,
        &_draw_item,
        &_fs_updated,
        &_mounts_updated,
    };
    return &loc;
}
