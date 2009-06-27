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
#include <string.h>

#include <Ecore_File.h>
#include <Efreet_Mime.h>
#include <Eina.h>

#include "fileinfo.h"
#include "utils.h"
#include "madshelf_extractors.h"

#define CACHE_LIMIT 32

static int init_cnt;
static extractors_t* extractors;

static int cache_size;
static Eina_Hash* infos;

void fileinfo_init()
{
    if(!init_cnt++)
    {
        if(!efreet_mime_init())
            die("fileinfo_init: Unable to initialize Efreet_Mime subsystem");
        if(!eina_hash_init())
            die("fileinfo_init: Unable to initialize Eina_Hash subsystem");
        infos = eina_hash_string_superfast_new((Eina_Free_Cb)&fileinfo_destroy);

        extractors = load_extractors();
        cache_size = 0;
    }
}

static fileinfo_t* fileinfo_copy(const fileinfo_t* info)
{
    fileinfo_t* copy = malloc(sizeof(fileinfo_t));
    if(!copy)
        return NULL;

    memcpy(copy, info, sizeof(fileinfo_t));
    copy->filename = strdup(info->filename);
    copy->basename = strdup(info->basename);
    copy->author = info->author ? strdup(info->author) : NULL;
    copy->mime_type = info->mime_type ? strdup(info->mime_type) : NULL;
    copy->title = info->title ? strdup(info->title) : NULL;
    copy->series = info->series ? strdup(info->series) : NULL;

    return copy;
}

static fileinfo_t* fileinfo_parse(const char* filename)
{
    fileinfo_t* i = calloc(1, sizeof(fileinfo_t));
    i->filename = strdup(filename);
    i->basename = strdup(basename(filename));
    i->exists = ecore_file_exists(filename);
    if(i->exists)
    {
        i->is_dir = ecore_file_is_dir(filename);
        i->mtime = ecore_file_mod_time(filename);
        i->size = ecore_file_size(filename);
    }
    else
    {
        i->mtime = -1;
        i->size = -1;
    }
    i->series_num = -1;

    if(i->exists && !i->is_dir)
    {
        EXTRACTOR_KeywordList* keywords = extractor_get_keywords(extractors, filename);
        EXTRACTOR_KeywordList* j;
        for(j = keywords; j; j = j->next)
        {
            if(j->keywordType == EXTRACTOR_AUTHOR)
            {
                if(!i->author)
                    i->author = strdup(j->keyword);
                else
                {
                    char* authors;
                    asprintf(&authors, "%s, %s", i->author, j->keyword);
                    free(i->author);
                    i->author = authors;
                }
            }
            if(j->keywordType == EXTRACTOR_MIMETYPE)
                i->mime_type = strdup(j->keyword);
            if(j->keywordType == EXTRACTOR_TITLE)
                i->title = strdup(j->keyword);
            if(j->keywordType == EXTRACTOR_ALBUM)
                i->series = strdup(j->keyword);
            if(j->keywordType == EXTRACTOR_TRACK_NUMBER)
                i->series_num = atoi(j->keyword);
        }

        if(!i->mime_type)
            i->mime_type = strdup(efreet_mime_type_get(i->filename));
    }

    return i;
}

Eina_Bool _del_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
    eina_hash_del((Eina_Hash*)hash, key, NULL);
    return 0;
}

fileinfo_t* fileinfo_create(const char* filename)
{
    fileinfo_t* i = eina_hash_find(infos, filename);
    if(!i)
    {
        i = fileinfo_parse(filename);
        if(cache_size >= CACHE_LIMIT)
        {
            eina_hash_foreach(infos, _del_cb, NULL);
            cache_size--;
        }

        eina_hash_add(infos, filename, i);
    }
    else
    {
        long long mtime = ecore_file_exists(filename) ? ecore_file_mod_time(filename) : -1;
        if(mtime != i->mtime)
        {
            i = fileinfo_parse(filename);
            eina_hash_del(infos, filename, NULL);
            eina_hash_add(infos, filename, i);
        }
    }
    return fileinfo_copy(i);
}

void fileinfo_destroy(fileinfo_t* fileinfo)
{
    free(fileinfo->basename);
    free(fileinfo->filename);
    free(fileinfo->mime_type);
    free(fileinfo->author);
    free(fileinfo->title);
    free(fileinfo->series);
    free(fileinfo);
}

void fileinfo_fini()
{
    if(!--init_cnt)
    {
        unload_extractors(extractors);

        eina_hash_free(infos);
        infos = NULL;
        eina_hash_shutdown();
        efreet_mime_shutdown();
    }
}
