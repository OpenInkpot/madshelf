#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include <Efreet.h>
#include <Ecore_File.h>

#include "app_defaults.h"

#define LIST "defaults.list"
#define ASSOCIATIONS "Default Applications"
#define REMOVED_ASSOCIATIONS "Removed Associations"

static bool initialized;
static Efreet_Ini* system_defs;
static char* userfile;
static Efreet_Ini* user_defs;

static void appdef_init()
{
    efreet_init();

    system_defs = efreet_ini_new("/usr/share/applications/defaults.list");
    asprintf(&userfile, "%s/.local/share/applications/defaults.list",
             getenv("HOME"));
    user_defs = efreet_ini_new(userfile);

    initialized = true;
}

void appdef_fini()
{
    if(initialized)
    {
        free(userfile);
        efreet_ini_free(user_defs);
        efreet_ini_free(system_defs);
        efreet_shutdown();

        initialized = false;
    }
}

const char* appdef_get_preferred(const char* mime_type)
{
    Eina_List* res = appdef_get_list(mime_type);
    if(!res)
        return NULL;

    const char* preferred = res->data;
    res = eina_list_remove_list(res, res);

    char* data;
    EINA_LIST_FREE(res, data)
        free(data);

    return preferred;
}

Eina_List* appdef_get_list(const char* mime_type)
{
    if(!initialized)
    {
        appdef_init();
    }

    if(efreet_ini_section_set(user_defs, REMOVED_ASSOCIATIONS))
    {
        if(efreet_ini_string_get(user_defs, mime_type))
            return NULL;
    }

    if(efreet_ini_section_set(user_defs, ASSOCIATIONS))
    {
        const char* desktops = efreet_ini_string_get(user_defs, mime_type);
        if(desktops)
            return efreet_desktop_string_list_parse(desktops);
    }

    if(efreet_ini_section_set(system_defs, ASSOCIATIONS))
    {
        const char* desktops = efreet_ini_string_get(system_defs, mime_type);
        if(desktops)
            return efreet_desktop_string_list_parse(desktops);
    }

    return NULL;
}

/* FIXME: move to Efreet */
static void efreet_ini_key_del(Efreet_Ini* ini, const char* key)
{
    if(!ini || !key || !ini->section) return;
    eina_hash_del(ini->section, key, NULL);
}


void appdef_set_default(const char* mime_type, const char* app)
{
    /* Get existing associations' list and add our type to the front */
    Eina_List* handlers = appdef_get_list(mime_type);
    char* data;
    Eina_List* l, * l_next;
    EINA_LIST_FOREACH_SAFE(handlers, l, l_next, data)
        if(!strcmp(app, data))
            handlers = eina_list_remove_list(handlers, l);
    handlers = eina_list_prepend(handlers, eina_stringshare_add(app));

    /* Save new list to user's config */

    /* Remove mime_type from [Removed Associations] if any */
    if(efreet_ini_section_set(user_defs, REMOVED_ASSOCIATIONS))
    {
        if(efreet_ini_string_get(user_defs, mime_type))
            efreet_ini_key_del(user_defs, mime_type);
    }

    /* Add new list to config */
    if(!efreet_ini_section_set(user_defs, ASSOCIATIONS))
    {
        efreet_ini_section_add(user_defs, ASSOCIATIONS);
        efreet_ini_section_set(user_defs, ASSOCIATIONS);
    }
    char* handlers_str = efreet_desktop_string_list_join(handlers);
    efreet_ini_string_set(user_defs, mime_type, handlers_str);
    free(handlers_str);

    eina_list_free(handlers);

    /* Save config */
    char* userfile2 = strdup(userfile);
    char* dir = dirname(userfile2);
    ecore_file_mkpath(dir);
    free(userfile2);

    efreet_ini_save(user_defs, userfile);
}
