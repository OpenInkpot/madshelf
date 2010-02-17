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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

const char* file_ext(const char* f)
{
    const char* c = strrchr(f, '.');
    if(c)
        return c + 1;
    else
        return f + strlen(f);
}

#ifdef DEBUG

#include <Evas.h>

void dump_evas_hier(Evas* canvas)
{
    fprintf(stderr, "dump_evas_hier\n");

    Evas_Object* i = evas_object_bottom_get(canvas);
    while(i)
    {
        const char* name = evas_object_name_get(i);
        if(!name)
            name = "NULL";

        int x, y, w, h;
        evas_object_geometry_get(i, &x, &y, &w, &h);

        const char* type = evas_object_type_get(i);
        if(!type)
            type = "NULL";

        fprintf(stderr, "0x%p %s \"%s\" %d-%d %d-%d\n", i, type, name, x, y, w, h);

        i = evas_object_above_get(i);
    }
}
#endif
