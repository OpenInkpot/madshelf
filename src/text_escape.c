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

#include <Eina.h>

char *
textblock_escape_string(const char *text)
{
    if (!text)
        return NULL;

    Eina_Strbuf *buf = eina_strbuf_new();
    eina_strbuf_append(buf, text);

    eina_strbuf_replace_all(buf, "\n", "<br>");
    eina_strbuf_replace_all(buf, "\t", "<\t>");
    eina_strbuf_replace_all(buf, "<", "&lt;");
    eina_strbuf_replace_all(buf, ">", "&gt;");
    eina_strbuf_replace_all(buf, "&", "&amp;");

    char *res = strdup(eina_strbuf_string_get(buf));
    eina_strbuf_free(buf);

    return res;
}
