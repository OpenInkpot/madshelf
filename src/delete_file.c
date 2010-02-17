#include <string.h>
#include <libintl.h>
#include <err.h>

#include <Evas.h>
#include <Edje.h>

#include "delete_file.h"
#include "dir.h"
#include <libeoi.h>
#include <libeoi_themes.h>

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

    evas_object_focus_set(evas_object_name_find(e, "contents"), true);
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

    Evas_Object* wnd = eoi_create_themed_edje(canvas, "madshelf", "delete-confirm-window");
    if (!wnd)
        errx(1, "Unable to open theme madshelf(delete-confirm-window)");

    evas_object_name_set(wnd, "delete-confirm-window");
    evas_object_focus_set(wnd, true);
    evas_object_event_callback_add(wnd, EVAS_CALLBACK_KEY_UP,
                                   &delete_key_up, params);

    edje_object_part_text_set(wnd, "title", gettext("Delete file?"));
    edje_object_part_text_set(wnd, "text",
                              gettext("Press \"OK\" to delete file<br><br>Press \"C\" to cancel"));

    Ecore_Evas *window = ecore_evas_ecore_evas_get(canvas);
    eoi_fullwindow_object_register(window, wnd);

    evas_object_move(wnd, 0, 0);
    int w, h;
    evas_output_size_get(canvas, &w, &h);
    evas_object_resize(wnd, w, h);

    evas_object_show(wnd);
}
