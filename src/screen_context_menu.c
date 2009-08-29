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

#include "screen_context_menu.h"

#include <string.h>

#include <Edje.h>
#include <libchoicebox.h>
#include <eoi.h>

#include "utils.h"

typedef struct
{
    madshelf_state_t* state;
    int actions_num;
    draw_screen_context_action_t draw_action;
    handle_screen_context_action_t handle_action;
    screen_context_menu_closed_t closed;
} screen_context_menu_info_t;

static void _return_focus_to_main_choicebox(Evas* canvas)
{
    Evas_Object* main_choicebox = evas_object_name_find(canvas, "contents");
    evas_object_focus_set(main_choicebox, true);
}

static void _item_handler(Evas_Object* choicebox, int item_num, bool is_alt, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "screen-context-menu");
    screen_context_menu_info_t* info = evas_object_data_get(menu, "info");

    (*info->handle_action)(info->state, item_num, is_alt);
}

static void _draw_item_handler(Evas_Object* choicebox, Evas_Object* item, int item_num,
                               int page_position, void* param)
{
    item_clear(item);

    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "screen-context-menu");
    screen_context_menu_info_t* info = evas_object_data_get(menu, "info");

    (*info->draw_action)(info->state, item, item_num);
}

static void _page_handler(Evas_Object* choicebox, int cur_page, int total_pages, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* footer = evas_object_name_find(canvas, "screen-context-menu");
    choicebox_aux_edje_footer_handler(footer, "footer", cur_page, total_pages);
}

static void _close_handler(Evas_Object* choicebox, void* param)
{
    Evas* evas = evas_object_evas_get(choicebox);
    close_screen_context_menu(evas);
}

void open_screen_context_menu(madshelf_state_t* state,
                              const char* title,
                              int actions_num,
                              draw_screen_context_action_t draw_action,
                              handle_screen_context_action_t handle_action,
                              screen_context_menu_closed_t closed)
{
    screen_context_menu_info_t* info = malloc(sizeof(screen_context_menu_info_t));
    info->state = state;
    info->actions_num = actions_num;
    info->draw_action = draw_action;
    info->handle_action = handle_action;
    info->closed = closed;

    Evas_Object* main_edje = evas_object_name_find(state->canvas, "main_edje");

    Evas_Object* screen_context_menu = eoi_settings_left_create(state->canvas);
    evas_object_name_set(screen_context_menu, "screen-context-menu");
    evas_object_data_set(screen_context_menu, "info", info);

    edje_object_part_text_set(screen_context_menu, "title", title);

    choicebox_info_t choicebox_info = {
        NULL,
        "/usr/share/choicebox/choicebox.edj",
        "settings-left",
        "/usr/share/choicebox/choicebox.edj",
        "item-settings",
        _item_handler,
        _draw_item_handler,
        _page_handler,
        _close_handler,
    };

    Evas_Object* screen_context_menu_choicebox
        = choicebox_new(state->canvas, &choicebox_info, info);

    evas_object_name_set(screen_context_menu_choicebox, "screen-context-menu-choicebox");
    edje_object_part_swallow(screen_context_menu, "contents", screen_context_menu_choicebox);
    edje_object_part_swallow(main_edje, "left-overlay", screen_context_menu);

    choicebox_set_size(screen_context_menu_choicebox, actions_num);

    evas_object_show(screen_context_menu_choicebox);
    evas_object_show(screen_context_menu);

    choicebox_aux_subscribe_key_up(screen_context_menu_choicebox);
    evas_object_focus_set(screen_context_menu_choicebox, true);

#ifdef DEBUG
    dump_evas_hier(state->canvas);
#endif
}

void close_screen_context_menu(Evas* canvas)
{
    Evas_Object* main_edje = evas_object_name_find(canvas, "main_edje");
    Evas_Object* screen_context_menu = evas_object_name_find(canvas, "screen-context-menu");
    Evas_Object* screen_context_choicebox = evas_object_name_find(canvas, "screen-context-menu-choicebox");

    if(!screen_context_menu)
        return;

    screen_context_menu_info_t* info = evas_object_data_get(screen_context_menu, "info");

    edje_object_part_unswallow(main_edje, screen_context_menu);
    edje_object_part_unswallow(screen_context_menu, screen_context_choicebox);
    evas_object_del(screen_context_menu);
    evas_object_del(screen_context_choicebox);

    _return_focus_to_main_choicebox(canvas);

    (*info->closed)(info->state);
    free(info);
}
