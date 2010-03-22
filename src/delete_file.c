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
#include <string.h>
#include <libintl.h>
#include <err.h>

#include <Evas.h>
#include <Edje.h>

#include "delete_file.h"
#include "dir.h"

#include <libeoi.h>
#include <libeoi_themes.h>
#include <libeoi_dialog.h>

typedef struct
{
    char* filename;
    void* param;
    delete_file_handler_t delete_handler;
    madshelf_state_t* state;
} delete_key_up_params_t;

static void delete_key_up(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    delete_key_up_params_t* params = (delete_key_up_params_t*)param;
    Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
    const char* action = keys_lookup(params->state->keys, "delete-confirm", ev->keyname);

    if(action && !strcmp(action, "Confirm"))
        (*params->delete_handler)(params->param, params->filename);
    else if(action && !strcmp(action, "Cancel"))
        ; /* nothing */
    else
        return;

    Evas_Object *text = evas_object_name_find(e, "delete-confirm-text");
    Evas_Object *icon = evas_object_name_find(e, "delete-confirm-icon");

    evas_object_focus_set(evas_object_name_find(e, "contents"), true);
    evas_object_hide(o);

    edje_object_part_unswallow(o, text);
    evas_object_del(text);

    edje_object_part_unswallow(o, icon);
    evas_object_del(icon);

    evas_object_del(o);

    free(params->filename);
    free(params);
}

/* malloc-ed filename should be passed */
void delete_file_dialog(madshelf_state_t* state, char* filename,
                        delete_file_handler_t delete_handler, void* param)
{
    delete_key_up_params_t* params = malloc(sizeof(*params));
    params->filename = filename;
    params->param = param;
    params->delete_handler = delete_handler;
    params->state = state;

    Evas* canvas = state->canvas;

    Evas_Object* text = eoi_create_themed_edje(canvas, "madshelf", "delete-confirm-text");
    evas_object_name_set(text, "delete-confirm-text");
    if (!text)
        errx(1, "Unable to open theme madshelf(delete-confirm-text)");

    edje_object_part_text_set(text, "text",
                              gettext("Press \"OK\" to delete file<br><br>Press \"C\" to cancel"));

    Evas_Object *dlg = eoi_dialog_create("dlg", text);
    eoi_dialog_title_set(dlg, gettext("Delete file?"));
    evas_object_focus_set(dlg, true);
    evas_object_event_callback_add(dlg, EVAS_CALLBACK_KEY_UP,
                                   &delete_key_up, params);

    Evas_Object *icon = eoi_create_themed_edje(canvas, "madshelf", "delete-confirm-icon");
    evas_object_name_set(icon, "delete-confirm-icon");
    edje_object_part_swallow(dlg, "icon", icon);

    Ecore_Evas *window = ecore_evas_ecore_evas_get(canvas);
    eoi_fullwindow_object_register(window, dlg);

    evas_object_move(dlg, 0, 0);
    int w, h;
    evas_output_size_get(canvas, &w, &h);
    evas_object_resize(dlg, w, h);

    evas_object_show(dlg);
}
