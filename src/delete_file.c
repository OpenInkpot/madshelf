#include <string.h>
#include <unistd.h>
#include <libintl.h>

#include <Evas.h>
#include <Edje.h>

#include "delete_file.h"
#include "dir.h"

typedef struct
{
   char* filename;
   madshelf_state_t* state;
} delete_key_up_params_t;

static void delete_key_up(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    delete_key_up_params_t* params = (delete_key_up_params_t*)param;
    Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
    const char* action = keys_lookup(params->state->keys, "delete-confirm", ev->keyname);

    if(action && !strcmp(action, "Confirm"))
        unlink(params->filename);
    else if(action && !strcmp(action, "Cancel"))
        ; /* nothing */
    else
        return;

    evas_object_focus_set(evas_object_name_find(e, "contents"), true);
    evas_object_del(o);
    go(params->state, dir_refresh(params->state));

    free(params->filename);
    free(params);
}

/* malloc-ed filename should be passed */
void delete_file(madshelf_state_t* state, char* filename)
{
    delete_key_up_params_t* params = malloc(sizeof(params));
    params->filename = filename;
    params->state = state;

    Evas* canvas = state->canvas;

    Evas_Object* wnd = edje_object_add(canvas);
    edje_object_file_set(wnd, THEMEDIR "/delete-confirm.edj", "delete-confirm-window");

    evas_object_name_set(wnd, "delete-confirm-window");
    evas_object_focus_set(wnd, true);
    evas_object_event_callback_add(wnd, EVAS_CALLBACK_KEY_UP,
                                   &delete_key_up, params);

    edje_object_part_text_set(wnd, "title", gettext("Delete file?"));
    edje_object_part_text_set(wnd, "text",
                              gettext("Press \"OK\" to delete file<br><br>Press \"C\" to cancel"));

    evas_object_move(wnd, 0, 0);
    int w, h;
    evas_output_size_get(canvas, &w, &h);
    evas_object_resize(wnd, w, h);

    evas_object_show(wnd);
}
