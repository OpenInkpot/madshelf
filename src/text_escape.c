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
#include "text_escape.h"

#include <string.h>

#include <Ecore_Data.h>

char *
textblock_escape_string(const char *text)
{
    if (!text)
        return NULL;

    Ecore_Strbuf *buf = ecore_strbuf_new();
    ecore_strbuf_append(buf, text);

    ecore_strbuf_replace_all(buf, "\n", "<br>");
    ecore_strbuf_replace_all(buf, "\t", "<\t>");
    ecore_strbuf_replace_all(buf, "<", "&lt;");
    ecore_strbuf_replace_all(buf, ">", "&gt;");
    ecore_strbuf_replace_all(buf, "&", "&amp;");

    char *res = strdup(ecore_strbuf_string_get(buf));
    ecore_strbuf_free(buf);

    return res;
}
