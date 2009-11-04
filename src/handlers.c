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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <Efreet_Mime.h>
#include <Efreet.h>
#include <Ecore_File.h>
#include <Eina.h>


#include "handlers.h"
#include "app_defaults.h"

#define DESKTOP_DIR "/usr/share/applications"

#define NO_HANDLER ((Efreet_Desktop*)-1)

typedef struct
{
    /* Programs may handle more than one MIME type, so keep them in separate
       list to free later instead of freeing when hash is destroyed */
    Eina_List* desktop_files;

    Eina_Hash* handlers;
    Eina_Hash* handlers_types;
} appdb_t;

static appdb_t appdb;

static void free_openers(void* data)
{
    openers_t* openers = (openers_t*)data;
    eina_list_free(openers->apps);
    free(openers);
}

#ifdef DEBUG_HANDLERS
static Eina_Bool handler_dump(const Eina_Hash* hash, const void* key, void* data, void* fdata)
{
    const char* mime_type = (const char*)key;
    Eina_List* i = ((openers_t*)data)->apps;

    fprintf(stderr, "%s (%d):\n", mime_type, ((openers_t*)data)->app_types);

    for(; i; i = eina_list_next(i))
    {
        Efreet_Desktop* d = (Efreet_Desktop*)eina_list_data_get(i);
        fprintf(stderr, " %s (%s)\n", d->name, d->exec);
    }
    return 1;
}

static void handlers_dump()
{
    eina_hash_foreach(appdb.handlers, &handler_dump, NULL);
}
#endif

static openers_app_type_t app_type(Efreet_Desktop* d)
{
    openers_app_type_t types = OPENERS_TYPE_NONE;

    Eina_List* i = d->categories;
    for(; i; i = eina_list_next(i))
    {
        const char* cat = eina_list_data_get(i);
        if(!strcmp(cat, "Literature"))
            types |= OPENERS_TYPE_BOOKS;
        if(!strcmp(cat, "Graphics"))
            types |= OPENERS_TYPE_IMAGE;
        if(!strcmp(cat, "Audio"))
            types |= OPENERS_TYPE_AUDIO;
    }

    return types;
}

static int search_list_str(Efreet_Desktop* lhs, char* rhs)
{
    return strcmp(basename(lhs->orig_path), rhs);
}

static Eina_List* openers_resort(Eina_List* apps, const char* mime_type)
{
    Eina_List* appdef = appdef_get_list(mime_type);
    if(!appdef)
        return NULL;

    Eina_List* l, *l_next;
    char* desktop;
    EINA_LIST_FOREACH_SAFE(appdef, l, l_next, desktop)
    {
        Eina_List* p = eina_list_search_unsorted_list(apps, EINA_COMPARE_CB(search_list_str), desktop);
        if(p)
        {
            Efreet_Desktop* d = p->data;
            apps = eina_list_remove_list(apps, p);
            apps = eina_list_prepend(apps, d);
            return apps;
        }
    }
    return NULL;
}

static Eina_Bool sort_handler(const Eina_Hash* hash, const void* key, void* data, void* fdata)
{
    const char* mime_type = key;
    openers_t* op = data;

    op->apps = openers_resort(op->apps, mime_type);

    return 1;
}

void openers_init()
{
    if(!efreet_init())
        exit(1);

    appdb.handlers = eina_hash_string_superfast_new(&free_openers);

    Eina_List* ls = ecore_file_ls(DESKTOP_DIR);

    for(; ls; ls = eina_list_next(ls))
    {
        const char* f = eina_list_data_get(ls);
        char filename[1024];
        snprintf(filename, 1024, "%s/%s", DESKTOP_DIR, f);

        if(ecore_file_is_dir(filename))
            continue;

        Efreet_Desktop* d = efreet_desktop_get(filename);
        if(!d)
            continue;

        Eina_List* j;
        for(j = d->mime_types; j; j = eina_list_next(j))
        {
            const char* mime_type = (const char*)eina_list_data_get(j);
            openers_t* op = eina_hash_find(appdb.handlers, mime_type);
            if(op)
            {
                op->app_types |= app_type(d);
                op->apps = eina_list_append(op->apps, d);

                eina_hash_modify(appdb.handlers, mime_type, op);
            }
            else
            {
                op = malloc(sizeof(openers_t));
                op->app_types = app_type(d);
                op->has_default = false;
                op->apps = eina_list_append(NULL, d);

                eina_hash_add(appdb.handlers, mime_type, op);
            }
        }

        appdb.desktop_files = eina_list_append(appdb.desktop_files, d);
    }

    eina_list_free(ls);

    eina_hash_foreach(appdb.handlers, &sort_handler, NULL);

#ifdef DEBUG_HANDLERS
    handlers_dump();
#endif
}

openers_t* openers_get(const char* mime_type)
{
    return eina_hash_find(appdb.handlers, mime_type);
}

void openers_set_default(const char* mime_type, Efreet_Desktop* desktop)
{
    appdef_set_default(mime_type, basename(desktop->orig_path));

    openers_t* openers = openers_get(mime_type);
    if(openers)
    {
        Eina_List* newapps = openers_resort(openers->apps, mime_type);
        if(newapps)
        {
            openers->has_default = true;
            openers->apps = newapps;
        }
    }
}

void openers_fini()
{
    eina_hash_free(appdb.handlers);

    Eina_List* i;
    for(i = appdb.desktop_files; i; i = eina_list_next(i))
    {
        Efreet_Desktop* desktop = (Efreet_Desktop*)eina_list_data_get(i);
        efreet_desktop_free(desktop);
    }
    eina_list_free(appdb.desktop_files);

    efreet_shutdown();
}
