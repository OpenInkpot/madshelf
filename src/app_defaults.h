/*
 * MadShelf - bookshelf application.
 *
 * Copyright © 2009 Mikhail Gusarov <dottedmag@dottedmag.net>
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
#ifndef APP_DEFAULTS_H
#define APP_DEFAULTS_H

#include <Eina.h>

/* Result is not to be freed */
const char* appdef_get_preferred(const char* mime_type);

/* Only list is to be freed, not the strings */
Eina_List* appdef_get_list(const char* mime_type);

void appdef_set_default(const char* mime_type, const char* app);

void appdef_fini();

#endif
