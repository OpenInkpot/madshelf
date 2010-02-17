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

#include <err.h>

#include <Ecore_File.h>
#include <Efreet_Mime.h>

#include "filters.h"
#include "handlers.h"
#include "utils.h"

bool is_visible(madshelf_filter_t filter, const char* filename)
{
    if(filter == MADSHELF_FILTER_NO)
        return true;

    if(!ecore_file_exists(filename)) return true;
    if(ecore_file_is_dir(filename)) return true;

    const char* mime_type = efreet_mime_type_get(filename);
    openers_t* openers = openers_get(mime_type);
    if(!openers)
        return false;

    if(filter == MADSHELF_FILTER_BOOKS)
        return openers->app_types & OPENERS_TYPE_BOOKS;
    if(filter == MADSHELF_FILTER_IMAGE)
        return openers->app_types & OPENERS_TYPE_IMAGE;
    if(filter == MADSHELF_FILTER_AUDIO)
        return openers->app_types & OPENERS_TYPE_AUDIO;

    errx(1, "is_visible: Unknown filter type: %d", filter);
}
