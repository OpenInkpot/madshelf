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

#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdbool.h>
#include <Eina.h>

typedef enum
{
    OPENERS_TYPE_NONE  = 0x0,
    OPENERS_TYPE_BOOKS = 0x1,
    OPENERS_TYPE_IMAGE = 0x2,
    OPENERS_TYPE_AUDIO = 0x4,
} openers_app_type_t;

typedef struct
{
    openers_app_type_t app_types;
    bool has_default;
    Eina_List* apps; /* List of Efreet_Desktop* */
} openers_t;

void openers_init();

/*
 * Returns openers_t for given mime type.
 *
 * To be freed by eina_list_free()
 */
openers_t* openers_get(const char* mime_type);

void openers_set_default(const char* mime_type, Efreet_Desktop* app);

void openers_fini();

#endif
