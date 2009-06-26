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

#include <Efreet_Mime.h>
#include <Efreet.h>
#include <Ecore_File.h>
#include <Eina.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "handlers.h"

#define DESKTOP_DIR "/usr/share/applications"

#define NO_HANDLER ((Efreet_Desktop*)-1)

struct handlers_t
{
    /* Programs may handle more than one MIME type, so keep them in separate
       list to free later instead of freeing when hash is destroyed */
    Eina_List* desktop_files;

    Eina_Hash* handlers;
};

static void free_handlers_list(void* data)
{
    eina_list_free((Eina_List*)data);
}

#ifdef DEBUG_HANDLERS
static Eina_Bool handler_dump(const Eina_Hash* hash, const void* key, void* data, void* fdata)
{
    const char* mime_type = (const char*)key;
    Eina_List* i = (Eina_List*)data;

    fprintf(stderr, "%s:\n", mime_type);

    for(; i; i = eina_list_next(i))
    {
        Efreet_Desktop* d = (Efreet_Desktop*)eina_list_data_get(i);
        fprintf(stderr, " %s (%s)\n", d->name, d->exec);
    }
    return 1;
}

static void handlers_dump(handlers_t* handlers)
{
    eina_hash_foreach(handlers->handlers, &handler_dump, NULL);
}
#endif

handlers_t* handlers_init()
{
    if(!efreet_init())
        return NULL;

    handlers_t* handlers = malloc(sizeof(handlers_t));
    handlers->desktop_files = NULL;
    handlers->handlers = eina_hash_string_superfast_new(&free_handlers_list);

#ifdef OLD_ECORE
    Ecore_List* ls = ecore_file_ls(DESKTOP_DIR);
#else
    Eina_List* ls = ecore_file_ls(DESKTOP_DIR);
#endif

#ifdef OLD_ECORE
    const char* f;
    while((f = ecore_list_next(ls)))
    {
#else
    for(; ls; ls = eina_list_next(ls))
    {
        const char* f = eina_list_data_get(ls);
#endif
        char filename[1024];
        snprintf(filename, 1024, "%s/%s", DESKTOP_DIR, f);

        if(ecore_file_is_dir(filename))
            continue;

        Efreet_Desktop* d = efreet_desktop_get(filename);
        if(!d)
            continue;

#ifdef OLD_ECORE
        const char* mime_type;
        ecore_list_index_goto(d->mime_types, 0);
        while(d->mime_types && (mime_type = ecore_list_next(d->mime_types)))
        {
#else
        Eina_List* j;
        for(j = d->mime_types; j; j = eina_list_next(j))
        {
            const char* mime_type = (const char*)eina_list_data_get(j);
#endif
            Eina_List* h = eina_hash_find(handlers->handlers, mime_type);
            if(h)
            {
                h = eina_list_append(h, d);
                eina_hash_modify(handlers->handlers, mime_type, h);
            }
            else
            {
                h = eina_list_append(h, d);
                eina_hash_add(handlers->handlers, mime_type, h);
            }
        }

        handlers->desktop_files = eina_list_append(handlers->desktop_files, d);
    }

#ifdef OLD_ECORE
    ecore_list_destroy(ls);
#else
    eina_list_free(ls);
#endif

#ifdef DEBUG_HANDLERS
    handlers_dump(handlers);
#endif

    return handlers;
}

Eina_List* handlers_get(handlers_t* handlers, const char* mime_type)
{
    return eina_hash_find(handlers->handlers, mime_type);
}

void handlers_fini(handlers_t* handlers)
{
    eina_hash_free(handlers->handlers);

    Eina_List* i;
    for(i = handlers->desktop_files; i; i = eina_list_next(i))
    {
        Efreet_Desktop* desktop = (Efreet_Desktop*)eina_list_data_get(i);
        efreet_desktop_free(desktop);
    }
    eina_list_free(handlers->desktop_files);

    free(handlers);
    efreet_shutdown();
}
