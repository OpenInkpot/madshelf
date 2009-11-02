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

#ifndef MADSHELF_DATABASE_H
#define MADSHELF_DATABASE_H

#include <stdbool.h>

typedef struct tags_t tags_t;

tags_t* tags_init(const char* filename, char** errstr);
void tags_fini(tags_t*);

typedef enum
{
    DB_SORT_NAME,
    DB_SORT_NAMEREV,
    DB_SORT_DATE,
} tags_sort_t;

typedef void (*tags_list_t)(const char* filename, int serial, void* param);

void tag_add(tags_t* db, const char* tag, const char* filename);
void tag_remove(tags_t* db, const char* tag, const char* filename);
bool has_tag(tags_t* db, const char* tag, const char* filename);
void tag_list(tags_t* db, const char* tag, tags_sort_t sort, tags_list_t callback, void* param);
void tag_clear(tags_t* db, const char* tag);

/* Utility */
void tag_remove_absent(tags_t* db, const char* tag);

#endif
