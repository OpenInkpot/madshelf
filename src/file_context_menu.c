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

#include "file_context_menu.h"

#include <libintl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <Ecore_File.h>
#include <Edje.h>
#include <echoicebox.h>

#include "fileinfo.h"
#include "utils.h"
#include "handlers.h"

typedef struct
{
    madshelf_state_t* state;
    char* filename;

    fileinfo_t* fileinfo;
    Eina_List* handlers;
    int handlers_num;
    int fileop_targets_num;
    int add_actions_num;
    draw_file_context_action_t draw_action;
    handle_file_context_action_t handle_action;
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

static madshelf_disk_t* get_fileop_target(file_context_menu_info_t* info, int num)
{
    madshelf_disks_t* disks = info->state->disks;
    madshelf_disk_t* cur_disk = find_disk(disks, info->filename);

    int i;
    for(i = 0; i < disks->n; ++i)
    {
        if(disks->disk + i == cur_disk) continue;
        if(!disks->disk[i].copy_target) continue;
        if(num-- == 0)
            return disks->disk + i;
    }
    return NULL;
}

static void _return_focus_to_main_choicebox(Evas* canvas)
{
    Evas_Object* main_choicebox = evas_object_name_find(canvas, "contents");
    evas_object_focus_set(main_choicebox, true);
}

static void _run_file(file_context_menu_info_t* info, Efreet_Desktop* handler)
{
    tag_add(info->state->tags, "recent", info->filename);

#ifdef OLD_ECORE
    Ecore_List* l = ecore_list_new();
    ecore_list_append(l, info->filename);
#else
    Eina_List* l = NULL;
    l = eina_list_append(l, info->filename);
#endif

    efreet_desktop_exec(handler, l, NULL);

#ifdef OLD_ECORE
    ecore_list_destroy(l);
#else
    eina_list_free(l);
#endif
}

static void _item_handler(Evas_Object* choicebox, int item_num, bool is_alt, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* menu = evas_object_name_find(canvas, "file-context-menu");
    file_context_menu_info_t* info = evas_object_data_get(menu, "info");

    if(item_num < info->handlers_num)
    {
        Eina_List* nth = eina_list_nth_list(info->handlers, item_num);
        Efreet_Desktop* handler = eina_list_data_get(nth);

        _run_file(info, handler);

        close_file_context_menu(canvas, false);
        return;
    }
    else
        item_num -= info->handlers_num;

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
        unlink(info->filename);
        close_file_context_menu(canvas, true);
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

    if(item_num < info->handlers_num)
    {
        const char* open_text;

        if(item_num == 0) /* Special-case first (preferred) handler */
            open_text = gettext("Open (with %s)");
        else
            open_text = gettext("Open with %s");

        Eina_List* nth = eina_list_nth_list(info->handlers, item_num);
        Efreet_Desktop* handler = eina_list_data_get(nth);

        char* f;
        asprintf(&f, open_text, handler->name);
        edje_object_part_text_set(item, "text", f);
        free(f);

        return;
    }
    else
        item_num -= info->handlers_num;

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
        edje_object_part_text_set(item, "text", gettext("Delete"));
        return;
    }
    else
        item_num -= 1;

    die("_draw_item_handler: Unknown item: %d\n", item_num);
}

static void _page_handler(Evas_Object* choicebox, int cur_page, int total_pages, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* footer = evas_object_name_find(canvas, "file-context-menu");
    choicebox_aux_edje_footer_handler(footer, "footer", cur_page, total_pages);
}

static void _key_down(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;

    if(!strcmp(ev->keyname, "Escape"))
    {
        close_file_context_menu(e, false);
        return;
    }

    choicebox_aux_key_down_handler(o, ev);
}

#ifdef DEBUG
#include <stdio.h>
static void dump_evas_hier(Evas* canvas)
{
    Evas_Object* i = evas_object_top_get(canvas);
    while(i)
    {
        const char* name = evas_object_name_get(i);
        if(!name)
            name = "NULL";

        int x, y, w, h;
        evas_object_geometry_get(i, &x, &y, &w, &h);

        const char* type = evas_object_type_get(i);
        if(!type)
            type = "NULL";

        printf("0x%p %s \"%s\" %d-%d %d-%d\n", i, type, name, x, y, w, h);

        i = evas_object_below_get(i);
    }
}
#endif

void open_file_context_menu(madshelf_state_t* state,
                            const char* title,
                            const char* filename,
                            int add_actions_num,
                            draw_file_context_action_t draw_action,
                            handle_file_context_action_t handle_action,
                            file_context_menu_closed_t closed)
{
    file_context_menu_info_t* info = malloc(sizeof(file_context_menu_info_t));
    info->state = state;
    info->filename = strdup(filename);
    info->add_actions_num = add_actions_num;
    info->draw_action = draw_action;
    info->handle_action = handle_action;
    info->closed = closed;
    info->fileop_targets_num = get_fileop_targets_num(info);
    info->fileinfo = fileinfo_create(filename);
    if(ecore_file_is_dir(filename))
    {
        info->handlers_num = 0;
        info->handlers = NULL;
    }
    else
    {
        info->handlers = handlers_get(state->handlers, info->fileinfo->mime_type);
        info->handlers_num = eina_list_size(info->handlers);
    }

    Evas_Object* main_edje = evas_object_name_find(state->canvas, "main_edje");

    Evas_Object* file_context_menu = edje_object_add(state->canvas);
    evas_object_name_set(file_context_menu, "file-context-menu");
    evas_object_data_set(file_context_menu, "info", info);

    edje_object_file_set(file_context_menu,
                         "/usr/share/madshelf/main_window.edj",
                         "settings");

    edje_object_part_text_set(file_context_menu, "title", title);

    Evas_Object* file_context_menu_choicebox
        = choicebox_new(state->canvas,
                        "/usr/share/echoicebox/echoicebox.edj",
                        "settings-right",
                        _item_handler,
                        _draw_item_handler,
                        _page_handler, info);

    evas_object_name_set(file_context_menu_choicebox, "file-context-menu-choicebox");
    edje_object_part_swallow(file_context_menu, "contents", file_context_menu_choicebox);
    edje_object_part_swallow(main_edje, "right-context-menu", file_context_menu);

    int actions_num = add_actions_num
        + info->handlers_num;

    if(!ecore_file_is_dir(filename))
        actions_num +=
//        + 2*info->fileop_targets /* Copy, move */
        + 1 /* Delete */
        ;

    choicebox_set_size(file_context_menu_choicebox, actions_num);

    evas_object_show(file_context_menu_choicebox);
    evas_object_show(file_context_menu);

    evas_object_event_callback_add(file_context_menu_choicebox, EVAS_CALLBACK_KEY_DOWN, &_key_down, info);
    evas_object_focus_set(file_context_menu_choicebox, true);
}

void close_file_context_menu(Evas* canvas, bool touched)
{
#ifdef DEBUG
    dump_evas_hier(canvas);
#endif

    Evas_Object* main_edje = evas_object_name_find(canvas, "main_edje");
    Evas_Object* file_context_menu = evas_object_name_find(canvas, "file-context-menu");
    Evas_Object* file_context_choicebox = evas_object_name_find(canvas, "file-context-menu-choicebox");

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