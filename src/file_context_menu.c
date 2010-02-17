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

#include "file_context_menu.h"

#include <libintl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>

#include <Ecore_File.h>
#include <Edje.h>
#include <libchoicebox.h>
#include <libeoi.h>

#include "fileinfo.h"
#include "utils.h"
#include "handlers.h"
#include "delete_file.h"
#include "text_escape.h"

typedef struct
{
    madshelf_state_t* state;
    char* filename;

    fileinfo_t* fileinfo;
    openers_t* openers;
    int openers_num;
    int fileop_targets_num;
    int add_actions_num;
    draw_file_context_action_t draw_action;
    handle_file_context_action_t handle_action;
    delete_file_context_action_t delete_action;
    file_context_menu_closed_t closed;
} file_context_menu_info_t;

/* FIXME: use some sane data structure instead of Eina_List */
static int eina_list_size(Eina_List* list)
{
    int count = 0;
    while(list)
    {
        count++;
        list = eina_list_next(list);
    }
    return count;
}

static int get_fileop_targets_num(file_context_menu_info_t* info)
{
    madshelf_disks_t* disks = info->state->disks;
    madshelf_disk_t* cur_disk = find_disk(disks, info->filename);

    int num = 0;

    int i;
    for(i = 0; i < disks->n; ++i)
    {
        if(disks->disk + i == cur_disk) continue;
        if(disks->disk[i].copy_target)
            num++;
    }
    return num;
}

static void _return_focus_to_main_choicebox(Evas* canvas)
{
    Evas_Object* main_choicebox = evas_object_name_find(canvas, "contents");
    evas_object_focus_set(main_choicebox, true);
}

static void _run_file(file_context_menu_info_t* info, Efreet_Desktop* handler)
{
    tag_add(info->state->tags, "recent", info->filename);

    Eina_List* l = NULL;
    l = eina_list_append(l, info->filename);

    efreet_desktop_exec(handler, l, NULL);

    eina_list_free(l);
}

static void draw_handler_item(Evas_Object* choicebox, Evas_Object* item,
                              int item_num, const char* fmt)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "file-context-menu");
    file_context_menu_info_t* info = evas_object_data_get(menu, "info");

    Eina_List* nth = eina_list_nth_list(info->openers->apps, item_num);
    Efreet_Desktop* handler = eina_list_data_get(nth);

    char* name = textblock_escape_string(handler->name);
    char* f;
    asprintf(&f, fmt, name);
    free(name);

    edje_object_part_text_set(item, "title", f);
    free(f);
}

static void _page_handler(Evas_Object* choicebox, int cur_page, int total_pages, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* footer = evas_object_name_find(canvas, "file-context-menu");
    choicebox_aux_edje_footer_handler(footer, "footer", cur_page, total_pages);
}

static void _close_handler(Evas_Object* choicebox, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    close_file_context_menu(canvas, false);
}

/* "Always open with..." */

static void _set_default_handler_item_handler(Evas_Object* choicebox, int item_num, bool is_alt, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "file-context-menu");
    file_context_menu_info_t* info = evas_object_data_get(menu, "info");

    fileinfo_t* fileinfo = fileinfo_create(info->filename);

    Eina_List* nth = eina_list_nth_list(info->openers->apps, item_num);
    Efreet_Desktop* handler = eina_list_data_get(nth);

    openers_set_default(fileinfo->mime_type, handler);

    close_file_context_menu(canvas, false);
}

static void _set_default_handler_draw_item_handler(Evas_Object* choicebox, Evas_Object* item,
                                                   int item_num, int page_position, void* param)
{
    item_clear(item);
    draw_handler_item(choicebox, item, item_num, "%s");
}

static void _set_default_handler(file_context_menu_info_t* info)
{
    /* Replace context menu choicebox with new one */
    choicebox_info_t choicebox_info = {
        NULL,
        "choicebox",
        "settings-right",
        "choicebox",
        "item-settings",
        _set_default_handler_item_handler,
        _set_default_handler_draw_item_handler,
        _page_handler,
        _close_handler
    };

    Evas_Object* file_context_menu = evas_object_name_find(info->state->canvas, "file-context-menu");

    Evas_Object* file_context_choicebox = evas_object_name_find(info->state->canvas, "file-context-menu-choicebox");
    edje_object_part_unswallow(file_context_menu, file_context_choicebox);
    evas_object_del(file_context_choicebox);


    Evas_Object* set_default_handler_choicebox = choicebox_new(info->state->canvas,
                                                               &choicebox_info, info);
    evas_object_name_set(set_default_handler_choicebox, "file-context-menu-choicebox");
    eoi_register_fullscreen_choicebox(set_default_handler_choicebox);

    edje_object_part_swallow(file_context_menu, "contents", set_default_handler_choicebox);

    edje_object_part_text_set(file_context_menu, "title", gettext("Always open with..."));

    choicebox_set_size(set_default_handler_choicebox, info->openers_num);

    evas_object_show(set_default_handler_choicebox);
    evas_object_focus_set(set_default_handler_choicebox, true);
    choicebox_aux_subscribe_key_up(set_default_handler_choicebox);
}

/* End of "Always open with..." */

typedef struct
{
    madshelf_state_t* state;
    delete_file_context_action_t action;
} delete_handler_params_t;

static void _delete_handler(void* param, const char* filename)
{
    delete_handler_params_t* params = param;
    (*params->action)(params->state, filename);
    free(params);
}

static void _item_handler(Evas_Object* choicebox, int item_num, bool is_alt, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "file-context-menu");
    file_context_menu_info_t* info = evas_object_data_get(menu, "info");

    if(item_num < info->openers_num)
    {
        Eina_List* nth = eina_list_nth_list(info->openers->apps, item_num);
        Efreet_Desktop* handler = eina_list_data_get(nth);

        _run_file(info, handler);

        close_file_context_menu(canvas, false);
        return;
    }
    else
        item_num -= info->openers_num;

    if(info->openers_num > 0)
    {
        if(item_num == 0)
        {
            _set_default_handler(info);
            return;
        }
        else
            item_num--;
    }

    if(item_num < info->add_actions_num)
    {
        (*info->handle_action)(info->state, info->filename, item_num, is_alt);
        return;
    }
    else
        item_num -= info->add_actions_num;

    /* Files-only ops */

    if(item_num == 0)
    {
        delete_handler_params_t* params = malloc(sizeof(delete_handler_params_t));
        params->state = info->state;
        params->action = info->delete_action;

        madshelf_state_t* state = info->state;
        char* filename = strdup(info->filename);
        close_file_context_menu(canvas, true);
        delete_file_dialog(state, filename, _delete_handler, params);
    }
    else
        item_num--;
}

static void _draw_item_handler(Evas_Object* choicebox, Evas_Object* item, int item_num,
                               int page_position, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "file-context-menu");
    file_context_menu_info_t* info = evas_object_data_get(menu, "info");

    item_clear(item);

    if(item_num < info->openers_num)
    {
        draw_handler_item(choicebox, item, item_num,
                          item_num ? gettext("Open with %s") : gettext("Open (with %s)"));
        return;
    }
    else
        item_num -= info->openers_num;

    if(info->openers_num > 0)
    {
        if(item_num == 0)
        {
            edje_object_part_text_set(item, "title", gettext("Always open with..."));
            return;
        }
        else
            item_num --;
    }

    if(item_num < info->add_actions_num)
    {
        (*info->draw_action)(info->state, item, info->filename, item_num);
        return;
    }
    else
        item_num -= info->add_actions_num;

    /* Files-only ops */

    if(item_num == 0) /* Delete */
    {
        edje_object_part_text_set(item, "title", gettext("Delete"));
        return;
    }
    else
        item_num -= 1;

    errx(1, "_draw_item_handler: Unknown item: %d\n", item_num);
}

void open_file_context_menu(madshelf_state_t* state,
                            const char* title,
                            const char* filename,
                            int add_actions_num,
                            draw_file_context_action_t draw_action,
                            handle_file_context_action_t handle_action,
                            delete_file_context_action_t delete_action,
                            file_context_menu_closed_t closed)
{
    file_context_menu_info_t* info = malloc(sizeof(file_context_menu_info_t));
    info->state = state;
    info->filename = strdup(filename);
    info->add_actions_num = add_actions_num;
    info->draw_action = draw_action;
    info->handle_action = handle_action;
    info->delete_action = delete_action;
    info->closed = closed;
    info->fileop_targets_num = get_fileop_targets_num(info);
    info->fileinfo = fileinfo_create(filename);
    if(!ecore_file_exists(filename) || ecore_file_is_dir(filename))
    {
        info->openers_num = 0;
        info->openers = NULL;
    }
    else
    {
        info->openers = openers_get(info->fileinfo->mime_type);
        if(info->openers)
            info->openers_num = eina_list_size(info->openers->apps);
        else
            info->openers_num = 0;
    }

    Evas_Object* main_edje = evas_object_name_find(state->canvas, "main_edje");

    Evas_Object* file_context_menu = eoi_settings_right_create(state->canvas);
    evas_object_name_set(file_context_menu, "file-context-menu");
    evas_object_data_set(file_context_menu, "info", info);

    edje_object_part_text_set(file_context_menu, "title", title);

    choicebox_info_t choicebox_info = {
        NULL,
        "choicebox",
        "settings-right",
        "choicebox",
        "item-settings",
        _item_handler,
        _draw_item_handler,
        _page_handler,
        _close_handler
    };

    Evas_Object* file_context_menu_choicebox
        = choicebox_new(state->canvas, &choicebox_info, info);
    eoi_register_fullscreen_choicebox(file_context_menu_choicebox);

    evas_object_name_set(file_context_menu_choicebox, "file-context-menu-choicebox");
    edje_object_part_swallow(file_context_menu, "contents", file_context_menu_choicebox);
    edje_object_part_swallow(main_edje, "right-overlay", file_context_menu);

    int actions_num = add_actions_num
        + info->openers_num;

    if(!ecore_file_is_dir(filename))
        actions_num +=
//        + 2*info->fileop_targets /* Copy, move */
            + (info->openers_num > 0 ? 1 : 0) /* Always open with */
            + 1 /* Delete */
            ;

    choicebox_set_size(file_context_menu_choicebox, actions_num);

    evas_object_show(file_context_menu_choicebox);
    evas_object_show(file_context_menu);

    evas_object_focus_set(file_context_menu_choicebox, true);
    choicebox_aux_subscribe_key_up(file_context_menu_choicebox);
}

void close_file_context_menu(Evas* canvas, bool touched)
{
    Evas_Object* main_edje = evas_object_name_find(canvas, "main_edje");
    Evas_Object* file_context_menu = evas_object_name_find(canvas, "file-context-menu");
    Evas_Object* file_context_choicebox = evas_object_name_find(canvas, "file-context-menu-choicebox");

    if(!file_context_menu)
        return;

#ifdef DEBUG
    dump_evas_hier(canvas);
#endif

    file_context_menu_info_t* info = evas_object_data_get(file_context_menu, "info");

    edje_object_part_unswallow(main_edje, file_context_menu);
    edje_object_part_unswallow(file_context_menu, file_context_choicebox);
    evas_object_del(file_context_menu);
    evas_object_del(file_context_choicebox);

    _return_focus_to_main_choicebox(canvas);

    (*info->closed)(info->state, info->filename, touched);
    fileinfo_destroy(info->fileinfo);
    free(info->filename);
    free(info);
}
