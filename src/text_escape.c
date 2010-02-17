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
#include <malloc.h>
#include <Evas.h>

static char *
_str_append(char *str, const char *txt, int *len, int *alloc)
{
    int txt_len = strlen(txt);

    if (txt_len <= 0) return str;
    if ((*len + txt_len) >= *alloc)
    {
        char *str2;
        int alloc2;

        alloc2 = *alloc + txt_len + 128;
        str2 = realloc(str, alloc2);
        if (!str2) return str;
        *alloc = alloc2;
        str = str2;
    }
    strcpy(str + *len, txt);
    *len += txt_len;
    return str;
}

char* textblock_escape_string(const char *text)
{
    char *str = strdup("");
    int str_len = 0, str_alloc = 0;
    int ch, pos = 0, pos2 = 0;

    if (!text) return NULL;
    for (;;)
    {
        pos = pos2;
        pos2 = evas_string_char_next_get(text, pos2, &ch);
        if ((ch <= 0) || (pos2 <= 0)) break;
        if (ch == '\n')
            str = _str_append(str, "<br>", &str_len, &str_alloc);
        else if (ch == '\t')
            str = _str_append(str, "<\t>", &str_len, &str_alloc);
        else if (ch == '<')
            str = _str_append(str, "&lt;", &str_len, &str_alloc);
        else if (ch == '>')
            str = _str_append(str, "&gt;", &str_len, &str_alloc);
        else if (ch == '&')
            str = _str_append(str, "&amp;", &str_len, &str_alloc);
        else
        {
            char tstr[16];
            strncpy(tstr, text + pos, pos2 - pos);
            tstr[pos2 - pos] = 0;
            str = _str_append(str, tstr, &str_len, &str_alloc);
        }
    }
    return str;
}
