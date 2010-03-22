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

#include <err.h>
#include <string.h>
#include <libgen.h>

#include <Ecore_File.h>
#include <Efreet_Mime.h>

#include "filters.h"
#include "handlers.h"
#include "utils.h"

static bool
has_hidden_tag(const madshelf_state_t *state, const char *filename)
{
    char* filename_copy = strdup(filename);
    char* b = basename(filename_copy);
    bool is_hidden = *b == '.' || has_tag(state->tags, "hidden", filename);
    free(filename_copy);
    return is_hidden;
}

bool
is_hidden(const madshelf_state_t *state, const char *filename)
{
    if (!ecore_file_exists(filename)) return true;
    if (has_hidden_tag(state, filename)) return true;
    if (ecore_file_is_dir(filename)) return false;

    if (state->filter == MADSHELF_FILTER_NO)
        return false;

    const char *mime_type = efreet_mime_type_get(filename);
    openers_t *openers = openers_get(mime_type);
    if (!openers)
        return true;

    if (state->filter == MADSHELF_FILTER_BOOKS)
        return !(openers->app_types & OPENERS_TYPE_BOOKS);
    if (state->filter == MADSHELF_FILTER_IMAGE)
        return !(openers->app_types & OPENERS_TYPE_IMAGE);
    if (state->filter == MADSHELF_FILTER_AUDIO)
        return !(openers->app_types & OPENERS_TYPE_AUDIO);

    errx(1, "is_visible: Unknown filter type: %d", state->filter);
}
