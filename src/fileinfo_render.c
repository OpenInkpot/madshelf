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
#include <libintl.h>

#include <Ecore_File.h>
#include <Edje.h>
#include <Eina.h>

#include "madshelf.h"
#include "utils.h"
#include "fileinfo_render.h"

#define KILOBYTE (1024)
#define MEGABYTE (1024*1024)

char* format_size(long long size)
{
    char* res;

    if(size >= MEGABYTE)
        asprintf(&res, "%.1fM", ((double)size)/((double)MEGABYTE));
    else
        asprintf(&res, "%dk", (int)(((double)size)/((double)KILOBYTE)+0.5));
    return res;
}

bool endswith(const char* s, const char* t)
{
    int sl = strlen(s);
    int tl = strlen(t);

    if(sl < tl) return false;

    return !strcmp(s + sl - tl, t);
}

void count_files(const char* directory, int* files, int* directories)
{
    *files = 0;
    *directories = 0;

#ifdef OLD_ECORE
    Ecore_List* ls = ecore_file_ls(directory);
    const char* file;
    while((file = ecore_list_next(ls)))
    {
#else
    Eina_List* ls = ecore_file_ls(directory);
    Eina_List* i;
    for(i = ls; i; i = eina_list_next(i))
    {
        const char* file = eina_list_data_get(i);
#endif
        char* filename;
        asprintf(&filename, "%s/%s", directory, file);

        if(ecore_file_is_dir(filename))
            (*directories)++;
        else
            (*files)++;

        free(filename);
    }

#ifdef OLD_ECORE
    ecore_list_destroy(ls);
#else
    eina_list_free(ls);
#endif
}

static void _draw_title(Evas_Object* item, const char* text, bool is_dim)
{
    if(is_dim)
    {
        char* f;
        asprintf(&f, "<inactive>%s</inactive>", text);
        edje_object_part_text_set(item, "title", f);
        free(f);
    }
    else
        edje_object_part_text_set(item, "title", text);
}

void fileinfo_render(Evas_Object* item, fileinfo_t* fileinfo, bool is_dim)
{
    item_clear(item);

    if(!fileinfo->exists)
    {
        _draw_title(item, fileinfo->basename, is_dim);
        edje_object_part_text_set(item, "type", gettext("Does not exist"));
        return;
    }

    if(fileinfo->is_dir)
    {
        _draw_title(item, fileinfo->basename, is_dim);
        edje_object_part_text_set(item, "type", gettext("Directory"));
        return;
    }

    if(fileinfo->title)
        _draw_title(item, fileinfo->title, is_dim);
    else
        _draw_title(item, fileinfo->basename, is_dim);

    if(fileinfo->author)
        edje_object_part_text_set(item, "author", fileinfo->author);

    if(fileinfo->series && fileinfo->series_num != -1)
    {
        char* s;
        asprintf(&s, "%s #%d", fileinfo->series, fileinfo->series_num);
        edje_object_part_text_set(item, "series", s);
        free(s);
    }
    else if(fileinfo->series)
        edje_object_part_text_set(item, "series", fileinfo->series);

    if(fileinfo->size != -1)
    {
        char* s = format_size(fileinfo->size);
        edje_object_part_text_set(item, "size", s);
        free(s);
    }

    /* FIXME: use MIME info */
    if(endswith(fileinfo->basename, ".fb2.zip"))
    {
        edje_object_part_text_set(item, "type", "fb2.zip");
    }
    else
    {
        const char* suffix = file_ext(fileinfo->basename);
        edje_object_part_text_set(item, "type", suffix);
    }
}