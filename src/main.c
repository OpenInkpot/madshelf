/*
 * MadShelf - bookshelf application.
 *
 * Copyright (C) 2008 by Marc Lajoie
 * Copyright © 2008,2009,2010 Mikhail Gusarov <dottedmag@dottedmag.net>
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
#include <locale.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>
#include <err.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include <Ecore_Config.h>
#include <Ecore_Con.h>
#include <Ecore_X.h>
#include <Edje.h>
#include <Efreet.h>
#include <libchoicebox.h>
#include <libeoi.h>
#include <libeoi_help.h>
#include <libeoi_clock.h>
#include <libeoi_battery.h>
#include <libeoi_themes.h>

#include "madshelf.h"

#include "overview.h"
#include "fileinfo.h"
#include "utils.h"
#include "handlers.h"
#include "curdir.h"
#include "dir.h"
#include "app_defaults.h"
#include "recent.h"
#include "favorites.h"
#include "favorites_menu.h"

#define SYS_CONFIG_DIR SYSCONFDIR "/madshelf"
#define USER_CONFIG_DIR "/.e/apps/madshelf"

#define CURDIR_DB_NAME "dir-state.db"
#define DISKS_CONFIG_NAME "disks.conf"
#define PREFS_CONFIG_NAME "prefs.conf"
#define TAGS_DB_NAME "tags.db"

/* Uhm? */
void item_clear(Evas_Object* item)
{
    edje_object_signal_emit(item, "set-icon", "none");
    edje_object_part_text_set(item, "center-caption", "");
    edje_object_part_text_set(item, "title", "");
    edje_object_part_text_set(item, "author", "");
    edje_object_part_text_set(item, "series", "");
    edje_object_part_text_set(item, "type", "");
    edje_object_part_text_set(item, "size", "");
    edje_object_part_text_set(item, "progress", "");
}

/* FIXME */

static void update_gui(madshelf_state_t* state)
{
    (*state->loc->update_gui)(state);
}

void go(madshelf_state_t* state, madshelf_loc_t* loc)
{
    if(state->loc && state->loc->free)
        (*state->loc->free)(state);

    state->loc = loc;

    (*state->loc->init_gui)(state);
    update_gui(state);
}

void set_show_hidden(madshelf_state_t* state, bool show_hidden)
{
    state->show_hidden = show_hidden;
    ecore_config_boolean_set("show-hidden", show_hidden);
    ecore_config_save();
}

void set_sort(madshelf_state_t* state, madshelf_sort_t sort)
{
    state->sort = sort;
    ecore_config_int_set("sort", (int)sort);
    ecore_config_save();
}

void set_favorites_sort(madshelf_state_t* state, madshelf_sort_t sort)
{
    state->favorites_sort = sort;
    ecore_config_int_set("favorites-sort", (int)sort);
    ecore_config_save();
}

void set_recent_sort(madshelf_state_t* state, madshelf_sort_t sort)
{
    state->recent_sort = sort;
    ecore_config_int_set("recent-sort", (int)sort);
    ecore_config_save();
}

void set_disk_current_path(madshelf_disk_t* disk, const char* path)
{
    free(disk->current_path);
    disk->current_path = strdup(path);
    ecore_config_string_set(disk->path, disk->current_path);
    ecore_config_save();
}

static void free_state(madshelf_state_t* state)
{
    if(state->loc->free)
        (*state->loc->free)(state);

    keys_free(state->keys);
    tags_fini(state->tags);
    free_disks(state->disks);
}


static void
help(madshelf_state_t *state)
{
    eoi_help_show(state->canvas, "madshelf", NULL,
                  gettext("Bookshelf"), NULL, NULL);
}

static void contents_draw_item_handler(Evas_Object* choicebox, Evas_Object* item,
                    int item_num, int page_position, void* param)
{
    const madshelf_state_t* state = (const madshelf_state_t*)param;
    (*state->loc->draw_item)(state, item, item_num);
}

static void contents_page_handler(Evas_Object* choicebox, int cur_page,
                                   int total_pages, void* param)
{
    Evas* canvas = evas_object_evas_get(choicebox);

    Evas_Object* footer = evas_object_name_find(canvas, "main_edje");
    choicebox_aux_edje_footer_handler(footer, "footer", cur_page, total_pages);
}

static void contents_item_handler(Evas_Object* choicebox, int item_num,
                                  bool is_alt, void* param)
{
    madshelf_state_t* state = (madshelf_state_t*)param;
    (*state->loc->activate_item)(state, choicebox, item_num, is_alt);
}

static void contents_close_handler(Evas_Object* choicebox, void* param)
{
    madshelf_state_t* state = (madshelf_state_t*)param;
    if(state->loc->request_exit)
        (*state->loc->request_exit)(state, choicebox);
    ecore_main_loop_quit();
}

static void main_win_close_handler(Ecore_Evas* main_win)
{
    ecore_main_loop_quit();
}

static void update_gui_on_resize(Ecore_Evas *main_win __attribute__((unused)),
                                Evas_Object *obj,
                                int w,
                                int h,
                                void *param)
{
    /* Resize main edje, then update header/hooter, because
       eoi_edje_text_trim_left() require object to be _already_ resized
       properly

       This hack still exists, until EDJE will be fixed
       */
    evas_object_resize(obj, w, h);
    update_gui((madshelf_state_t *)param);
}

static void contents_key_up(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    madshelf_state_t* state = (madshelf_state_t*)param;
    Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;

    const char* action = keys_lookup(state->keys, "main", ev->keyname);
    if(action && !strcmp(action, "Help"))
    {
        help(state);
        return;
    }

    if(state->loc->key_up && (*state->loc->key_up)(state, o, ev))
        return;

    choicebox_aux_key_up_handler(o, ev);
}


static void load_config(madshelf_state_t* state)
{
    ecore_config_int_default("sort", (int)MADSHELF_SORT_NAME);
    ecore_config_int_default("favorites-sort", (int)MADSHELF_SORT_DATE);
    ecore_config_int_default("recent-sort", (int)MADSHELF_SORT_DATE);
    ecore_config_boolean_default("show-hidden", false);

    ecore_config_load();

    state->sort = ecore_config_int_get("sort");
    if (state->sort < 0 || state->sort > MADSHELF_SORT_NAMEREV)
        state->sort = MADSHELF_SORT_NAME;
    state->favorites_sort = ecore_config_int_get("favorites-sort");
    if (state->favorites_sort < 0 || state->favorites_sort > MADSHELF_SORT_DATE)
        state->favorites_sort = MADSHELF_SORT_DATE;
    state->recent_sort = ecore_config_int_get("recent-sort");
    if (state->recent_sort < 0 || state->recent_sort > MADSHELF_SORT_DATE)
        state->recent_sort = MADSHELF_SORT_DATE;
    state->show_hidden = ecore_config_boolean_get("show-hidden");

    int i;
    for(i = 0; i < state->disks->n; ++i)
    {
        char* cur_path = ecore_config_string_get(state->disks->disk[i].path);
        if(cur_path)
            state->disks->disk[i].current_path = cur_path;
        else
            state->disks->disk[i].current_path = NULL;
    }
}

static madshelf_disks_t* load_disks()
{
    char user_config_filename[PATH_MAX];
    snprintf(user_config_filename, PATH_MAX,
             "%s" USER_CONFIG_DIR "/" DISKS_CONFIG_NAME, getenv("HOME"));

    Efreet_Ini* disks_config = NULL;

    if(ecore_file_exists(user_config_filename))
        disks_config = efreet_ini_new(user_config_filename);

    if(!disks_config && ecore_file_exists(SYS_CONFIG_DIR "/" DISKS_CONFIG_NAME))
       disks_config = efreet_ini_new(SYS_CONFIG_DIR "/" DISKS_CONFIG_NAME);

    madshelf_disks_t* disks;
    if(disks_config)
    {
        disks = fill_disks(disks_config);
        efreet_ini_free(disks_config);
    }
    else
        disks = fill_stub_disk();
    return disks;
}

static bool
load_prefs()
{
    char user_prefs_filename[PATH_MAX];
    snprintf(user_prefs_filename, PATH_MAX,
             "%s" USER_CONFIG_DIR "/" PREFS_CONFIG_NAME, getenv("HOME"));

    Efreet_Ini *prefs_config = NULL;
    if (ecore_file_exists(user_prefs_filename))
        prefs_config = efreet_ini_new(user_prefs_filename);

    if (!prefs_config && ecore_file_exists(SYS_CONFIG_DIR "/" PREFS_CONFIG_NAME))
        prefs_config = efreet_ini_new(SYS_CONFIG_DIR "/" PREFS_CONFIG_NAME);

    bool menu_navigation = false;

    if (prefs_config) {
        efreet_ini_section_set(prefs_config, "Interface");
        menu_navigation = efreet_ini_boolean_get(prefs_config, "Menu-Navigation");
        efreet_ini_free(prefs_config);
    }

    return menu_navigation;
}

madshelf_loc_t*
find_first_mounted_disk(madshelf_state_t *state)
{
    for (int i = 0; i < state->disks->n; ++i) {
        madshelf_disk_t *disk = state->disks->disk + i;
        if (disk_mounted(disk)) {
            const char *path = disk->current_path ? disk->current_path : disk->path;
            return dir_make(state, path);
        }
    }

    /* Ugh. Let's use first disk */
    const char *path = state->disks->disk->path;
    return dir_make(state, path);
}

typedef struct
{
    char* msg;
    int size;
} client_data_t;

static Eina_Bool _client_add(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Add* e = ev;
    client_data_t* msg = malloc(sizeof(client_data_t));
    msg->msg = strdup("");
    msg->size = 0;
    ecore_con_client_data_set(e->client, msg);
    return 0;
}

static Eina_Bool _client_del(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Del* e = ev;
    client_data_t* msg = ecore_con_client_data_get(e->client);

    /* Handle */

    char *cmdline = strndup(msg->msg, msg->size);
    char **cmds = eina_str_split(cmdline ,"\n", 0);
    free(cmdline);
    madshelf_loc_t* location = NULL;
    madshelf_loc_type_t loc_type = MADSHELF_LOC_DIR;
    madshelf_state_t* state = (madshelf_state_t*)param;

    /* restore navigation state */
    state->menu_navigation = state->menu_navigation_prefs;

    char *folder = NULL;
    int i=0;
    while(cmds[i])
    {
        char *cmd = cmds[i];
        if(!strcmp(cmd, "books"))
            state->filter = MADSHELF_FILTER_BOOKS;
        else if(!strcmp(cmd, "audio"))
            state->filter = MADSHELF_FILTER_AUDIO;
        else if(!strcmp(cmd, "image"))
            state->filter = MADSHELF_FILTER_IMAGE;
        else if(cmd[0] == '/')
        {
            folder = strdup(cmd);
            loc_type = MADSHELF_LOC_DIR;
        }
        else if(!strcmp(cmd, "recent"))
            loc_type = MADSHELF_LOC_RECENT;
        else if(!strcmp(cmd, "favorites"))
            loc_type = MADSHELF_LOC_FAVORITES;
        else if(!strcmp(cmd, "overview"))
            loc_type = MADSHELF_LOC_OVERVIEW;
        i++;
    }

    switch(loc_type) {
        case MADSHELF_LOC_FAVORITES:
            location = favorites_make(state, state->filter);
            state->menu_navigation = true;
            break;

        case MADSHELF_LOC_RECENT:
            location = recent_make(state);
            state->menu_navigation = true;
            break;

        default:  /* case MADSHELF_LOC_DIR: */
            if(folder)
                location = dir_make(state, folder);
            else
                if (state->menu_navigation) {
                    location = find_first_mounted_disk(state);
                } else {
                    location =  overview_make(state);
                }
            break;
    }
    if(location)
        go(state, location);
    free(folder);
    free(*cmds);
    free(cmds);

    Ecore_Evas* win = state->win;

    ecore_evas_show(win);
    ecore_evas_raise(win);

    free(msg->msg);
    free(msg);
    return 0;
}

static Eina_Bool _client_data(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Data* e = ev;
    client_data_t* msg = ecore_con_client_data_get(e->client);
    msg->msg = realloc(msg->msg, msg->size + e->size);
    memcpy(msg->msg + msg->size, e->data, e->size);
    msg->size += e->size;
    return 0;
}

static bool
send_line_to_running_instance(const char *line)
{
    Ecore_Con_Server* server;
    server = ecore_con_server_connect(ECORE_CON_LOCAL_USER,
                                      "madshelf-singleton", 0, NULL);

    if(!server)
        return false;
    ecore_con_server_send(server, line, strlen(line));
    ecore_con_server_flush(server);
    ecore_con_server_del(server);

    return true;

}

static bool check_running_instance(madshelf_filter_t filter,
    const char *folder, madshelf_loc_type_t default_location)
{
    Ecore_Con_Server* server
        = ecore_con_server_add(ECORE_CON_LOCAL_USER, "madshelf-singleton", 0, NULL);

    const char *filters[] = {
        "no", "books", "image", "audio",
    };

    const char *locs[] = {
        "overview", "dir", "favmenu", "favorites", "recent",
    };

    if(!server)
    {
        /* Somebody already listens there */
        char *str;
        size_t strsize = 0;
        FILE *file = open_memstream(&str, &strsize);

        fprintf(file, "%s\n%s", filters[filter], locs[default_location]);
        if(folder)
            fprintf(file, "\n%s", folder);

        fclose(file);
        if(!send_line_to_running_instance(str))
            return false;
        return true;
    }

    return false;
}

static Eina_Bool sighup_signal_handler(void* data, int type, void* event)
{
    madshelf_state_t* state = (madshelf_state_t*)data;

    if(state->loc->mounts_updated)
        (*state->loc->mounts_updated)(state);

    /* Purge "current position in directory" cache when SD card is inserted, as
     * it might have been out-of-date */

    curdir_fini();
    curdir_init(":memory:");

    return 1;
}

static struct option options[] = {
    { "overview", false, NULL, 'O'},
    { "recent", false, NULL, 'R'},
    { "favorites", false, NULL, 'F'},
    { "filter", true, NULL, 'f' },
    { NULL, 0, 0, 0 }
};

static void exit_all(void *param)
{
    ecore_main_loop_quit();
}

static Eina_Bool exit_handler(void *param, int ev_type, void *event)
{
   ecore_main_loop_quit();
   return 1;
}

static void exit_app(void* param)
{
    exit(1);
}

int main(int argc, char** argv)
{
    madshelf_filter_t filter = MADSHELF_FILTER_NO;
    madshelf_loc_type_t default_location = MADSHELF_LOC_DIR;
    char *folder = NULL;

    /* mdev handler may send SIGUSR1/2 before we have got the handler for those
     * signals. Let's work around it by ignoring SIGUSR1/2 at the startup. Those
     * signals are nothing but "re-read your state", and if we are starting up,
     * then we have nothing to re-read */
    struct sigaction ignore = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART
    };

    sigaction(SIGUSR1, &ignore, NULL);
    sigaction(SIGUSR2, &ignore, NULL);

    int option_index = 0;
    while(1)
    {
        int c = getopt_long(argc, argv, "f:RFO", options, &option_index);
        if(c == -1)
            break;
        if(c == 'R')
            default_location = MADSHELF_LOC_RECENT;
        else if(c == 'F')
            default_location = MADSHELF_LOC_FAVORITES;
        else if(c == 'O')
            default_location = MADSHELF_LOC_OVERVIEW;
        else if(c == 'f')
        {
            if(!strcmp(optarg, "books"))
                filter = MADSHELF_FILTER_BOOKS;
            else if(!strcmp(optarg, "image"))
                filter = MADSHELF_FILTER_IMAGE;
            else if(!strcmp(optarg, "audio"))
                filter = MADSHELF_FILTER_AUDIO;
            else
            {
                fprintf(stderr, "Unknown filter type: %s\n", optarg);
                return 1;
            }
        }

        else if(c == '?')
            return 1;
        else
        {
            fprintf(stderr, "Unexpected getopt return code: %d (%c)\n", c, c);
            return 1;
        }
    }
    if(optind < argc)
    {
        int old_optind =optind;
        while(old_optind < argc)
            printf("arg=%s\n", argv[old_optind++]);
        folder = argv[optind];
        if(folder[0] != '/')
        {
            fprintf(stderr, "%s must me an absolute path", folder);
            return 1;
        }

        if(!ecore_file_is_dir(folder))
        {
            fprintf(stderr, "%s must me a directory", folder);
            return 1;
        }
    }

    if(ecore_config_init("madshelf") != ECORE_CONFIG_ERR_SUCC)
        errx(1, "Unable to initialize Ecore_Config");

    if (!ecore_x_init(NULL))
        errx(1, "Unable to initialize Ecore_X, maybe missing DISPLAY");
    if(!ecore_init())
        errx(1, "Unable to initialize Ecore");

    if(check_running_instance(filter, folder, default_location))
    {
        ecore_con_shutdown();
        ecore_shutdown();
        ecore_x_shutdown();
        return 0;
    }

    ecore_x_io_error_handler_set(exit_app, NULL);

    if(!evas_init())
        errx(1, "Unable to initialize Evas");
    if(!edje_init())
        errx(1, "Unable to initialize Edje");
    if(!ecore_evas_init())
        errx(1, "Unable to initialize Ecore_Evas");

    position_engine_init();
    fileinfo_init();

    setlocale(LC_ALL, "");
    textdomain("madshelf");

    char configdir[PATH_MAX];
    snprintf(configdir, PATH_MAX, "%s" USER_CONFIG_DIR, getenv("HOME"));
    ecore_file_mkpath(configdir);

    char tags_db_filename[PATH_MAX];
    snprintf(tags_db_filename, PATH_MAX, "%s/" TAGS_DB_NAME, configdir);

    /* Use in-memory database instead of file-backed as our filesystem is
     * volatile, and caches go out of date quite often. Clean up cache on disk
     * mount/unmount too. */

    /* char curdir_db_filename[PATH_MAX]; */
    /* snprintf(curdir_db_filename, PATH_MAX, "%s/" CURDIR_DB_NAME, configdir); */
    /* curdir_init(curdir_db_filename); */
    curdir_init(":memory:");

    madshelf_state_t state = {};

    state.filter = filter;
    openers_init();
    state.disks = load_disks();
    state.menu_navigation_prefs = load_prefs();
    state.menu_navigation = state.menu_navigation_prefs;
    state.keys = keys_alloc("madshelf");

    load_config(&state);

    char* err = NULL;
    state.tags = tags_init(tags_db_filename, &err);
    if(!state.tags)
        errx(1, "Unable to initialize tags database: %s", err);
    free(err);

    /* End of state */

    int width, height;
    Ecore_X_Screen *screen = ecore_x_default_screen_get();
    ecore_x_screen_size_get(screen, &width, &height);

    Ecore_Evas* main_win = ecore_evas_software_x11_8_new(0, 0, 0, 0, width, height);
    state.win = main_win;
    ecore_evas_title_set(main_win, "Madshelf");
    ecore_evas_name_class_set(main_win, "Madshelf", "Madshelf");
    ecore_evas_callback_delete_request_set(main_win, main_win_close_handler);

    Evas* main_canvas = ecore_evas_get(main_win);
    state.canvas = main_canvas;
    Evas_Object* main_edje = eoi_main_window_create(main_canvas);
    if (!main_edje)
        errx(1, "Unable to load main window theme");

    evas_object_name_set(main_edje, "main_edje");

    evas_object_move(main_edje, 0, 0);
    evas_object_resize(main_edje, width, height);

    /* don't use eoi_fullwindow_object_register -- we need to preserve
       order of edje resizing

       This hack still exists, until EDJE will be fixed
       */
    eoi_resize_object_register(main_win, main_edje, update_gui_on_resize,
            &state);

    /* Add "contents" choicebox */

    choicebox_info_t info = {
        NULL,
        "choicebox",
        "full",
        "madshelf",
        "fileitem",
        contents_item_handler,
        contents_draw_item_handler,
        contents_page_handler,
        contents_close_handler,
    };

    Evas_Object* contents = choicebox_new(main_canvas, &info, &state);
    if (!contents)
        errx(1, "Unable to create choicebox. Bailing out.");

    evas_object_name_set(contents, "contents");
    edje_object_part_swallow(main_edje, "contents", contents);
    evas_object_focus_set(contents, true);
    evas_object_event_callback_add(contents, EVAS_CALLBACK_KEY_UP, &contents_key_up, &state);

    eoi_register_fullscreen_choicebox(contents);

    /* Add "sort" icons */

    Evas_Object *sort_icons
        = eoi_create_themed_edje(main_canvas, "madshelf", "sort-icons");
    evas_object_name_set(sort_icons, "sort-icons");
    evas_object_show(sort_icons);
    edje_object_part_swallow(main_edje, "state-icons", sort_icons);

    /* Let's go */

    if(folder)
        go(&state, dir_make(&state, folder));
    else {

        switch(default_location)
        {
            case MADSHELF_LOC_OVERVIEW:
                state.menu_navigation = false;
                go(&state, overview_make(&state));
                break;

            case MADSHELF_LOC_RECENT:
                state.menu_navigation = true;
                go(&state, recent_make(&state));
                break;

            case MADSHELF_LOC_FAVORITES:
                state.menu_navigation = true;
                go(&state, favorites_make(&state, state.filter));
                break;

            default:
                if (state.menu_navigation) {
                    go(&state, find_first_mounted_disk(&state));
                } else {
                    go(&state, overview_make(&state));
                }
        }
    }

    evas_object_show(contents);
    evas_object_show(main_edje);
    ecore_evas_show(main_win);

    eoi_run_clock(main_edje);
    eoi_run_battery(main_edje);

    ecore_x_io_error_handler_set(exit_all, NULL);
    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_handler, NULL);
    ecore_event_handler_add(ECORE_EVENT_SIGNAL_HUP, sighup_signal_handler, &state);

    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, _client_add, NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, _client_data, NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, _client_del, &state);

    ecore_main_loop_begin();

    ecore_config_save();

    free_state(&state);

    openers_fini();
    curdir_fini();
    fileinfo_fini();
    position_engine_fini();

    appdef_fini();


    /* Keep valgrind happy */
    edje_file_cache_set(0);
    edje_collection_cache_set(0);

    ecore_evas_shutdown();
    evas_shutdown();
    edje_shutdown();
    ecore_shutdown();
    ecore_config_shutdown();
    ecore_x_shutdown();

    return 0;
}

static const char* _signals[] = {
    "sort-mode,none",
    "sort-mode,name",
    "sort-mode,namerev",
    "sort-mode,date",
};

void set_sort_icon(const madshelf_state_t* state, madshelf_icon_sort_t icon)
{
    Evas_Object* sort_icons = evas_object_name_find(state->canvas, "sort-icons");
    edje_object_signal_emit(sort_icons, _signals[icon+1], "");
}
