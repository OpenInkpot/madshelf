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

#include <Edje.h>
#include <echoicebox.h>

#include "favorites_menu.h"
#include "overview.h"
#include "favorites.h"

static void _update_gui(const madshelf_state_t* state)
{
    Evas_Object* choicebox = evas_object_name_find(state->canvas, "contents");
    Evas_Object* header = evas_object_name_find(state->canvas, "main_edje");

    choicebox_set_size(choicebox, 3);
    choicebox_invalidate_interval(choicebox, 0, 3);

    edje_object_part_text_set(header, "title", gettext("Favorites"));
}

static bool _key_down(madshelf_state_t* state, Evas_Object* choicebox,
                             Evas_Event_Key_Down* ev)
{
    const char* k = ev->keyname;

    if(!strcmp(k, "Escape"))
    {
        go(state, overview_make(state));
        return true;
    }

    return false;
}

static void _activate_item(madshelf_state_t* state, Evas_Object* choicebox,
                        int item_num, bool is_alt)
{
    go(state, favorites_make(state, (madshelf_favorites_type_t)(item_num+1)));
}

static const char* items[] = {
    "Favorite books",
    "Favorite pictures",
    "Favorite music"
};

static void _draw_item(const madshelf_state_t* state,
                              Evas_Object* item, int item_num)
{
    item_clear(item);
    edje_object_part_text_set(item, "text", gettext(items[item_num]));
}

madshelf_loc_t* favorites_menu_make(madshelf_state_t* state)
{
    static madshelf_loc_t loc = {
        NULL,
        &_update_gui,
        &_key_down,
        &_activate_item,
        &_draw_item,
    };

    return &loc;
}
