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

#include <Edje.h>
#include <echoicebox.h>

#include "overview.h"
#include "dir.h"
/* #include "favorites_menu.h" */
#include "favorites.h"
#include "recent.h"

static const char* titles[] = {
    "Bookshelf",
    "Bookshelf",
    "Pictures (navigation)",
    "Audio (navigation)"
};

static void _init_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    choicebox_set_selection(choicebox, 0);
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

static bool _key_down(madshelf_state_t* state, Evas_Object* choicebox,
                       Evas_Event_Key_Down* ev)
{
    const char* k = ev->keyname;

    return false;
}

static void _activate_item(madshelf_state_t* state, Evas_Object* choicebox,
                       int item_num, bool is_alt)
{
    /* FIXME? stored paths for every disk? */
    if(item_num < state->disks->n)
    {
        madshelf_disk_t* disk = state->disks->disk + item_num;
        const char* path = disk->current_path ? disk->current_path : disk->path;
        go(state, dir_make(state, path));
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
        edje_object_part_text_set(item, "title", state->disks->disk[item_num].name);

    if(item_num - state->disks->n == 0)
        edje_object_part_text_set(item, "title", gettext("Favorites"));

    if(item_num - state->disks->n == 1)
        edje_object_part_text_set(item, "title", gettext("Recent files"));
}

madshelf_loc_t* overview_make(madshelf_state_t* state)
{
    static madshelf_loc_t loc = {
        NULL,
        &_init_gui,
        &_update_gui,
        &_key_down,
        &_activate_item,
        &_draw_item,
    };
    return &loc;
}
