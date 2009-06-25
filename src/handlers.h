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

#include <Eina.h>

typedef struct handlers_t handlers_t;

handlers_t* handlers_init();

/*
 * Returns list of Efreet_Desktop* handlers for given mime type, sorted by the
 * decreasing of priority. Returns empty list if no handler exists for given
 * mime type.
 *
 * To be freed by eina_list_free().
 */
Eina_List* handlers_get(handlers_t* handlers, const char* mime_type);

void handlers_fini(handlers_t* handlers);

#endif
